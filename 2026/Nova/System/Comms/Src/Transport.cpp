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
    }
    taskEXIT_CRITICAL();
    return ok;
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
