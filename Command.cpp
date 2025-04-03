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
    _commandHandlers["LIST"] = handleList;
    _commandHandlers["NAMES"] = handleNames;
}

void Command::execute(Server* server, Client* client, const std::string& command, const std::string& params) {
    std::map<std::string, CommandHandler>::iterator it = _commandHandlers.find(command);
    if (it != _commandHandlers.end()) {
        std::vector<std::string> paramList = splitParams(params);
        it->second(server, client, paramList);
    } else if (!command.compare("WHO") && !command.compare("CAP")) {
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
}

void Command::handleKick(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.size() < 2) {
        client->sendMessage("461 " + client->getNickname() + " KICK :Not enough parameters");
        return;
    }
    
    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string kickReason = (params.size() > 2) ? params[2] : client->getNickname();
    
    // Check if channel exists
    Channel* channel = server->getChannelByName(channelName);
    if (channel == NULL) {
        client->sendMessage("403 " + client->getNickname() + " " + channelName + " :No such channel");
        return;
    }
    
    // Check if client is in the channel
    if (!channel->isClientInChannel(client)) {
        client->sendMessage("442 " + client->getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    
    // Check if client is a channel operator
    if (!channel->isOperator(client)) {
        client->sendMessage("482 " + client->getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    
    // Find the target client
    Client* targetClient = server->getClientByNickname(targetNick);
    if (targetClient == NULL) {
        client->sendMessage("401 " + client->getNickname() + " " + targetNick + " :No such nick/channel");
        return;
    }
    
    // Check if target is in the channel
    if (!channel->isClientInChannel(targetClient)) {
        client->sendMessage("441 " + client->getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    // Check if target is the Client
    if (!client->getNickname().compare(targetNick)) {
        client->sendMessage("499 " + client->getNickname() + " " + channelName + " :You can't kick yourself");
        return;
    }
    
    // Broadcast the kick message
    std::string kickMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                          " KICK " + channelName + " " + targetNick + " :" + kickReason;
    channel->broadcastMessage(kickMsg, NULL);
    
    // Remove the client from the channel
    channel->removeClient(targetClient);
}

void Command::handleInvite(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.size() < 2) {
        client->sendMessage("461 " + client->getNickname() + " INVITE :Not enough parameters");
        return;
    }
    
    std::string targetNick = params[0];
    std::string channelName = params[1];
    
    // Check if channel exists
    Channel* channel = server->getChannelByName(channelName);
    if (channel == NULL) {
        client->sendMessage("403 " + client->getNickname() + " " + channelName + " :No such channel");
        return;
    }
    
    // Check if client is in the channel
    if (!channel->isClientInChannel(client)) {
        client->sendMessage("442 " + client->getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    
    // Check if client is a channel operator (only required for invite-only channels)
    if (channel->isInviteOnly() && !channel->isOperator(client)) {
        client->sendMessage("482 " + client->getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    
    // Find the target client
    Client* targetClient = server->getClientByNickname(targetNick);
    if (targetClient == NULL) {
        client->sendMessage("401 " + client->getNickname() + " " + targetNick + " :No such nick/channel");
        return;
    }
    
    // Check if target is already in the channel
    if (channel->isClientInChannel(targetClient)) {
        client->sendMessage("443 " + client->getNickname() + " " + targetNick + " " + channelName + " :is already on channel");
        return;
    }
    
    // Add target to the invite list
    channel->inviteClient(targetClient);
    
    // Send invite notification to target
    std::string inviteMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                           " INVITE " + targetNick + " :" + channelName;
    targetClient->sendMessage(inviteMsg);
    
    // Send confirmation to inviter
    client->sendMessage("341 " + client->getNickname() + " " + targetNick + " " + channelName);
}

void Command::handleTopic(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.empty()) {
        client->sendMessage("461 " + client->getNickname() + " TOPIC :Not enough parameters");
        return;
    }
    
    std::string channelName = params[0];
    
    // Check if channel exists
    Channel* channel = server->getChannelByName(channelName);
    if (channel == NULL) {
        client->sendMessage("403 " + client->getNickname() + " " + channelName + " :No such channel");
        return;
    }
    
    // Check if client is in the channel
    if (!channel->isClientInChannel(client)) {
        client->sendMessage("442 " + client->getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    
    // If no topic parameter, return the current topic
    if (params.size() == 1) {
        if (channel->getTopic().empty()) {
            client->sendMessage("331 " + client->getNickname() + " " + channelName + " :No topic is set");
        } else {
            client->sendMessage("332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic());
        }
        return;
    }
    
    // Check if client is allowed to change the topic
    if (channel->isTopicRestricted() && !channel->isOperator(client)) {
        client->sendMessage("482 " + client->getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    
    // Set new topic
    std::string newTopic = params[1];
    channel->setTopic(newTopic);
    
    // Broadcast topic change
    std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                         " TOPIC " + channelName + " :" + newTopic;
    channel->broadcastMessage(topicMsg, NULL);
}

void Command::handleMode(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.empty()) {
        client->sendMessage("461 " + client->getNickname() + " MODE :Not enough parameters");
        return;
    }
    
    std::string target = params[0];
    
    // Handle channel mode
    if (target[0] == '#' || target[0] == '&') {
        // Check if channel exists
        Channel* channel = server->getChannelByName(target);
        if (channel == NULL) {
            client->sendMessage("403 " + client->getNickname() + " " + target + " :No such channel");
            return;
        }
        
        // If no mode parameter, return current channel modes
        if (params.size() == 1) {
            std::string modeString = "+";
            if (channel->isInviteOnly()) modeString += "i";
            if (channel->isTopicRestricted()) modeString += "t";
            if (channel->hasPassword()) modeString += "k";
            if (channel->hasUserLimit()) modeString += "l";
            
            client->sendMessage("324 " + client->getNickname() + " " + target + " " + modeString);
            return;
        }
        
        // Check if client is a channel operator
        if (!channel->isOperator(client)) {
            client->sendMessage("482 " + client->getNickname() + " " + target + " :You're not channel operator");
            return;
        }
        
        // Parse mode string (e.g. +nt-i)
        std::string modeString = params[1];
        bool adding = true;  // Default mode is adding (+)
        size_t paramIndex = 2;  // Index for mode parameters
        
        for (size_t i = 0; i < modeString.length(); ++i) {
            char c = modeString[i];
            
            if (c == '+') {
                adding = true;
            } else if (c == '-') {
                adding = false;
            } else if (c == 'i') {
                // Invite-only flag
                channel->setInviteOnly(adding);
                
                // Broadcast mode change
                std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                    " MODE " + target + " " + (adding ? "+" : "-") + "i";
                channel->broadcastMessage(modeMsg, NULL);
            } else if (c == 't') {
                // Topic restriction flag
                channel->setTopicRestricted(adding);
                
                // Broadcast mode change
                std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                    " MODE " + target + " " + (adding ? "+" : "-") + "t";
                channel->broadcastMessage(modeMsg, NULL);
            } else if (c == 'k') {
                // Channel key (password)
                if (adding) {
                    if (paramIndex < params.size()) {
                        channel->setPassword(params[paramIndex]);
                        
                        // Broadcast mode change
                        std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                            " MODE " + target + " +k " + params[paramIndex];
                        channel->broadcastMessage(modeMsg, NULL);
                        
                        paramIndex++;
                    } else {
                        client->sendMessage("461 " + client->getNickname() + " MODE +k :Not enough parameters");
                    }
                } else {
                    channel->removePassword();
                    
                    // Broadcast mode change
                    std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                        " MODE " + target + " -k";
                    channel->broadcastMessage(modeMsg, NULL);
                }
            } else if (c == 'l') {
                // User limit
                if (adding) {
                    if (paramIndex < params.size()) {
                        size_t limit = 0;
                        std::istringstream iss(params[paramIndex]);
                        iss >> limit;
                        
                        if (limit > 0) {
                            channel->setUserLimit(limit);
                            
                            // Broadcast mode change
                            std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                                " MODE " + target + " +l " + params[paramIndex];
                            channel->broadcastMessage(modeMsg, NULL);
                        }
                        
                        paramIndex++;
                    } else {
                        client->sendMessage("461 " + client->getNickname() + " MODE +l :Not enough parameters");
                    }
                } else {
                    channel->removeUserLimit();
                    
                    // Broadcast mode change
                    std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                        " MODE " + target + " -l";
                    channel->broadcastMessage(modeMsg, NULL);
                }
            } else if (c == 'o') {
                // Channel operator
                if (paramIndex < params.size()) {
                    std::string targetNick = params[paramIndex];
                    Client* targetClient = server->getClientByNickname(targetNick);
                    
                    if ((targetClient != NULL && channel->isClientInChannel(targetClient)) &&(
                            (adding && !channel->isOperator(targetClient)) || 
                            (!adding && channel->isOperator(targetClient)))) {
                        if (adding) {
                            channel->addOperator(targetClient);
                        } else {
                            channel->removeOperator(targetClient);
                        }
                        
                        // Broadcast mode change
                        std::string modeMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                           " MODE " + target + " " + (adding ? "+" : "-") + "o " + targetNick;
                        channel->broadcastMessage(modeMsg, NULL);
                    } else {
                        if ((targetClient != NULL && channel->isClientInChannel(targetClient)) && adding) {
                            client->sendMessage("441 " + client->getNickname() + " " + targetNick + " " + target + " :is already an operator");
                        } else if ((targetClient != NULL && channel->isClientInChannel(targetClient)) && !adding) {
                            client->sendMessage("441 " + client->getNickname() + " " + targetNick + " " + target + " :is not an operator");
                        } else {
                            client->sendMessage("441 " + client->getNickname() + " " + targetNick + " " + target + " :They aren't on that channel");
                        }
                    }
                    
                    paramIndex++;
                } else {
                    client->sendMessage("461 " + client->getNickname() + " MODE +o :Not enough parameters");
                }
            }
        }
    } else {
        // User modes are not implemented in this basic IRC server
        client->sendMessage("502 " + client->getNickname() + " :User modes are not implemented");
    }
}

void Command::handlePart(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    if (params.empty()) {
        client->sendMessage("461 " + client->getNickname() + " PART :Not enough parameters");
        return;
    }
    
    std::string channelsStr = params[0];
    std::string partMessage = (params.size() > 1) ? params[1] : "Leaving";
    
    // Split channel names on commas
    std::vector<std::string> channelNames;
    size_t start = 0, end = 0;
    while ((end = channelsStr.find(',', start)) != std::string::npos) {
        channelNames.push_back(channelsStr.substr(start, end - start));
        start = end + 1;
    }
    channelNames.push_back(channelsStr.substr(start));
    
    // Process each channel
    for (size_t i = 0; i < channelNames.size(); ++i) {
        std::string channelName = channelNames[i];
        Channel* channel = server->getChannelByName(channelName);
        
        if (channel == NULL) {
            client->sendMessage("403 " + client->getNickname() + " " + channelName + " :No such channel");
            continue;
        }
        
        if (!channel->isClientInChannel(client)) {
            client->sendMessage("442 " + client->getNickname() + " " + channelName + " :You're not on that channel");
            continue;
        }
        
        // Send PART message to all clients in the channel
        std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                           " PART " + channelName + " :" + partMessage;
        channel->broadcastMessage(partMsg, NULL);
        
        // Remove client from channel
        channel->removeClient(client);
        if (!channel->getClients().empty() && channel->getOperators().empty()) {
            Client*  newOperator = *channel->getClients().begin();
            channel->addOperator(newOperator);
            std::string newOpMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + 
                                           " MODE " + newOperator->getNickname() + " +o";
        channel->broadcastMessage(newOpMsg , NULL);
        }
        
        // If channel is empty, remove it
        if (channel->getClients().empty()) {
            server->removeChannel(channelName);
        }
    }
}



void Command::handleList(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    // Send list header
    client->sendMessage("321 " + client->getNickname() + " Channel :Users  Name");
    
    // Get the list of channels
    const std::map<std::string, Channel*>& channels = server->getChannels();
    
    // If a specific channel is requested
    if (!params.empty() && params[0] != "*") {
        std::string channelName = params[0];
        std::map<std::string, Channel*>::const_iterator it = channels.find(channelName);
        
        if (it != channels.end()) {
            Channel* channel = it->second;
            std::ostringstream oss;
            oss << channel->getClients().size();
            client->sendMessage("322 " + client->getNickname() + " " + channel->getName() + " " + 
                              oss.str() + " :" + channel->getTopic());
        }
    } else {
        // List all channels
        for (std::map<std::string, Channel*>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
            Channel* channel = it->second;
            // Only show public channels or channels the client is in
            if (!channel->isInviteOnly() || channel->isClientInChannel(client)) {
                std::ostringstream oss;
                oss << channel->getClients().size();
                client->sendMessage("322 " + client->getNickname() + " " + channel->getName() + " " + 
                                  oss.str() + " :" + channel->getTopic());
            }
        }
    }
    
    // Send end of list
    client->sendMessage("323 " + client->getNickname() + " :End of LIST");
}

void Command::handleNames(Server* server, Client* client, const std::vector<std::string>& params) {
    if (!client->isRegistered()) {
        client->sendMessage("451 :You have not registered");
        return;
    }
    
    // If a specific channel is requested
    if (!params.empty()) {
        std::string channelName = params[0];
        Channel* channel = server->getChannelByName(channelName);
        
        if (channel != NULL) {
            // Send names list for the channel
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
        } else {
            client->sendMessage("403 " + client->getNickname() + " " + channelName + " :No such channel");
        }
    } else {
        // List names for all channels
        const std::map<std::string, Channel*>& channels = server->getChannels();
        
        for (std::map<std::string, Channel*>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
            Channel* channel = it->second;
            
            // Only show public channels or channels the client is in
            if (!channel->isInviteOnly() || channel->isClientInChannel(client)) {
                std::string namesList = "353 " + client->getNickname() + " = " + channel->getName() + " :";
                const std::set<Client*>& clients = channel->getClients();
                
                for (std::set<Client*>::const_iterator cit = clients.begin(); cit != clients.end(); ++cit) {
                    if (channel->isOperator(*cit)) {
                        namesList += "@";
                    }
                    namesList += (*cit)->getNickname() + " ";
                }
                
                client->sendMessage(namesList);
                client->sendMessage("366 " + client->getNickname() + " " + channel->getName() + " :End of /NAMES list");
            }
        }
    }
}
