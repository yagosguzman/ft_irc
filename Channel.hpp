#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>
#include "Client.hpp"

class Client;

class Channel {
private:
    std::string _name;
    std::string _topic;
    std::set<Client*> _clients;
    std::set<Client*> _operators;
    std::set<Client*> _invitedClients;
    
    bool _inviteOnly;
    bool _topicRestricted;
    bool _hasPassword;
    std::string _password;
    bool _hasUserLimit;
    size_t _userLimit;

public:
    Channel(const std::string& name, Client* creator);
    ~Channel();

    // Getters
    const std::string& getName() const;
    const std::string& getTopic() const;
    const std::set<Client*>& getClients() const;
    const std::set<Client*>& getOperators() const;
    bool isInviteOnly() const;
    bool isTopicRestricted() const;
    bool hasPassword() const;
    const std::string& getPassword() const;
    bool hasUserLimit() const;
    size_t getUserLimit() const;

    // Setters
    void setTopic(const std::string& topic);
    void setInviteOnly(bool inviteOnly);
    void setTopicRestricted(bool restricted);
    void setPassword(const std::string& password);
    void removePassword();
    void setUserLimit(size_t limit);
    void removeUserLimit();

    // Client operations
    bool addClient(Client* client, const std::string& password = "");
    void removeClient(Client* client);
    bool isClientInChannel(const Client* client) const;
    bool isOperator(const Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);
    void inviteClient(Client* client);
    bool isClientInvited(const Client* client) const;
    void removeInvite(Client* client);

    // Message broadcasting
    void broadcastMessage(const std::string& message, Client* exclude = NULL) const;
};

#endif /* CHANNEL_HPP */
