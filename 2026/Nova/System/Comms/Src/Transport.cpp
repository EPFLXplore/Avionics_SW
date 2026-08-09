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

void CdcTransport::onRxISR(const uint8_t* d, uint16_t n) {
    for (uint16_t i = 0; i < n; ++i) {
        uint16_t next = static_cast<uint16_t>((_rxHead + 1) % RX_BUF_SIZE);
        if (next == _rxTail) break; // ring full -> drop (the framing FSM resyncs)
        _rxBuf[_rxHead] = d[i];
        _rxHead = next;
    }
}

uint16_t CdcTransport::read(uint8_t* dst, uint16_t max) {
    uint16_t cnt = 0;
    while (cnt < max && _rxTail != _rxHead) {
        dst[cnt++] = _rxBuf[_rxTail];
        _rxTail = static_cast<uint16_t>((_rxTail + 1) % RX_BUF_SIZE);
    }
    return cnt;
}

/* ---- TX: completion-driven frame ring ---------------------------------- */

bool CdcTransport::write(const uint8_t* d, uint16_t n) {
    if (n > MAX_FRAME) return false;
    bool ok = false;
    taskENTER_CRITICAL(); // masks the USB IRQ: ring update + kick are atomic vs onTxCpltISR
    uint16_t next = static_cast<uint16_t>((_tail + 1) % RING_N);
    if (next != _head) {  // not full
        std::memcpy(_ring[_tail].buf, d, n);
        _ring[_tail].len = n;
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
    _zlp = false;
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
    OutFrame& f = _ring[_head];
    if (_zlp) {
        _zlp = false; // the ZLP finished the frame
        _head = static_cast<uint16_t>((_head + 1) % RING_N);
        _off = 0;
    } else if (_off == f.len) {
        if (f.len % USB_PACKET == 0) {
            _zlp = true; // ended on a 64-B boundary -> owe a ZLP
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
    OutFrame& f = _ring[_head];
    _busy = true;
    if (_zlp) {
        if (CDC_Transmit_FS(f.buf, 0) != USBD_OK) _busy = false; // retry on next kick
        return;
    }
    uint16_t rem = static_cast<uint16_t>(f.len - _off);
    uint16_t n = rem < USB_PACKET ? rem : USB_PACKET;
    if (CDC_Transmit_FS(f.buf + _off, n) != USBD_OK) {
        _busy = false; // endpoint busy -> leave queued; next write()/TxCplt re-pumps
        return;
    }
    _off = static_cast<uint16_t>(_off + n);
}

/* ---- static forwarders to the singleton (called by the C bridge) -------- */

void CdcTransport::dispatchRxISR(const uint8_t* d, uint16_t n) {
    if (gCdc) gCdc->onRxISR(d, n);
}

void CdcTransport::dispatchTxCpltISR() {
    if (gCdc) gCdc->onTxCpltISR();
}

void CdcTransport::dispatchReset() {
    if (gCdc) gCdc->reset();
}
