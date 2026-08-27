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
    bool write(const uint8_t* data, uint16_t len);

    /** Largest frame a TX ring slot can hold. Public so the protocol layer can
     *  assert it is big enough - see the static_assert in SerialThread.h. It is
     *  stated here rather than derived because a transport must not depend on
     *  the protocol riding on it; the assert is what keeps the two honest. */
    static constexpr uint16_t MAX_FRAME = 135;   // SerialProtocol<128>: 128 + 7

    /** Packets NAK'd back to the host because the ring was full. NOT a loss
     *  count: every one of these was retried and eventually delivered. A rising
     *  value means the consumer is not draining fast enough, nothing worse. */
    uint32_t rxDeferred() const { return _rxDeferred; }

    /** USB FS bulk max packet. Public because it is the natural size for a
     *  caller's read() buffer: the host can never hand the device more than one
     *  packet at a time, so a buffer this big drains everything a single ISR
     *  delivered, and a bigger one only ever comes back part full. */
    static constexpr uint16_t USB_PACKET = 64;

    /** Copy up to max bytes currently available from the RX ring (non-blocking). */
    uint16_t read(uint8_t* dest, uint16_t maxLen);

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
    void serviceLink(uint32_t nowMs);

    /** True ONCE per link re-establishment. On this side "the link came back"
     *  means CDC_Init_FS ran: the host (re)configured the interface, so anything
     *  queued for the previous one is meaningless. Consumed by the protocol's
     *  tick(), which resyncs its parser on it. */
    bool linkReset();

    /** Frames dropped because the ring was full (diagnostic). */
    uint16_t txDropped() const { return _txDropped; }
    /** TX transfers force-released by serviceLink() (diagnostic). */
    uint16_t txTimeouts() const { return _txTimeouts; }

    /* ---- called from the CDC ISR (via the C bridge in Bridge.cpp) -------- */
    /** ISR side. All-or-nothing: copies the WHOLE packet or none of it, and
     *  returns false when the ring cannot take it. The caller must then NOT
     *  re-arm the endpoint - the host controller NAKs and retries, so the bytes
     *  wait on the wire instead of being destroyed. read() re-arms once it has
     *  drained room. This is what makes RX loss structurally impossible rather
     *  than merely counted. */
    bool onRxISR(const uint8_t* data, uint16_t len);
    void onTxCpltISR();

    /* Static forwarders to the singleton transport: the C bridge calls these. */
    static bool dispatchRxISR(const uint8_t* data, uint16_t len);
    static void dispatchTxCpltISR();
    static void dispatchReset();

  private:
    void pump(); // start at most one IN packet; runs in thread (IRQ-masked) or ISR

    /* RX byte ring. Sized in CHUNKS, not in bytes: the protocol drains
     * RX_CHUNK (1216) per tick, so this is ~6.7 ticks of slack for when the
     * comms thread is held off by something higher-priority. At 2048 it was
     * 1.7, which the NAK backpressure made lossless rather than destructive -
     * but every overflow still costs a host retry, and this is cheap: 128 KB of
     * RAM with ~50 KB free. */
    static constexpr uint16_t RX_BUF_SIZE = 8192;
    static constexpr uint8_t RING_N = 8;         // frames in flight

    /* A 64-byte FS bulk IN completes in well under a millisecond when the host
     * is polling. 100 ms of "busy" therefore means the transfer is never coming
     * back, not that the link is merely slow. */
    static constexpr uint32_t TX_TIMEOUT_TICKS = 100; // configTICK_RATE_HZ = 1000

    /* RX: single-producer (ISR) / single-consumer (thread) byte ring */
    volatile uint8_t _rxBuf[RX_BUF_SIZE];
    volatile uint16_t _rxHead = 0; // written by ISR
    volatile uint16_t _rxTail = 0; // written by thread
    volatile bool     _rxArmPending = false; // a packet was NAK'd; re-arm after draining
    volatile bool     _rxFlush = false;      // ISR asked for a ring flush; read() does it
    volatile bool     _linkReset = false;    // CDC (re)configured; consumed by linkReset()
    volatile uint32_t _rxDeferred = 0;       // packets pushed back, NOT losses

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
    volatile bool _zeroLengthPacket = false;  // a terminating zero-length packet is owed

    /* Stall recovery + diagnostics */
    volatile uint32_t _busySince = 0; // tick _busy was first observed set (0 = idle)
    volatile uint16_t _txDropped = 0; // frames lost to a full ring
    volatile uint16_t _txTimeouts = 0; // transfers force-released by serviceLink()
};

