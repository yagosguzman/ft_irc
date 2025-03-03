#include "Command.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

std::map<std::string, CommandHandler> Command::_commandHandlers;

Command::Command() {}
Command::~Command() {}

void Command::initialize() {
    _commandHandlers["PASS"] = handlePass;
    _commandHandlers["NICK"] = handleNick;
    _commandHandlers["USER"] = handleUser;
    _commandHandlers["JOIN"] = handleJoin;
    _commandHandlers["PRIVMSG"] = handlePrivmsg;
    _commandHandlers["QUIT"] = handleQuit;
    _commandHandlers["KICK"] = handleKick;
    _commandHandlers["INVITE"] = handleInvite;
    _commandHandlers["TOPIC"] = handleTopic;
    _commandHandlers["MODE"] = handleMode;
    _commandHandlers["PART"] = handlePart;
    _commandHandlers["PING"] = handlePing;
    _commandHandlers["PONG"] = handlePong;
    _commandHandlers["LIST"] = handleList;
    _commandHandlers["NAMES"] = handleNames;
}

void Command::execute(Server* server, Client* client, const std::string& command, const std::string& params) {
    std::map<std::string, CommandHandler>::iterator it = _commandHandlers.find(command);
    if (it != _commandHandlers.end()) {
        std::vector<std::string> paramList = splitParams(params);
        it->second(server, client, paramList);
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
    }
}

std::vector<std::string> Command::splitParams(const std::string& params) {
    std::vector<std::string> result;
    std::istringstream stream(params);
    std::string param;
    
    // Check if the last parameter starts with a colon (special case for IRC protocol)
    size_t colonPos = params.find(" :");
    if (colonPos != std::string::npos) {
        // Split regular parameters before the colon
        std::string regularParams = params.substr(0, colonPos);
        std::istringstream regularStream(regularParams);
        while (regularStream >> param) {
            result.push_back(param);
        }
        
        // Add the trailing parameter (after the colon) as a single parameter
        std::string trailingParam = params.substr(colonPos + 2);
        result.push_back(trailingParam);
    } else {
        // Simple case - just split by whitespace
        while (stream >> param) {
            result.push_back(param);
        }
    }
    
    return result;
}

// Command implementations - Part 1
void Command::handlePass(Server* server, Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        client->sendMessage("461 " + client->getNickname() + " PASS :Not enough parameters");
        return;
    }
    
    if (client->isRegistered()) {
        client->sendMessage("462 " + client->getNickname() + " :You may not reregister");
        return;
    }
    
    if (params[0] == server->getPassword()) {
        client->setAuthenticated(true);
    } else {
        client->sendMessage("464 " + client->getNickname() + " :Password incorrect");
    }
}

void Command::handleNick(Server* server, Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        client->sendMessage("431 :No nickname given");
        return;
    }
    
    std::string nickname = params[0];
    
    // Check nickname format (simple check for invalid characters)
    for (size_t i = 0; i < nickname.length(); i++) {
        char c = nickname[i];
        if (!isalnum(c) && c != '-' && c != '_' && c != '[' && c != ']' && c != '{' && c != '}' && c != '\\' && c != '`' && c != '|' && c != '^') {
            client->sendMessage("432 " + nickname + " :Erroneous nickname");
            return;
        }
    }
    
    // Check if nickname is already in use
    Client* existingClient = server->getClientByNickname(nickname);
    if (existingClient != NULL && existingClient != client) {
        client->sendMessage("433 " + nickname + " :Nickname is already in use");
        return;
    }
    
    // If client already has a nickname (nickname change)
    std::string oldNickname = client->getNickname();
    if (!oldNickname.empty() && client->isRegistered()) {
        // Broadcast the nickname change to all channels the client is in
        std::string nickChangeMsg = ":" + oldNickname + "!" + client->getUsername() + "@" + client->getHostname() + " NICK :" + nickname;
        
        std::set<Channel*> clientChannels = client->getChannels();
        for (std::set<Channel*>::iterator it = clientChannels.begin(); it != clientChannels.end(); ++it) {
            (*it)->broadcastMessage(nickChangeMsg, NULL);
        }
    }
    
    // Update the nickname
    client->setNickname(nickname);
    
    // If this is the first time setting a nickname, try to register the client
    if (oldNickname.empty()) {
        server->registerClient(client);
    }
}

void Command::handleUser(Server* server, Client* client, const std::vector<std::string>& params) {
    if (params.size() < 4) {
        client->sendMessage("461 " + client->getNickname() + " USER :Not enough parameters");
        return;
    }
    
    if (client->isRegistered()) {
        client->sendMessage("462 " + client->getNickname() + " :You may not reregister");
        return;
    }
    
    client->setUsername(params[0]);
    client->setRealname(params[3]);
    
    // Try to register the client now that we have the username
    server->registerClient(client);
}

void Command::handleJoin(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.empty()) {
        client->sendMessage("461 " + client->getNickname() + " JOIN :Not enough parameters");
        return;
    }
    
    // Split channel names on commas (e.g. JOIN #channel1,#channel2)
    std::string channelsStr = params[0];
    std::vector<std::string> channelNames;
    
    size_t start = 0, end = 0;
    while ((end = channelsStr.find(',', start)) != std::string::npos) {
        channelNames.push_back(channelsStr.substr(start, end - start));
        start = end + 1;
    }
    channelNames.push_back(channelsStr.substr(start));
    
    // Split keys on commas if provided
    std::vector<std::string> keys;
    if (params.size() > 1) {
        std::string keysStr = params[1];
        start = 0, end = 0;
        while ((end = keysStr.find(',', start)) != std::string::npos) {
            keys.push_back(keysStr.substr(start, end - start));
            start = end + 1;
        }
        keys.push_back(keysStr.substr(start));
    }
    
    // Join each channel
    for (size_t i = 0; i < channelNames.size(); i++) {
        std::string channelName = channelNames[i];
        
        // Check for valid channel name format
        if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&')) {
            client->sendMessage("403 " + client->getNickname() + " " + channelName + " :No such channel");
            continue;
        }
        
        // Get or create the channel
        Channel* channel = server->getChannelByName(channelName);
        if (channel == NULL) {
            channel = server->createChannel(channelName, client);
        } else {
            // Try to join the existing channel
            std::string password = (i < keys.size()) ? keys[i] : "";
            if (!channel->addClient(client, password)) {
                if (channel->isInviteOnly()) {
                    client->sendMessage("473 " + client->getNickname() + " " + channelName + " :Cannot join channel (+i)");
                } else if (channel->hasPassword()) {
                    client->sendMessage("475 " + client->getNickname() + " " + channelName + " :Cannot join channel (+k)");
                } else if (channel->hasUserLimit()) {
                    client->sendMessage("471 " + client->getNickname() + " " + channelName + " :Cannot join channel (+l)");
                } else {
                    client->sendMessage("471 " + client->getNickname() + " " + channelName + " :Cannot join channel");
                }
                continue;
            }
        }
        
        // Send JOIN notification to all users in the channel
        std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " JOIN :" + channelName;
        channel->broadcastMessage(joinMsg, NULL);
        
        // Send channel topic
        if (!channel->getTopic().empty()) {
            client->sendMessage("332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic());
        }
        
        // Send names list
        std::string namesList = "353 " + client->getNickname() + " = " + channelName + " :";
        const std::set<Client*>& clients = channel->getClients();
        for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
            if (channel->isOperator(*it)) {
                namesList += "@";
            }
            namesList += (*it)->getNickname() + " ";
        }
        client->sendMessage(namesList);
        client->sendMessage("366 " + client->getNickname() + " " + channelName + " :End of /NAMES list");
    }
}

void Command::handlePrivmsg(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.empty()) {
        client->sendMessage("411 " + client->getNickname() + " :No recipient given (PRIVMSG)");
        return;
    }
    
    if (params.size() < 2) {
        client->sendMessage("412 " + client->getNickname() + " :No text to send");
        return;
    }
    
    std::string target = params[0];
    std::string message = params[1];
    
    if (target[0] == '#' || target[0] == '&') {
        // Message to a channel
        Channel* channel = server->getChannelByName(target);
        if (channel == NULL) {
            client->sendMessage("401 " + client->getNickname() + " " + target + " :No such nick/channel");
            return;
        }
        
        // Check if client is in the channel
        if (!channel->isClientInChannel(client)) {
            client->sendMessage("404 " + client->getNickname() + " " + target + " :Cannot send to channel");
            return;
        }
        
        // Send message to all clients in the channel except the sender
        std::string msgToSend = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " PRIVMSG " + target + " :" + message;
        channel->broadcastMessage(msgToSend, client);
    } else {
        // Message to a user
        Client* targetClient = server->getClientByNickname(target);
        if (targetClient == NULL) {
            client->sendMessage("401 " + client->getNickname() + " " + target + " :No such nick/channel");
            return;
        }
        
        // Send private message
        std::string msgToSend = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " PRIVMSG " + target + " :" + message;
        targetClient->sendMessage(msgToSend);
    }
}

void Command::handleQuit(Server* server, Client* client, const std::vector<std::string>& params) {
    (void)server; // Avoid unused parameter warning
    std::string quitMessage = (params.empty()) ? "Client Quit" : params[0];
    
    // Notify all channels this client is in
    std::set<Channel*> clientChannels = client->getChannels();
    for (std::set<Channel*>::iterator it = clientChannels.begin(); it != clientChannels.end(); ++it) {
        Channel* channel = *it;
        std::string quitNotification = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " QUIT :" + quitMessage;
        channel->broadcastMessage(quitNotification, client);
    }
    
    // Server will handle removing the client
}
