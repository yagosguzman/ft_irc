#include "channel.hpp"

Channel::Channel() {}; 

Channel::Channel(std::string channel_name) : name(channel_name) {}

std::map<int, std::string> Channel::getClients() const {
	return nicknames;
};

void Channel::addClient(int client_fd, const std::string &nickname, const std::string& username) {
	nicknames[client_fd] = nickname;
	users[client_fd] = username;
}

void Channel::removeClient(int client_fd) {
	nicknames.erase(client_fd);
	users.erase(client_fd);
}

void Channel::broadcastMessage(int sender_fd, const std::string &message, const std::string &serverName) {
	if(users.find(sender_fd) == users.end())
	{
		std::string errorMessage = "You dont belong to this server. Use JOIN to join the server.";
		send(sender_fd, errorMessage.c_str(), errorMessage.size(), 0);
	}
	else {
		for (std::map<int, std::string>::iterator it = users.begin(); it != users.end(); ++it) {
			if (it->first != sender_fd) {
				std::string fullMessage = ":" + nicknames[sender_fd]  + " PRIVMSG " +"#" + name + " :" + message + "\r\n";
				send(it->first, fullMessage.c_str(), fullMessage.size(), 0);
			}
		}
	}
}

bool Channel::isEmpty() const {
	return users.empty();
}