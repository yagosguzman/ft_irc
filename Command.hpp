#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <vector>
#include <map>
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

class Server;
class Client;
class Channel;

typedef void (*CommandHandler)(Server* server, Client* client, const std::vector<std::string>& params);

class Command {
private:
    static std::map<std::string, CommandHandler> _commandHandlers;
    
    // Command handlers
    static void handlePass(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleNick(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleUser(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleJoin(Server* server, Client* client, const std::vector<std::string>& params);
    static void handlePrivmsg(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleQuit(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleKick(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleInvite(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleTopic(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleMode(Server* server, Client* client, const std::vector<std::string>& params);
    static void handlePart(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleList(Server* server, Client* client, const std::vector<std::string>& params);
    static void handleNames(Server* server, Client* client, const std::vector<std::string>& params);
    
    // Helper functions
    static std::vector<std::string> splitParams(const std::string& params);

public:
    Command();
    ~Command();
    
    static void initialize();
    static void execute(Server* server, Client* client, const std::string& command, const std::string& params);
};

#endif /* COMMAND_HPP */
