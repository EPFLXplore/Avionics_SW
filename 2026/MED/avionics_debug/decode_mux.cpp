// #include <fcntl.h>
// #include <termios.h>
// #include <unistd.h>

// #include <cstdint>
// #include <cstring>
// #include <iomanip>
// #include <iostream>
// #include <string>
// #include <vector>

// // ─────────────── open & configure serial port ────────────────
// int openSerial(const std::string& dev, int baud)
// {
//     int fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
//     if (fd < 0) { perror(dev.c_str()); return -1; }

//     termios tty{};
//     if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); ::close(fd); return -1; }
//     cfmakeraw(&tty);

//     speed_t spd = B115200;
//     switch (baud) {
//         case 9600:   spd = B9600;   break;
//         case 19200:  spd = B19200;  break;
//         case 38400:  spd = B38400;  break;
//         case 57600:  spd = B57600;  break;
// #ifdef B230400
//         case 230400: spd = B230400; break;
// #endif
//         default:     spd = B115200;
//     }
//     cfsetispeed(&tty, spd);
//     cfsetospeed(&tty, spd);
//     tty.c_cflag |= (CLOCAL | CREAD);
//     if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); ::close(fd); return -1; }

//     return fd;
// }

// // ─────────────── helper for nice hex output ──────────────────
// inline void hexByte(uint8_t b)
// {
//     std::cout << std::hex << std::uppercase << std::setw(2)
//               << std::setfill('0') << unsigned(b) << std::dec;
// }

// int main(int argc, char* argv[])
// {
//     if (argc < 3) {
//         std::cerr << "Usage: " << argv[0] << " <serial-port> <baud>\n";
//         return 1;
//     }
//     const std::string port = argv[1];
//     const int         baud = std::stoi(argv[2]);

//     int fd = openSerial(port, baud);
//     if (fd < 0) return 1;

//     enum State { WAIT_STX1, WAIT_STX2, LEN_LO, LEN_HI, FRAME } state = WAIT_STX1;
//     uint16_t len = 0;
//     std::vector<uint8_t> frame;   // ID + payload + CRC

//     while (true) {
//         uint8_t byte;
//         if (::read(fd, &byte, 1) != 1) continue;

//         switch (state) {
//             case WAIT_STX1:
//                 if (byte == 0xA5) state = WAIT_STX2;
//                 break;
//             case WAIT_STX2:
//                 if (byte == 0x5A) state = LEN_LO;
//                 else if (byte != 0xA5) state = WAIT_STX1;   // restart sync
//                 break;
//             case LEN_LO:
//                 len = byte;
//                 state = LEN_HI;
//                 break;
//             case LEN_HI:
//                 len |= static_cast<uint16_t>(byte) << 8;
//                 if (len > 1024 || len == 0) {      // sanity limit
//                     state = WAIT_STX1;
//                     break;
//                 }
//                 frame.clear();
//                 frame.reserve(len + 2);            // ID+payload+CRC
//                 state = FRAME;
//                 break;
//             case FRAME:
//                 frame.push_back(byte);
//                 if (frame.size() == len + 2) {     // got everything
//                     // -------- print -------------
//                     std::cout << "ID=0x";  hexByte(frame[0]);
//                     std::cout << " len=" << (len - 1) << " payload=";
//                     for (size_t i = 1; i < frame.size(); ++i) {
//                         hexByte(frame[i]);
//                         if (i + 1 != frame.size()) std::cout << ' ';
//                     }
//                     std::cout << '\n';
//                     state = WAIT_STX1;
//                 }
//                 break;
//         }
//     }
// }


/* hybrid_dumper.cpp  --------------------------------------------------------
 * 1. Waits for 0xA5 0x5A, reads LEN, ID, payload, CRC
 * 2. If ID matches one of your known packet IDs it copies the payload into
 *    the matching struct and prints the fields by name.
 * 3. Otherwise it just hex-dumps the bytes.
 *
 * Build  (Linux/macOS):
 *     g++ -std=c++17 -I. hybrid_dumper.cpp -o hybrid_dumper
 *     sudo ./hybrid_dumper /dev/ttyUSB0 115200
 *
 * Build  (Windows, MSVC):
 *     cl /EHsc /std:c++17 hybrid_dumper.cpp
 *     hybrid_dumper COM3 115200
 * -------------------------------------------------------------------------*/
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

// ─────── your packet IDs & structs ───────
#include "packet_id.hpp"          // e.g. MassDrill_ID, DustData_ID, …
#include "packet_definition.hpp"  // e.g. MassPacket, DustData, …

// ─────── open & configure serial port ─────
int openSerial(const std::string& dev, int baud)
{
    int fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { perror(dev.c_str()); return -1; }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); ::close(fd); return -1; }
    cfmakeraw(&tty);

    speed_t spd = (baud == 115200) ? B115200 :
                  (baud == 57600)  ? B57600  :
                  (baud == 38400)  ? B38400  :
                  (baud == 19200)  ? B19200  : B9600;
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);
    tty.c_cflag |= (CLOCAL | CREAD);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); ::close(fd); return -1; }

    return fd;
}

// ─────── min-hex helper ───────
inline void hx(uint8_t b)
{
    std::cout << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << unsigned(b) << std::dec;
}

// ─────── memcpy payload → struct safely ───────
template<typename T>
bool as(const std::vector<uint8_t>& pl, T& out)
{
    if (pl.size() != sizeof(T)) return false;
    std::memcpy(&out, pl.data(), sizeof(T));
    return true;
}

// ─────── pretty-printers for your structs ───────
void show(const MassPacket& m)
{
    std::cout << "MassPacket { id=" << unsigned(m.id)
              << ", mass=" << m.mass
              << ", tare=" << m.tare << " }\n";
}
void show(const DustData& d)
{
    std::cout << "DustData { pm1_0_std=" << d.pm1_0_std
              << ", pm2_5_std=" << d.pm2_5_std
              << ", pm10_std="  << d.pm10_std
              << " … }\n";   // shorten for terminal
}
void show(const ServoResponse& s)
{
    std::cout << "ServoResponse { id=" << unsigned(s.id)
              << ", angle=" << s.angle
              << ", success=" << s.success << " }\n";
}
// add more show() overloads here as you define new packets

// ───────────────────────── main ──────────────────────────────
int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <serial-port> <baud>\n";
        return 1;
    }
    int fd = openSerial(argv[1], std::stoi(argv[2]));
    if (fd < 0) return 1;

    enum State { STX1, STX2, LEN_LO, LEN_HI, FRAME } st = STX1;
    uint16_t len = 0;
    std::vector<uint8_t> frame;   // ID + payload + CRC

    while (true) {
        uint8_t byte;
        if (::read(fd, &byte, 1) != 1) continue;

        switch (st) {
            case STX1:  if (byte == 0xA5) st = STX2; break;
            case STX2:
                if (byte == 0x5A) st = LEN_LO;
                else if (byte != 0xA5) st = STX1;
                break;
            case LEN_LO: len = byte; st = LEN_HI; break;
            case LEN_HI:
                len |= static_cast<uint16_t>(byte) << 8;
                if (len == 0 || len > 1024) { st = STX1; break; }  // sanity
                frame.clear(); frame.reserve(len + 2); st = FRAME;
                break;
            case FRAME:
                frame.push_back(byte);
                if (frame.size() == len + 2) {         // full frame (ID+payload+CRC)
                    uint8_t id = frame[0];
                    std::vector<uint8_t> payload(frame.begin()+1, frame.begin()+len);
                    // -------- timestamp ----------
                    double ts = std::chrono::duration<double>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    std::cout << std::fixed << std::setprecision(3)
                              << ts << "  id=0x"; hx(id); std::cout << ' ';
                    // -------- decode or dump -----
                    bool printed = false;
                    switch (id) {
                        case MassDrill_ID:
                        case MassHD_ID: {
                            MassPacket mp; if (as(payload, mp)) { show(mp); printed = true; }
                            break;
                        }
                        case DustData_ID: {
                            DustData dd; if (as(payload, dd)) { show(dd); printed = true; }
                            break;
                        }
                        case ServoResponse_ID: {
                            ServoResponse sr; if (as(payload, sr)) { show(sr); printed = true; }
                            break;
                        }
                        // add more cases here …
                    }
                    if (!printed) {
                        std::cout << "len=" << payload.size() << " payload=";
                        for (auto b : payload) { hx(b); std::cout << ' '; }
                        // CRC (two bytes) — still print them for debugging
                        std::cout << "crc="; hx(frame[len]); std::cout << ' '; hx(frame[len+1]);
                        std::cout << '\n';
                    }
                    st = STX1;
                }
                break;
        }
    }
}
