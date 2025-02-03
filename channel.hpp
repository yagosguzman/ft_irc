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
    std::map<int, std::string> clients; // Mapa de fd -> nickname
	

public:
	Channel();
    Channel(std::string channel_name);

    void addClient(int client_fd, const std::string &nickname);
    void removeClient(int client_fd);
    void broadcastMessage(int sender_fd, const std::string &message);
    bool isEmpty() const;
};
