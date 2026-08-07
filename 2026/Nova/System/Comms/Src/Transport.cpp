/**
 * @file Transport.cpp
 * @brief CdcTransport implementation (see Transport.hpp).
 */

#include "Transport.hpp"

#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "usbd_cdc_if.h" // CDC_Transmit_FS, USBD_OK

/* The single transport the CDC ISR forwards to (set in begin()). */
static CdcTransport* g_cdc = nullptr;

void CdcTransport::begin() {
    g_cdc = this;
}

/* ---- RX: ISR producer, thread consumer (SPSC byte ring) ---------------- */

void CdcTransport::onRxISR(const uint8_t* d, uint16_t n) {
    for (uint16_t i = 0; i < n; ++i) {
        uint16_t next = static_cast<uint16_t>((rxHead_ + 1) % kRxBufSize);
        if (next == rxTail_) break; // ring full -> drop (the framing FSM resyncs)
        rxBuf_[rxHead_] = d[i];
        rxHead_ = next;
    }
}

uint16_t CdcTransport::read(uint8_t* dst, uint16_t max) {
    uint16_t cnt = 0;
    while (cnt < max && rxTail_ != rxHead_) {
        dst[cnt++] = rxBuf_[rxTail_];
        rxTail_ = static_cast<uint16_t>((rxTail_ + 1) % kRxBufSize);
    }
    return cnt;
}

/* ---- TX: completion-driven frame ring ---------------------------------- */

bool CdcTransport::write(const uint8_t* d, uint16_t n) {
    if (n > kMaxFrame) return false;
    bool ok = false;
    taskENTER_CRITICAL(); // masks the USB IRQ: ring update + kick are atomic vs onTxCpltISR
    uint16_t next = static_cast<uint16_t>((tail_ + 1) % kRingN);
    if (next != head_) {  // not full
        std::memcpy(ring_[tail_].buf, d, n);
        ring_[tail_].len = n;
        tail_ = next;
        pump();
        ok = true;
    } else {
        ++txDropped_; // ring full: caller already dequeued this frame, so it is lost
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
    head_ = tail_ = 0;   // discard anything queued for the link that just died
    off_ = 0;
    zlp_ = false;
    busy_ = false;       // the missing half of USBD_CDC_Init's TxState = 0
    busySince_ = 0;
}

void CdcTransport::serviceTx(uint32_t now_ticks) {
    taskENTER_CRITICAL();
    if (!busy_) {
        busySince_ = 0;                  // idle, or a transfer completed normally
    } else if (busySince_ == 0) {
        busySince_ = now_ticks ? now_ticks : 1; // first tick we saw it busy (0 = sentinel)
    } else if ((now_ticks - busySince_) >= kTxTimeoutTicks) {
        // The completion is never coming. Releasing the flag is safe even if the
        // transfer is somehow still live: CDC_Transmit_FS re-checks TxState and
        // returns USBD_BUSY, which pump() handles by leaving the frame queued.
        busy_ = false;
        busySince_ = 0;
        ++txTimeouts_;
        pump();
    }
    taskEXIT_CRITICAL();
}

void CdcTransport::onTxCpltISR() {
    busy_ = false;
    OutFrame& f = ring_[head_];
    if (zlp_) {
        zlp_ = false; // the ZLP finished the frame
        head_ = static_cast<uint16_t>((head_ + 1) % kRingN);
        off_ = 0;
    } else if (off_ == f.len) {
        if (f.len % kUsbPacket == 0) {
            zlp_ = true; // ended on a 64-B boundary -> owe a ZLP
        } else {
            head_ = static_cast<uint16_t>((head_ + 1) % kRingN);
            off_ = 0;
        }
    }
    pump();
}

void CdcTransport::pump() {
    if (busy_) return;
    if (head_ == tail_) return; // nothing queued
    OutFrame& f = ring_[head_];
    busy_ = true;
    if (zlp_) {
        if (CDC_Transmit_FS(f.buf, 0) != USBD_OK) busy_ = false; // retry on next kick
        return;
    }
    uint16_t rem = static_cast<uint16_t>(f.len - off_);
    uint16_t n = rem < kUsbPacket ? rem : kUsbPacket;
    if (CDC_Transmit_FS(f.buf + off_, n) != USBD_OK) {
        busy_ = false; // endpoint busy -> leave queued; next write()/TxCplt re-pumps
        return;
    }
    off_ = static_cast<uint16_t>(off_ + n);
}

/* ---- static forwarders to the singleton (called by the C bridge) -------- */

void CdcTransport::dispatchRxISR(const uint8_t* d, uint16_t n) {
    if (g_cdc) g_cdc->onRxISR(d, n);
}

void CdcTransport::dispatchTxCpltISR() {
    if (g_cdc) g_cdc->onTxCpltISR();
}

void CdcTransport::dispatchReset() {
    if (g_cdc) g_cdc->reset();
}
