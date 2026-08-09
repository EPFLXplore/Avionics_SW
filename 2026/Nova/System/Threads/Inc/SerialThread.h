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

  private:
    using Proto = SerialProtocol<128, CdcTransport>;
    using Frame = Proto::Frame;

    void dispatch(const Frame& f);

    CdcTransport _io;
    Proto _proto{_io};
};

