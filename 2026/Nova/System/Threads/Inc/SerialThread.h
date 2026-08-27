/**
 * @file SerialThread.h
 * @brief The MCU wire-owner: mirror of the RPi's Nexus.
 *
 * One thread owns the USB link. RX: drain the byte ring through the framing FSM
 * and push each command into the target worker's queue (MessageThread
 * pushCommand). TX: drain the workers' status queues (popStatus) onto the wire.
 * It is the only caller of _proto.send()/parse(), so the wire has one owner.
 */

#pragma once
#include "Thread.h"
#include "SerialProtocol.h"
#include "Transport.h"

class SerialThread : public Thread {
  public:
    SerialThread(const char* name, osPriority priority);

    void init() override;
    void loop() override;

  public:
    /* Public so SerialThread.cpp's frame-size asserts can name FRAME_OVERHEAD.
     * The largest payload this link carries. The only place the number is
     * written: Proto derives MAX_FRAME from it, and the static_assert below
     * makes the transport's TX slots agree rather than hoping they do. */
    static constexpr std::size_t SERIAL_MAX_PAYLOAD = 128;

    using Proto = SerialProtocol<SERIAL_MAX_PAYLOAD, CdcTransport>;
    using Frame = Proto::Frame;

  private:

    /* CdcTransport sizes its TX ring slots itself, so nothing in the type system
     * forces them to fit a full protocol frame - a payload cap raised here would
     * otherwise truncate silently, one layer down, on the wire. */
    static_assert(Proto::MAX_FRAME <= CdcTransport::MAX_FRAME,
                  "the TX ring slot cannot hold a full protocol frame: raise "
                  "CdcTransport::MAX_FRAME to match SERIAL_MAX_PAYLOAD + overhead");

    void dispatch(const Frame& f);

    /** Link re-establishments seen. Diagnostic only - the MCU has nothing to
     *  redo on link-up, since every worker's state is its own. */
    uint32_t _linkResets = 0;

    CdcTransport _io;
    Proto _proto{_io};

    /* Frames that reached the dispatcher intact and were still refused. Fixed
     * addresses in the statically allocated thread object, so the debugger can
     * watch them live - same rationale as the counters in MassType. Nothing
     * puts them on the wire yet.
     *   _badPayload: known id, wrong payload size -> wire contract drift.
     *   _unknownId : id this firmware has no case for -> the far side is newer. */
    uint16_t _badPayload = 0;
    uint16_t _unknownId  = 0;

  public:
    uint16_t badPayload() const { return _badPayload; }
    uint16_t unknownId()  const { return _unknownId; }
};

