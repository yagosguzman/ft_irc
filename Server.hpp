#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include "Client.hpp"
#include "Channel.hpp"

class Client;
class Channel;

class Server {
private:
    int _port;
    std::string _password;
    int _serverSocket;
    std::vector<pollfd> _pollfds;
    std::map<int, Client*> _clients;
    std::map<std::string, Channel*> _channels;

    // Private methods
    void setupServerSocket();
    void acceptNewConnection();
    void handleClientData(int clientFd);
    void removeClient(int clientFd);
    bool isNicknameInUse(const std::string& nickname) const;

public:
    Server(int port, const std::string& password);
    ~Server();

    void run();
    
    // Client management
    void registerClient(Client* client);
    Client* getClientByNickname(const std::string& nickname) const;
    
    // Channel management
    Channel* createChannel(const std::string& name, Client* creator);
    Channel* getChannelByName(const std::string& name) const;
    void removeChannel(const std::string& name);
    
    // Getters
    const std::string& getPassword() const;
    const std::map<std::string, Channel*>& getChannels() const;
    
    // Command processing
    void processCommand(int clientFd, const std::string& message);
};

#endif /* SERVER_HPP */
