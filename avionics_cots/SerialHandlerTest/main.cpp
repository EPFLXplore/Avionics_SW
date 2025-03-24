#include "SerialHandler.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>

// Example packet handler functions
void handleDustData(const uint8_t* data, size_t length)
{
    if (length >= sizeof(DustData)) {
        const DustData* packet = reinterpret_cast<const DustData*>(data);
        std::cout << "Received DustData packet:" << std::endl;
        std::cout << "  PM1.0: " << packet->pm1_0 << std::endl;
        std::cout << "  PM2.5: " << packet->pm2_5 << std::endl;
        std::cout << "  PM10: " << packet->pm10 << std::endl;
    } else {
        std::cout << "Received DustData packet with incorrect size" << std::endl;
    }
}

void handleMassConfigResponse(const uint8_t* data, size_t length)
{
    // Cast data to corresponding packet structure and process
    std::cout << "Received MassConfigResponse packet (" << length << " bytes)" << std::endl;
    // Process according to your MassConfigResponsePacket structure
}

void printHexDump(const uint8_t* data, size_t length)
{
    std::cout << "Raw data (hex): ";
    for (size_t i = 0; i < length; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

int main(int argc, char* argv[])
{
    // Default settings
    std::string port = "/dev/ttyUSB0";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = argv[++i];
        }
    }
    
    // Create and initialize serial handler
    SerialHandler serialHandler(port);
    
    if (!serialHandler.open()) {
        std::cerr << "Failed to open serial port. Exiting." << std::endl;
        return 1;
    }
    
    // Register packet handlers
    serialHandler.registerCallback(DustData_ID, handleDustData);
    serialHandler.registerCallback(MassConfigResponse_ID, handleMassConfigResponse);
    
    // Register a generic handler that prints raw data for all packets
    for (int id = 0x01; id <= 0x0F; ++id) {
        serialHandler.registerCallback(static_cast<PacketId>(id), printHexDump);
    }
    
    // Start the listener thread
    serialHandler.startListening();
    
    std::cout << "Serial communication test started. Available commands:" << std::endl;
    std::cout << "  dust - Send a test DustData packet" << std::endl;
    std::cout << "  config - Send a MassConfigRequest packet" << std::endl;
    std::cout << "  exit - Exit the program" << std::endl;
    
    // Main command loop
    std::string command;
    bool running = true;
    
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "exit") {
            running = false;
        }
        else if (command == "dust") {
            DustData packet = {1.23f, 4.56f, 7.89f};  // Test values
            if (serialHandler.sendDustDataPacket(packet)) {
                std::cout << "Sent DustData test packet" << std::endl;
            }
        }
        else if (command == "config") {
            MassConfigRequestPacket packet = {1};  // Request for sensor ID 1
            if (serialHandler.sendMassConfigRequestPacket(packet)) {
                std::cout << "Sent MassConfigRequest packet" << std::endl;
            }
        }
        else if (!command.empty()) {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
    
    // Cleanup
    serialHandler.stopListening();
    serialHandler.close();
    
    std::cout << "Exiting." << std::endl;
    return 0;
}