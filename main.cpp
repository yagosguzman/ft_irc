#include <iostream>
#include <cstdlib>
#include <csignal>
#include "Server.hpp"
#include "Command.hpp"

// Global variable for signal handling
Server* g_server = NULL;



// Signal handler function
void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nShutting down IRC server..." << std::endl;
        delete g_server;
        exit(0);
    }
}

// Function to validate port number
bool isValidPort(const std::string& port) {
    for (size_t i = 0; i < port.length(); i++) {
        if (!isdigit(port[i]))
            return false;
    }
    int portNum = std::atoi(port.c_str());
    return (portNum > 0 && portNum < 65536);
}

int main(int argc, char* argv[]) {
    // Check command-line arguments
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }
    
    // Validate port number
    if (!isValidPort(argv[1])) {
        std::cout << "Error: Invalid port number" << std::endl;
        return 1;
    }
    
    int port = std::atoi(argv[1]);
    std::string password = argv[2];
    
    if (password.empty()) {
        std::cout << "Error: Password can't be an empty string" << std::endl;
        return 1;
    }

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize command handlers
        Command::initialize();
        
        // Create and run server
        g_server = new Server(port, password);
        g_server->run();
        
        // Cleanup
        delete g_server;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        delete g_server;
        return 1;
    }
    
    return 0;
}
