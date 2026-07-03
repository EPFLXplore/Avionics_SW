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
T frame_as(const SerialProtocol<128, CdcTransport>::Frame& f) {
    T out{};
    if (f.length == sizeof(T)) std::memcpy(&out, f.payload.data(), sizeof(T));
    return out;
}
} // namespace

SerialThread::SerialThread(const char* name, osPriority priority) : Thread(name, priority) {
    setDelay(1); // poll the RX ring + drain status queues every 1 ms
}

void SerialThread::init() {
    io_.begin(); // register as the CDC singleton
}

void SerialThread::loop() {
    /* RX: serial -> worker command queues */
    uint8_t chunk[64];
    uint16_t n = io_.read(chunk, sizeof chunk);
    if (n) proto_.parse(chunk, n, [this](const Frame& f) { dispatch(f); });

    /* TX: worker status queues -> serial */
    MassPacket mp;
    while (System::mass().popStatus(mp)) proto_.send(MassPacket_ID, &mp, sizeof mp);

    Heartbeat hb;
    while (System::heartbeat().popStatus(hb)) proto_.send(Heartbeat_ID, &hb, sizeof hb);
}

void SerialThread::dispatch(const Frame& f) {
    switch (f.id) {
        case ServoRequest_ID: System::servo().pushCommand(frame_as<ServoRequest>(f)); break;
        case MassRequest_ID:  System::mass().pushCommand(frame_as<MassRequest>(f));   break;
        case LEDRequest_ID:   System::leds().pushCommand(frame_as<LEDRequest>(f));    break;
        default: break;
    }
}
