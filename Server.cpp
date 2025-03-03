#include "Server.hpp"
#include "Command.hpp"
#include <sstream>
#include <algorithm>

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverSocket(-1) {
    setupServerSocket();
    std::cout << "IRC Server initialized on port " << _port << std::endl;
}

Server::~Server() {
    // Close all client connections
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    
    // Delete all channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    
    // Close server socket
    if (_serverSocket != -1) {
        close(_serverSocket);
    }
    
    std::cout << "IRC Server shutdown complete" << std::endl;
}

void Server::setupServerSocket() {
    // Create socket
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1) {
        throw std::runtime_error("Failed to create server socket: " + std::string(strerror(errno)));
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(_serverSocket, F_GETFL, 0);
    if (flags == -1 || fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
    }
    
    // Bind socket
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);
    
    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Failed to bind server socket: " + std::string(strerror(errno)));
    }
    
    // Listen for connections
    if (listen(_serverSocket, 10) == -1) {
        close(_serverSocket);
        throw std::runtime_error("Failed to listen on server socket: " + std::string(strerror(errno)));
    }
    
    // Add server socket to pollfds
    pollfd pfd;
    pfd.fd = _serverSocket;
    pfd.events = POLLIN;
    _pollfds.push_back(pfd);
}

void Server::run() {
    std::cout << "IRC Server running. Waiting for connections..." << std::endl;
    
    while (true) {
        // Poll for events
        int pollResult = poll(&_pollfds[0], _pollfds.size(), -1);
        if (pollResult == -1) {
            if (errno == EINTR) continue; // Interrupted by signal, just continue
            throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
        }
        
        // Check for events on each file descriptor
        for (size_t i = 0; i < _pollfds.size(); ++i) {
            if (_pollfds[i].revents & POLLIN) {
                if (_pollfds[i].fd == _serverSocket) {
                    // New connection event on server socket
                    acceptNewConnection();
                } else {
                    // Data available on client socket
                    handleClientData(_pollfds[i].fd);
                }
            } else if (_pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                // Client disconnected or error
                if (_pollfds[i].fd != _serverSocket) {
                    removeClient(_pollfds[i].fd);
                    // Removed a client, so we need to adjust our loop counter
                    i--;
                }
            }
        }
    }
}

void Server::acceptNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientFd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        }
        return;
    }
    
    // Set client socket to non-blocking mode
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(clientFd);
        std::cerr << "Failed to set client socket to non-blocking mode: " << strerror(errno) << std::endl;
        return;
    }
    
    // Add client socket to pollfds
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    _pollfds.push_back(pfd);
    
    // Create and store the client
    Client* client = new Client(clientFd, this);
    _clients[clientFd] = client;
    
    std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) 
              << ":" << ntohs(clientAddr.sin_port) << " (fd: " << clientFd << ")" << std::endl;
    
    // Set client hostname
    client->setHostname(inet_ntoa(clientAddr.sin_addr));
}

void Server::handleClientData(int clientFd) {
    Client* client = _clients[clientFd];
    char buffer[1024];
    
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        if (bytesRead == 0 || errno != EAGAIN) {
            // Connection closed by client or error
            removeClient(clientFd);
        }
        return;
    }
    
    buffer[bytesRead] = '\0';
    client->appendToBuffer(buffer);
    
    // Process complete commands
    std::vector<std::string> commands = client->getCompletedCommands();
    for (size_t i = 0; i < commands.size(); ++i) {
        std::string& command = commands[i];
        if (!command.empty()) {
            processCommand(clientFd, command);
        }
    }
}

void Server::processCommand(int clientFd, const std::string& message) {
    Client* client = _clients[clientFd];
    
    std::string command;
    std::string params;
    
    // Parse command
    size_t spacePos = message.find(' ');
    if (spacePos == std::string::npos) {
        command = message;
        params = "";
    } else {
        command = message.substr(0, spacePos);
        params = message.substr(spacePos + 1);
    }
    
    // Convert command to uppercase
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    
    // Execute command
    Command::execute(this, client, command, params);
}

void Server::removeClient(int clientFd) {
    if (_clients.find(clientFd) != _clients.end()) {
        Client* client = _clients[clientFd];
        std::cout << "Client disconnected (fd: " << clientFd << ", nickname: " 
                  << (client->getNickname().empty() ? "unknown" : client->getNickname()) << ")" << std::endl;
        
        // Remove client from all channels
        std::set<Channel*> clientChannels = client->getChannels();
        for (std::set<Channel*>::iterator it = clientChannels.begin(); it != clientChannels.end(); ++it) {
            Channel* channel = *it;
            channel->removeClient(client);
            
            // Remove empty channels
            if (channel->getClients().empty()) {
                removeChannel(channel->getName());
            }
        }
        
        // Remove client from clients map
        _clients.erase(clientFd);
        
        // Remove client from pollfds
        for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
            if (it->fd == clientFd) {
                _pollfds.erase(it);
                break;
            }
        }
        
        // Close socket and delete client
        close(clientFd);
        delete client;
    }
}

bool Server::isNicknameInUse(const std::string& nickname) const {
    for (std::map<int, Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nickname) {
            return true;
        }
    }
    return false;
}

void Server::registerClient(Client* client) {
    // Check if client has provided all required registration information
    if (!client->isAuthenticated() || client->getNickname().empty() || client->getUsername().empty()) {
        return;
    }
    
    client->setRegistered(true);
    
    // Send welcome messages
    std::string welcomeMsg = "001 " + client->getNickname() + " :Welcome to the IRC server " 
                           + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + "\r\n";
    client->sendMessage(welcomeMsg);
    
    std::string yourHostMsg = "002 " + client->getNickname() + " :Your host is " + client->getHostname() 
                            + ", running version 1.0\r\n";
    client->sendMessage(yourHostMsg);
    
time_t now = time(NULL);
std::string createdMsg = "003 " + client->getNickname() + " :This server was created " 
                         + std::string(ctime(&now));

    client->sendMessage(createdMsg);
    
    std::cout << "Client registered: " << client->getNickname() << "!" << client->getUsername() 
              << "@" << client->getHostname() << std::endl;
}

Client* Server::getClientByNickname(const std::string& nickname) const {
    for (std::map<int, Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nickname) {
            return it->second;
        }
    }
    return NULL;
}

Channel* Server::createChannel(const std::string& name, Client* creator) {
    if (_channels.find(name) != _channels.end()) {
        return _channels[name];
    }
    
    Channel* channel = new Channel(name, creator);
    _channels[name] = channel;
    
    std::cout << "Channel created: " << name << " by " << creator->getNickname() << std::endl;
    
    return channel;
}

Channel* Server::getChannelByName(const std::string& name) const {
    std::map<std::string, Channel*>::const_iterator it = _channels.find(name);
    if (it != _channels.end()) {
        return it->second;
    }
    return NULL;
}

void Server::removeChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end()) {
        std::cout << "Channel removed: " << name << std::endl;
        delete it->second;
        _channels.erase(it);
    }
}

const std::string& Server::getPassword() const {
    return _password;
}

const std::map<std::string, Channel*>& Server::getChannels() const {
    return _channels;
}
