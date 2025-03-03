#include "Channel.hpp"
#include <algorithm>
#include <sstream>

Channel::Channel(const std::string& name, Client* creator)
    : _name(name), _topic(""), _inviteOnly(false), _topicRestricted(true),
      _hasPassword(false), _password(""), _hasUserLimit(false), _userLimit(0) {
    // Add creator as the first client and operator
    _clients.insert(creator);
    _operators.insert(creator);
    
    // Add channel to client's channels
    creator->joinChannel(this);
}

Channel::~Channel() {
    // Remove this channel from all clients
    for (std::set<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        (*it)->leaveChannel(this);
    }
}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

const std::set<Client*>& Channel::getClients() const {
    return _clients;
}

const std::set<Client*>& Channel::getOperators() const {
    return _operators;
}

bool Channel::isInviteOnly() const {
    return _inviteOnly;
}

bool Channel::isTopicRestricted() const {
    return _topicRestricted;
}

bool Channel::hasPassword() const {
    return _hasPassword;
}

const std::string& Channel::getPassword() const {
    return _password;
}

bool Channel::hasUserLimit() const {
    return _hasUserLimit;
}

size_t Channel::getUserLimit() const {
    return _userLimit;
}

void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}

void Channel::setInviteOnly(bool inviteOnly) {
    _inviteOnly = inviteOnly;
}

void Channel::setTopicRestricted(bool restricted) {
    _topicRestricted = restricted;
}

void Channel::setPassword(const std::string& password) {
    _hasPassword = true;
    _password = password;
}

void Channel::removePassword() {
    _hasPassword = false;
    _password = "";
}

void Channel::setUserLimit(size_t limit) {
    _hasUserLimit = true;
    _userLimit = limit;
}

void Channel::removeUserLimit() {
    _hasUserLimit = false;
    _userLimit = 0;
}

bool Channel::addClient(Client* client, const std::string& password) {
    // Check if client is already in the channel
    if (isClientInChannel(client)) {
        return true;
    }
    
    // Check channel restrictions
    if (_inviteOnly && !isClientInvited(client)) {
        return false; // Invite-only channel and client is not invited
    }
    
    if (_hasPassword && password != _password) {
        return false; // Wrong password
    }
    
    if (_hasUserLimit && _clients.size() >= _userLimit) {
        return false; // Channel is full
    }
    
    // Add client to channel
    _clients.insert(client);
    client->joinChannel(this);
    
    // Remove from invited list if present
    removeInvite(client);
    
    return true;
}

void Channel::removeClient(Client* client) {
    if (isClientInChannel(client)) {
        _clients.erase(client);
        client->leaveChannel(this);
        
        // Remove from operators list if present
        if (isOperator(client)) {
            _operators.erase(client);
        }
    }
}

bool Channel::isClientInChannel(const Client* client) const {
    return _clients.find(const_cast<Client*>(client)) != _clients.end();
}

bool Channel::isOperator(const Client* client) const {
    return _operators.find(const_cast<Client*>(client)) != _operators.end();
}

void Channel::addOperator(Client* client) {
    if (isClientInChannel(client)) {
        _operators.insert(client);
    }
}

void Channel::removeOperator(Client* client) {
    _operators.erase(client);
}

void Channel::inviteClient(Client* client) {
    _invitedClients.insert(client);
}

bool Channel::isClientInvited(const Client* client) const {
    return _invitedClients.find(const_cast<Client*>(client)) != _invitedClients.end();
}

void Channel::removeInvite(Client* client) {
    _invitedClients.erase(client);
}

void Channel::broadcastMessage(const std::string& message, Client* exclude) const {
    for (std::set<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        Client* client = *it;
        if (client != exclude) {
            client->sendMessage(message);
        }
    }
}
