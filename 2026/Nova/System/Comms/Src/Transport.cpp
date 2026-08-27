/**
 * @file Transport.cpp
 * @brief CdcTransport implementation (see Transport.h).
 */

#include "Transport.h"

#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "usbd_cdc_if.h" // CDC_Transmit_FS, USBD_OK

/* The single transport the CDC ISR forwards to (set in begin()). */
static CdcTransport* gCdc = nullptr;

void CdcTransport::begin() {
    gCdc = this;
}

/* ---- RX: ISR producer, thread consumer (SPSC byte ring) ---------------- */

void CdcTransport::onRxISR(const uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; ++i) {
        uint16_t next = static_cast<uint16_t>((_rxHead + 1) % RX_BUF_SIZE);
        if (next == _rxTail) break; // ring full -> drop (the framing FSM resyncs)
        _rxBuf[_rxHead] = data[i];
        _rxHead = next;
    }
}

uint16_t CdcTransport::read(uint8_t* destination, uint16_t maxLen) {
    uint16_t count = 0;
    while (count < maxLen && _rxTail != _rxHead) {
    	destination[count++] = _rxBuf[_rxTail];
        _rxTail = static_cast<uint16_t>((_rxTail + 1) % RX_BUF_SIZE);
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
    _zeroLengthPacket = false;
    _busy = false;       // the missing half of USBD_CDC_Init's TxState = 0
    _busySince = 0;
}

void CdcTransport::serviceTx(uint32_t nowTicks) {
    taskENTER_CRITICAL();
    if (!_busy) {
        _busySince = 0;                  // idle, or a transfer completed normally
    } else if (_busySince == 0) {
        _busySince = nowTicks ? nowTicks : 1; // first tick we saw it busy (0 = sentinel)
    } else if ((nowTicks - _busySince) >= TX_TIMEOUT_TICKS) {
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

void CdcTransport::dispatchRxISR(const uint8_t* data, uint16_t len) {
    if (gCdc) gCdc->onRxISR(data, len);
}

void CdcTransport::dispatchTxCpltISR() {
    if (gCdc) gCdc->onTxCpltISR();
}

void CdcTransport::dispatchReset() {
    if (gCdc) gCdc->reset();
}
