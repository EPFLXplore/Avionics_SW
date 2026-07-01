/**
 * @file SerialThread.h
 * @brief The MCU wire-owner: mirror of the RPi's Nexus.
 *
 * One thread owns the USB link. RX: drain the byte ring through the framing FSM
 * and push each command into the target worker's queue (MessageThread
 * pushCommand). TX: drain the workers' status queues (popStatus) onto the wire.
 * It is the only caller of proto_.send()/parse(), so the wire has one owner.
 */

#ifndef THREADS_SERIALTHREAD_H_
#define THREADS_SERIALTHREAD_H_

#include "Thread.h"
#include "SerialProtocol.hpp"
#include "Transport.hpp"

class SerialThread : public Thread {
  public:
    SerialThread(const char* name, osPriority priority);

    void init() override;
    void loop() override;

  private:
    using Proto = SerialProtocol<128, CdcTransport>;
    using Frame = Proto::Frame;

    void dispatch(const Frame& f);

    CdcTransport io_;
    Proto proto_{io_};
};

#endif /* THREADS_SERIALTHREAD_H_ */
