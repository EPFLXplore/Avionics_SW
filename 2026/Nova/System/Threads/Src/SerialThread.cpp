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
/* Size-checked reinterpret of a frame payload as a wire struct.
 *
 * Returns false and leaves `out` untouched when the payload length does not
 * match the struct. It used to return a ZERO-FILLED struct in that case and the
 * caller dispatched it anyway, which is worse than dropping the frame: a
 * ServoRequest of the wrong length became {id 0, angle 0, go_to_zero 0} - a
 * valid command that MOVES servo 0 to 0 degrees. A length that does not match
 * means the wire contract drifted or the frame is noise; neither is a reason to
 * drive hardware. The RPi has always guarded this (Nexus::lengthOk); this is the
 * missing half on the MCU. */
template <class T, class Frame>
bool _frameas(const Frame& f, T& out) {
    if (f.length != sizeof(T)) return false;
    std::memcpy(&out, f.payload.data(), sizeof(T));
    return true;
}
} // namespace

SerialThread::SerialThread(const char* name, osPriority priority) : Thread(name, priority) {
    // Tick rate is NOT set here: every thread's is in System::init(), so the
    // whole schedule reads in one place.
}

void SerialThread::init() {
    _io.begin(); //init transport
}

void SerialThread::loop() {
    /* Release a TX that never completed (USB suspend / host driver reset, where
     * CDC_Init never runs to re-arm us). Runs first so a recovered link can send
     * this iteration's telemetry rather than waiting for the next. */
    _io.serviceTx(xTaskGetTickCount());

    /* RX: serial -> worker command queues */
    uint8_t chunk[CdcTransport::USB_PACKET];
    uint16_t count = _io.read(chunk, sizeof chunk);
    if (count) _proto.parse(chunk, count, [this](const Frame& frame) { dispatch(frame); });
    else   _proto.idle(); // quiet line: abandon any half-received frame (see idle())

    /* TX: worker status queues -> serial */
    MassPacket mp;
    while (System::mass().popStatus(mp)) _proto.send(MassPacket_ID, &mp, sizeof mp);

    PhPacket pp;
    while (System::ph().popStatus(pp)) _proto.send(PhPacket_ID, &pp, sizeof pp);

    Heartbeat hb;
    while (System::heartbeat().popStatus(hb)) _proto.send(Heartbeat_ID, &hb, sizeof hb);
}

void SerialThread::dispatch(const Frame& f) {
    /* A frame whose payload is the wrong size for its id is REFUSED, not
     * dispatched zero-filled. Counted so a contract drift between the two sides
     * shows up as a number instead of as hardware doing something odd. */
    switch (f.id) {
        case ServoRequest_ID: {
            ServoRequest r;
            if (_frameas(f, r)) System::servo().pushCommand(r); else ++_badPayload;
            break;
        }
        case MassRequest_ID: {
            MassRequest r;
            if (_frameas(f, r)) System::mass().pushCommand(r); else ++_badPayload;
            break;
        }
        case LEDRequest_ID: {
            LEDRequest r;
            if (_frameas(f, r)) System::leds().pushCommand(r); else ++_badPayload;
            break;
        }
        case PhRequest_ID: {
            PhRequest r;
            if (_frameas(f, r)) System::ph().pushCommand(r); else ++_badPayload;
            break;
        }
        default: ++_unknownId; break;
    }
}
