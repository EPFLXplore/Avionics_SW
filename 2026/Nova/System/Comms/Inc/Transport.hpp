/**
 * @file Transport.hpp
 * @brief CdcTransport: the STM32 side of the SerialProtocol Transport concept.
 *
 * Mirror of the RPi's PosixTransport. Provides exactly the two buffer-oriented
 * methods SerialProtocol<MaxPayload, Transport> needs:
 *
 *      bool     write(const uint8_t* d, uint16_t n);  // whole frame -> TX ring
 *      uint16_t read (uint8_t* dst, uint16_t max);    // bytes from the RX ring
 *
 * RX: a lock-free SPSC byte ring filled from the CDC OUT ISR (onRxISR) and
 *     drained by the comms thread (read). No FreeRTOS StreamBuffer dependency.
 * TX: a completion-driven frame ring: write() queues a whole frame and pumps;
 *     CDC_TransmitCplt (onTxCpltISR) ships the next packet. The thread never
 *     spins on USBD_BUSY. ZLP is appended when a frame ends on a 64-B boundary.
 */

#ifndef CDC_TRANSPORT_HPP
#define CDC_TRANSPORT_HPP

#include <cstdint>

class CdcTransport {
  public:
    /** Register this instance as the singleton the CDC ISR forwards to. */
    void begin();

    /** Queue a whole frame for transmission; false if the TX ring is full. */
    bool write(const uint8_t* d, uint16_t n);

    /** Copy up to max bytes currently available from the RX ring (non-blocking). */
    uint16_t read(uint8_t* dst, uint16_t max);

    /* ---- called from the CDC ISR (via the C bridge in Bridge.cpp) -------- */
    void onRxISR(const uint8_t* d, uint16_t n);
    void onTxCpltISR();

    /* Static forwarders to the singleton transport: the C bridge calls these. */
    static void dispatchRxISR(const uint8_t* d, uint16_t n);
    static void dispatchTxCpltISR();

  private:
    void pump(); // start at most one IN packet; runs in thread (IRQ-masked) or ISR

    static constexpr uint16_t kRxBufSize = 2048; // RX byte ring
    static constexpr uint16_t kMaxFrame = 135;   // SerialProtocol<128>: 128 + 7
    static constexpr uint8_t kRingN = 8;         // frames in flight
    static constexpr uint16_t kUsbPacket = 64;   // USB FS bulk max packet

    /* RX: single-producer (ISR) / single-consumer (thread) byte ring */
    volatile uint8_t rxBuf_[kRxBufSize];
    volatile uint16_t rxHead_ = 0; // written by ISR
    volatile uint16_t rxTail_ = 0; // written by thread

    /* TX: frame ring drained one USB packet at a time */
    struct OutFrame {
        uint8_t buf[kMaxFrame];
        uint16_t len;
    };
    OutFrame ring_[kRingN];
    volatile uint16_t head_ = 0; // consumer (ISR/pump)
    volatile uint16_t tail_ = 0; // producer (write)
    uint16_t off_ = 0;           // bytes of the head frame already sent
    volatile bool busy_ = false; // an IN packet is outstanding
    volatile bool zlp_ = false;  // a terminating zero-length packet is owed
};

#endif /* CDC_TRANSPORT_HPP */
