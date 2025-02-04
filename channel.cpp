#include "channel.hpp"

Channel::Channel() {}; 

Channel::Channel(std::string channel_name) : name(channel_name) {}

void Channel::addClient(int client_fd, const std::string &nickname) {
	clients[client_fd] = nickname;
}

void Channel::removeClient(int client_fd) {
	clients.erase(client_fd);
}

void Channel::broadcastMessage(int sender_fd, const std::string &message) {
	for (std::map<int, std::string>::iterator it = clients.begin(); it != clients.end(); ++it) {
		if (it->first != sender_fd) {
			int bytes;
			std::string fullMessage = ":" + clients[sender_fd] + " PRIVMSG " + name + " :" + message + "\r\n";
			bytes = send(it->first, fullMessage.c_str(), fullMessage.size(), 0);
			std::cout << "Bytes sent: " << bytes << std::endl;
		}
	}
}

bool Channel::isEmpty() const {
	return clients.empty();
}