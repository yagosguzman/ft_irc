#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>


class Channel {
private:
    std::string name;
    std::map<int, std::string> nicknames; // Mapa de fd -> nickname
	std::map<int, std::string> users;   // Mapa de fd -> username

public:
	Channel();
    Channel(std::string channel_name);

    void addClient(int client_fd, const std::string &nickname, const std::string &username);
    std::map<int, std::string> getClients() const;
    void removeClient(int client_fd);
    void broadcastMessage(int sender_fd, const std::string &message, const std::string &serverName);
    bool isEmpty() const;
};
