/**
 * @file Transport.h
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

#pragma once
#include <cstdint>

class CdcTransport {
  public:
    /** Register this instance as the singleton the CDC ISR forwards to. */
    void begin();

    /** Queue a whole frame for transmission; false if the TX ring is full. */
    bool write(const uint8_t* d, uint16_t n);

    /** Copy up to max bytes currently available from the RX ring (non-blocking). */
    uint16_t read(uint8_t* dst, uint16_t max);

    /**
     * @brief Drop every in-flight TX transfer and re-arm the ring.
     *
     * Called on CDC (re)configuration. Without it a TX outstanding when the
     * host vanishes leaves _busy latched true forever: pump() then returns at
     * its first line for the rest of the run, the ring fills, and write() fails
     * silently. The USB stack clears its own TxState in USBD_CDC_Init, so this
     * is the missing half of that reset. Only matters when the MCU survives the
     * disconnect (externally powered); on USB power the reboot does it for us.
     */
    void reset();

    /**
     * @brief Force-release a TX that never completed. Call periodically from
     *        the comms thread (task context - it reads the FreeRTOS tick).
     *
     * Covers the case reset() cannot: a completion lost to a USB suspend or a
     * host driver reset, where CDC_Init never runs, so nothing re-arms us.
     * @param nowTicks xTaskGetTickCount() from the caller.
     */
    void serviceTx(uint32_t nowTicks);

    /** Frames dropped because the ring was full (diagnostic). */
    uint16_t txDropped() const { return _txDropped; }
    /** TX transfers force-released by serviceTx() (diagnostic). */
    uint16_t txTimeouts() const { return _txTimeouts; }

    /* ---- called from the CDC ISR (via the C bridge in Bridge.cpp) -------- */
    void onRxISR(const uint8_t* d, uint16_t n);
    void onTxCpltISR();

    /* Static forwarders to the singleton transport: the C bridge calls these. */
    static void dispatchRxISR(const uint8_t* d, uint16_t n);
    static void dispatchTxCpltISR();
    static void dispatchReset();

  private:
    void pump(); // start at most one IN packet; runs in thread (IRQ-masked) or ISR

    static constexpr uint16_t RX_BUF_SIZE = 2048; // RX byte ring
    static constexpr uint16_t MAX_FRAME = 135;   // SerialProtocol<128>: 128 + 7
    static constexpr uint8_t RING_N = 8;         // frames in flight
    static constexpr uint16_t USB_PACKET = 64;   // USB FS bulk max packet

    /* A 64-byte FS bulk IN completes in well under a millisecond when the host
     * is polling. 100 ms of "busy" therefore means the transfer is never coming
     * back, not that the link is merely slow. */
    static constexpr uint32_t TX_TIMEOUT_TICKS = 100; // configTICK_RATE_HZ = 1000

    /* RX: single-producer (ISR) / single-consumer (thread) byte ring */
    volatile uint8_t _rxBuf[RX_BUF_SIZE];
    volatile uint16_t _rxHead = 0; // written by ISR
    volatile uint16_t _rxTail = 0; // written by thread

    /* TX: frame ring drained one USB packet at a time */
    struct OutFrame {
        uint8_t buf[MAX_FRAME];
        uint16_t len;
    };
    OutFrame _ring[RING_N];
    volatile uint16_t _head = 0; // consumer (ISR/pump)
    volatile uint16_t _tail = 0; // producer (write)
    uint16_t _off = 0;           // bytes of the head frame already sent
    volatile bool _busy = false; // an IN packet is outstanding
    volatile bool _zlp = false;  // a terminating zero-length packet is owed

    /* Stall recovery + diagnostics */
    volatile uint32_t _busySince = 0; // tick _busy was first observed set (0 = idle)
    volatile uint16_t _txDropped = 0; // frames lost to a full ring
    volatile uint16_t _txTimeouts = 0; // transfers force-released by serviceTx()
};

