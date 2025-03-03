#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <set>
#include "Server.hpp"
#include "Channel.hpp"

class Server;
class Channel;

class Client {
private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _hostname;
    std::string _realname;
    std::string _buffer;
    bool _authenticated;
    bool _registered;
    std::set<Channel*> _channels;
    Server* _server;

public:
    Client(int fd, Server* server);
    ~Client();

    // Getters
    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    const std::string& getRealname() const;
    bool isAuthenticated() const;
    bool isRegistered() const;
    const std::set<Channel*>& getChannels() const;

    // Setters
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setHostname(const std::string& hostname);
    void setRealname(const std::string& realname);
    void setAuthenticated(bool authenticated);
    void setRegistered(bool registered);

    // Buffer handling
    void appendToBuffer(const std::string& data);
    std::vector<std::string> getCompletedCommands();
    void clearBuffer();

    // Channel operations
    void joinChannel(Channel* channel);
    void leaveChannel(Channel* channel);
    bool isInChannel(Channel* channel) const;
    bool isOperatorInChannel(Channel* channel) const;

    // Message sending
    void sendMessage(const std::string& message) const;
};

#endif /* CLIENT_HPP */
