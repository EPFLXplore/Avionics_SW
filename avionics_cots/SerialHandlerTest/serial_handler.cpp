#include "SerialHandler.hpp"

#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <algorithm>

// Include packet definitions that match your ESP32 code
// These should mirror packet_definition.hpp from your Arduino code
struct DustData {
    float pm1_0;
    float pm2_5;
    float pm10;
};

struct MassConfigRequestPacket {
    uint32_t sensor_id;
};

// Add other packet struct definitions as needed

SerialHandler::SerialHandler(const std::string& port, int baudRate)
    : m_portName(port), m_baudRate(baudRate), m_serialFd(-1), m_running(false)
{
}

SerialHandler::~SerialHandler()
{
    stopListening();
    close();
}

bool SerialHandler::open()
{
    // Open serial port
    m_serialFd = ::open(m_portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (m_serialFd < 0) {
        std::cerr << "Error opening serial port: " << m_portName << std::endl;
        return false;
    }

    // Configure serial port settings
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(m_serialFd, &tty) != 0) {
        std::cerr << "Error from tcgetattr" << std::endl;
        ::close(m_serialFd);
        m_serialFd = -1;
        return false;
    }

    // Set baud rate
    speed_t baud;
    switch (m_baudRate) {
        case 9600:   baud = B9600;   break;
        case 19200:  baud = B19200;  break;
        case 38400:  baud = B38400;  break;
        case 57600:  baud = B57600;  break;
        case 115200: baud = B115200; break;
        default:
            std::cerr << "Unsupported baud rate: " << m_baudRate << std::endl;
            ::close(m_serialFd);
            m_serialFd = -1;
            return false;
    }
    
    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    // 8N1 mode (8 bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;  // No parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8 bits
    tty.c_cflag &= ~CRTSCTS; // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines
    
    // Raw input
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Raw output
    tty.c_oflag &= ~OPOST;
    
    // No software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    
    // No special handling of bytes
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    // Set timeout for read
    tty.c_cc[VTIME] = 10;  // Wait up to 1 second (10 tenths)
    tty.c_cc[VMIN] = 0;    // Return immediately if data available, or wait for timeout
    
    // Apply settings
    if (tcsetattr(m_serialFd, TCSANOW, &tty) != 0) {
        std::cerr << "Error from tcsetattr" << std::endl;
        ::close(m_serialFd);
        m_serialFd = -1;
        return false;
    }
    
    std::cout << "Successfully opened serial port: " << m_portName << std::endl;
    return true;
}

void SerialHandler::close()
{
    if (m_serialFd >= 0) {
        ::close(m_serialFd);
        m_serialFd = -1;
        std::cout << "Serial port closed" << std::endl;
    }
}

bool SerialHandler::isOpen() const
{
    return m_serialFd >= 0;
}

void SerialHandler::startListening()
{
    if (!isOpen() || m_running) {
        return;
    }
    
    m_running = true;
    m_listenerThread = std::thread(&SerialHandler::listenerThreadFunc, this);
    std::cout << "Started listening on serial port" << std::endl;
}

void SerialHandler::stopListening()
{
    if (m_running) {
        m_running = false;
        if (m_listenerThread.joinable()) {
            m_listenerThread.join();
        }
        std::cout << "Stopped listening on serial port" << std::endl;
    }
}

void SerialHandler::listenerThreadFunc()
{
    constexpr size_t BUFFER_SIZE = 256;
    uint8_t buffer[BUFFER_SIZE];
    
    while (m_running) {
        // Read data from serial port
        ssize_t bytesRead = read(m_serialFd, buffer, BUFFER_SIZE);
        
        if (bytesRead > 0) {
            std::cout << "Received " << bytesRead << " bytes" << std::endl;
            
            // Process the received data
            parseIncomingData(buffer, bytesRead);
        }
        else if (bytesRead < 0 && errno != EAGAIN) {
            std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
            // Maybe add some error handling or retry logic
        }
        
        // Sleep a bit to avoid consuming too much CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SerialHandler::parseIncomingData(const uint8_t* buffer, size_t length)
{
    // Ensure we have at least a packet ID
    if (length < 1) {
        return;
    }
    
    // Extract the packet ID
    PacketId packetId = static_cast<PacketId>(buffer[0]);
    std::cout << "Received packet with ID: 0x" << std::hex << static_cast<int>(packetId) << std::dec << std::endl;
    
    // Notify registered callbacks
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    for (const auto& callback : m_callbacks) {
        if (callback.first == packetId) {
            // Pass the data (excluding the ID) to the callback
            callback.second(buffer + 1, length - 1);
        }
    }
}

bool SerialHandler::writePacket(PacketId id, const void* data, size_t size)
{
    if (!isOpen()) {
        std::cerr << "Cannot write to closed serial port" << std::endl;
        return false;
    }
    
    // Create buffer with ID + data
    std::vector<uint8_t> buffer(size + 1);
    buffer[0] = static_cast<uint8_t>(id);
    memcpy(buffer.data() + 1, data, size);
    
    // Write to serial port
    ssize_t bytesWritten = write(m_serialFd, buffer.data(), buffer.size());
    
    if (bytesWritten < 0) {
        std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (static_cast<size_t>(bytesWritten) != buffer.size()) {
        std::cerr << "Warning: Only wrote " << bytesWritten << " of " << buffer.size() << " bytes" << std::endl;
        return false;
    }
    
    std::cout << "Successfully sent packet with ID 0x" << std::hex << static_cast<int>(id) 
              << std::dec << " (" << size << " bytes of data)" << std::endl;
    return true;
}

bool SerialHandler::sendDustDataPacket(const DustData& packet)
{
    return writePacket(DustData_ID, &packet, sizeof(DustData));
}

bool SerialHandler::sendMassConfigRequestPacket(const MassConfigRequestPacket& packet)
{
    return writePacket(MassConfigRequest_ID, &packet, sizeof(MassConfigRequestPacket));
}

void SerialHandler::registerCallback(PacketId id, PacketCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks.emplace_back(id, callback);
}