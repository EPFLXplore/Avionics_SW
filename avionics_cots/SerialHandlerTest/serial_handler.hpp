#ifndef SERIAL_HANDLER_HPP
#define SERIAL_HANDLER_HPP

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

enum PacketId {
    MassData_ID = 0x01,
    MassConfigRequest_ID = 0x02,
    MassCalib_ID = 0x03,
    MassConfig_ID = 0x04,
    MassConfigResponse_ID = 0x05,
    Servo_ID = 0x06,
    ServoResponse_ID = 0x07,
    ServoConfigRequest_ID = 0x08,
    ServoConfig_ID = 0x09,
    ServoConfigResponse_ID = 0x0A,
    LED_ID = 0x0B,
    LEDResponse_ID = 0x0C,
    FourInOne_ID = 0x0D,
    NPK_ID = 0x0E,
    DustData_ID = 0x0F
};

// Forward declarations of packet structs
struct DustData;
struct MassConfigRequestPacket;
struct MassConfigResponsePacket;
// Add other packet structs as needed

class SerialHandler {
public:
    SerialHandler(const std::string& port = "/dev/ttyUSB0", int baudRate = 115200);
    ~SerialHandler();

    // Open/close connection
    bool open();
    void close();
    bool isOpen() const;

    // Start/stop the listener thread
    void startListening();
    void stopListening();

    // Send packet methods
    bool sendDustDataPacket(const DustData& packet);
    bool sendMassConfigRequestPacket(const MassConfigRequestPacket& packet);
    // Add other send methods as needed

    // Callback registration for packet reception
    using PacketCallback = std::function<void(const uint8_t*, size_t)>;
    void registerCallback(PacketId id, PacketCallback callback);

private:
    // Serial port details
    std::string m_portName;
    int m_baudRate;
    int m_serialFd;

    // Listener thread
    std::atomic<bool> m_running;
    std::thread m_listenerThread;
    void listenerThreadFunc();

    // Packet handling
    std::mutex m_callbackMutex;
    std::vector<std::pair<PacketId, PacketCallback>> m_callbacks;

    // Helper methods
    bool writePacket(PacketId id, const void* data, size_t size);
    void parseIncomingData(const uint8_t* buffer, size_t length);
};

#endif // SERIAL_HANDLER_HPP