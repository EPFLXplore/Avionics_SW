/**
 * @file Transport.cpp
 * @brief CdcTransport implementation (see Transport.h).
 */

#include "Transport.h"

#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "usbd_cdc_if.h" // CDC_Transmit_FS, USBD_OK
#include "Bridge.h"       // Cdc_ArmRx()

/* The single transport the CDC ISR forwards to (set in begin()). */
static CdcTransport* gCdc = nullptr;

void CdcTransport::begin() {
    gCdc = this;
}

/* ---- RX: ISR producer, thread consumer (SPSC byte ring) ---------------- */

bool CdcTransport::onRxISR(const uint8_t* data, uint16_t len) {
    /* Free space, computed once. One slot is always left empty so head==tail
     * means empty rather than full. */
    const uint16_t head = _rxHead, tail = _rxTail;
    const uint16_t used = static_cast<uint16_t>((head - tail) % RX_BUF_SIZE);
    const uint16_t room = static_cast<uint16_t>(RX_BUF_SIZE - 1 - used);

    /* ALL OR NOTHING. Copying a prefix is what used to destroy frames: the tail
     * of the packet vanished, the parser sat in Payload waiting for bytes that
     * no longer existed, and it then ate the next frame's header. Refusing the
     * whole packet leaves it on the wire for the host to retry. */
    if (len > room) {
        ++_rxDeferred;
        _rxArmPending = true;
        return false;
    }

    uint16_t next = head;
    for (uint16_t i = 0; i < len; ++i) {
        _rxBuf[next] = data[i];
        next = static_cast<uint16_t>((next + 1) % RX_BUF_SIZE);
    }
    _rxHead = next;
    return true;
}

uint16_t CdcTransport::read(uint8_t* destination, uint16_t maxLen) {
    /* Thread context, so this is where the ISR's flush request is honoured. */
    if (_rxFlush) {
        _rxFlush = false;
        _rxTail  = _rxHead;   // discard the previous link's bytes
    }

    uint16_t count = 0;
    while (count < maxLen && _rxTail != _rxHead) {
    	destination[count++] = _rxBuf[_rxTail];
        _rxTail = static_cast<uint16_t>((_rxTail + 1) % RX_BUF_SIZE);
    }

    /* Room again: take the endpoint off NAK. Deliberately after the drain, so
     * the host only resumes once there is somewhere to put the retry. */
    if (_rxArmPending && count) {
        _rxArmPending = false;
        Cdc_ArmRx();
    }
    return count;
}

/* ---- TX: completion-driven frame ring ---------------------------------- */

bool CdcTransport::write(const uint8_t* data, uint16_t len) {
    if (len > MAX_FRAME) return false;
    bool ok = false;
    taskENTER_CRITICAL(); // masks the USB IRQ: ring update + kick are atomic vs onTxCpltISR
    uint16_t next = static_cast<uint16_t>((_tail + 1) % RING_N);
    if (next != _head) {  // not full
        std::memcpy(_ring[_tail].buf, data, len);
        _ring[_tail].len = len;
        _tail = next;
        pump();
        ok = true;
    } else {
        ++_txDropped; // ring full: caller already dequeued this frame, so it is lost
    }
    taskEXIT_CRITICAL();
    return ok;
}

void CdcTransport::reset() {
    /* ISR CONTEXT. Reached from CDC_Init_FS, which the stack calls while
     * handling SET_CONFIGURATION inside the USB IRQ. Do NOT take a critical
     * section here: taskENTER_CRITICAL() asserts when called from an ISR, which
     * hangs the interrupt handler and makes the host time out the control
     * transfer ("can't set config #1, error -110" -> the device never
     * enumerates at all).
     *
     * No guard is needed anyway. The only concurrent writer is write(), running
     * on the comms task, and it masks this very IRQ for its whole body - so it
     * and this function can never interleave. */
    _head = _tail = 0;   // discard anything queued for the link that just died
    _off = 0;

    /* RX is stale too: bytes from the previous link cannot be part of any frame
     * the new one will send. Flagged rather than done here - _rxTail belongs to
     * the thread, and an ISR writing it would race the drain. read() honours it.
     *
     * _linkReset rides along so the protocol can resync its parser: a frame left
     * half-built when the cable went away must not be spliced onto the first
     * frame of the new link. */
    _rxFlush   = true;
    _linkReset = true;
    _zeroLengthPacket = false;
    _busy = false;       // the missing half of USBD_CDC_Init's TxState = 0
    _busySince = 0;
}

void CdcTransport::serviceLink(uint32_t nowMs) {
    /* A re-arm that never happened is a dead link, so do not rely on read()
     * alone to clear it: read() only re-arms when it actually moved bytes, and a
     * refusal that lands when the consumer has already drained would otherwise
     * sit here forever. Cheap, and it makes the failure self-healing. */
    if (_rxArmPending && _rxTail == _rxHead) {
        _rxArmPending = false;
        Cdc_ArmRx();
    }

    taskENTER_CRITICAL();
    if (!_busy) {
        _busySince = 0;                  // idle, or a transfer completed normally
    } else if (_busySince == 0) {
        _busySince = nowMs ? nowMs : 1; // first tick we saw it busy (0 = sentinel)
    } else if ((nowMs - _busySince) >= TX_TIMEOUT_TICKS) {
        // The completion is never coming. Releasing the flag is safe even if the
        // transfer is somehow still live: CDC_Transmit_FS re-checks TxState and
        // returns USBD_BUSY, which pump() handles by leaving the frame queued.
        _busy = false;
        _busySince = 0;
        ++_txTimeouts;
        pump();
    }
    taskEXIT_CRITICAL();
}

void CdcTransport::onTxCpltISR() {
    _busy = false;
    OutFrame& frame = _ring[_head];
    if (_zeroLengthPacket) {
        _zeroLengthPacket = false; // the ZLP finished the frame
        _head = static_cast<uint16_t>((_head + 1) % RING_N);
        _off = 0;
    } else if (_off == frame.len) {
        if (frame.len % USB_PACKET == 0) {
            _zeroLengthPacket = true; // ended on a 64-B boundary -> owe a ZLP
        } else {
            _head = static_cast<uint16_t>((_head + 1) % RING_N);
            _off = 0;
        }
    }
    pump();
}

void CdcTransport::pump() {
    if (_busy) return;
    if (_head == _tail) return; // nothing queued
    OutFrame& frame = _ring[_head];
    _busy = true;
    if (_zeroLengthPacket) {
        if (CDC_Transmit_FS(frame.buf, 0) != USBD_OK) _busy = false; // retry on next kick
        return;
    }
    uint16_t remaining = static_cast<uint16_t>(frame.len - _off);
    uint16_t chunkLen = remaining < USB_PACKET ? remaining : USB_PACKET;
    if (CDC_Transmit_FS(frame.buf + _off, chunkLen) != USBD_OK) {
        _busy = false; // endpoint busy -> leave queued; next write()/TxCplt re-pumps
        return;
    }
    _off = static_cast<uint16_t>(_off + chunkLen);
}

/* ---- static forwarders to the singleton (called by the C bridge) -------- */

bool CdcTransport::linkReset() {
    const bool was = _linkReset;
    _linkReset = false;
    return was;
}

bool CdcTransport::dispatchRxISR(const uint8_t* data, uint16_t len) {
    /* No transport registered yet. begin() runs in TASK context, and the host
     * can write long before that - Nexus sends its flush preamble the instant it
     * opens the port, and it is already running and retrying when a board
     * enumerates. There is nothing to hold these bytes, so they are dropped.
     *
     * But TRUE, not false. False here means "refused, leave the endpoint
     * un-armed and let the host retry" - and nothing would ever re-arm it,
     * because read() only does so when a refusal is outstanding and there is a
     * transport to have recorded one. The endpoint would stay dark for the life
     * of the boot: TX keeps heartbeating, RX never receives another byte, and
     * every command of every id is silently lost. */
    if (!gCdc) return true;

    return gCdc->onRxISR(data, len);
}

void CdcTransport::dispatchTxCpltISR() {
    if (gCdc) gCdc->onTxCpltISR();
}

void CdcTransport::dispatchReset() {
    if (gCdc) gCdc->reset();
}
