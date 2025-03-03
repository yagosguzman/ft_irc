#include "Client.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

Client::Client(int fd, Server* server)
    : _fd(fd), _nickname(""), _username(""), _hostname(""), _realname(""), _buffer(""),
      _authenticated(false), _registered(false), _server(server) {
}

Client::~Client() {
    // No need to close _fd here, it's already closed in Server::removeClient
}

int Client::getFd() const {
    return _fd;
}

const std::string& Client::getNickname() const {
    return _nickname;
}

const std::string& Client::getUsername() const {
    return _username;
}

const std::string& Client::getHostname() const {
    return _hostname;
}

const std::string& Client::getRealname() const {
    return _realname;
}

bool Client::isAuthenticated() const {
    return _authenticated;
}

bool Client::isRegistered() const {
    return _registered;
}

const std::set<Channel*>& Client::getChannels() const {
    return _channels;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}

void Client::setHostname(const std::string& hostname) {
    _hostname = hostname;
}

void Client::setRealname(const std::string& realname) {
    _realname = realname;
}

void Client::setAuthenticated(bool authenticated) {
    _authenticated = authenticated;
}

void Client::setRegistered(bool registered) {
    _registered = registered;
}

void Client::appendToBuffer(const std::string& data) {
    _buffer += data;
}

std::vector<std::string> Client::getCompletedCommands() {
    std::vector<std::string> commands;
    
    // Look for complete commands (terminated by \r\n)
    size_t pos;
    while ((pos = _buffer.find("\r\n")) != std::string::npos) {
        // Extract the command
        std::string command = _buffer.substr(0, pos);
        commands.push_back(command);
        
        // Remove the command and \r\n from the buffer
        _buffer.erase(0, pos + 2);
    }
    
    return commands;
}

void Client::clearBuffer() {
    _buffer.clear();
}

void Client::joinChannel(Channel* channel) {
    _channels.insert(channel);
}

void Client::leaveChannel(Channel* channel) {
    _channels.erase(channel);
}

bool Client::isInChannel(Channel* channel) const {
    return _channels.find(channel) != _channels.end();
}

bool Client::isOperatorInChannel(Channel* channel) const {
    return channel->isOperator(this);
}

void Client::sendMessage(const std::string& message) const {
    if (_fd != -1) {
        std::string fullMessage = message;
        if (fullMessage.find("\r\n") == std::string::npos) {
            fullMessage += "\r\n";
        }
        
        ssize_t bytesSent = send(_fd, fullMessage.c_str(), fullMessage.length(), 0);
        if (bytesSent == -1) {
            std::cerr << "Error sending message to client (fd: " << _fd << "): " << strerror(errno) << std::endl;
        }
    }
}
