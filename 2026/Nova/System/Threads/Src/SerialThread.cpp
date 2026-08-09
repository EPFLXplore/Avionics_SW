/**
 * @file SerialThread.cpp
 * @brief MCU wire-owner. Plain MessageThread routing: pushCommand on RX,
 *        popStatus on TX. No mailbox, no registry.
 */

#include "SerialThread.h"

#include "System.h"   // System::servo / mass / leds
#include "packets.h"  // ids + wire structs

#include <cstring>

namespace {
/* size-checked reinterpret of a frame payload as a wire struct */
template <class T>
T _frameas(const SerialProtocol<128, CdcTransport>::Frame& f) {
    T out{};
    if (f.length == sizeof(T)) std::memcpy(&out, f.payload.data(), sizeof(T));
    return out;
}
} // namespace

SerialThread::SerialThread(const char* name, osPriority priority) : Thread(name, priority) {
    setDelay(1); // poll the RX ring + drain status queues every 1 ms
}

void SerialThread::init() {
    _io.begin(); // register as the CDC singleton
}

void SerialThread::loop() {
    /* Release a TX that never completed (USB suspend / host driver reset, where
     * CDC_Init never runs to re-arm us). Runs first so a recovered link can send
     * this iteration's telemetry rather than waiting for the next. */
    _io.serviceTx(xTaskGetTickCount());

    /* RX: serial -> worker command queues */
    uint8_t chunk[64];
    uint16_t n = _io.read(chunk, sizeof chunk);
    if (n) _proto.parse(chunk, n, [this](const Frame& f) { dispatch(f); });

    /* TX: worker status queues -> serial */
    MassPacket mp;
    while (System::mass().popStatus(mp)) _proto.send(MassPacket_ID, &mp, sizeof mp);

    PhPacket pp;
    while (System::ph().popStatus(pp)) _proto.send(PhPacket_ID, &pp, sizeof pp);

    Heartbeat hb;
    while (System::heartbeat().popStatus(hb)) _proto.send(Heartbeat_ID, &hb, sizeof hb);
}

void SerialThread::dispatch(const Frame& f) {
    switch (f.id) {
        case ServoRequest_ID: System::servo().pushCommand(_frameas<ServoRequest>(f)); break;
        case MassRequest_ID:  System::mass().pushCommand(_frameas<MassRequest>(f));   break;
        case LEDRequest_ID:   System::leds().pushCommand(_frameas<LEDRequest>(f));    break;
        case PhRequest_ID:    System::ph().pushCommand(_frameas<PhRequest>(f));       break;
        default: break;
    }
}
