#include "channel.hpp"

Channel::Channel() {}; 

Channel::Channel(std::string channel_name) : name(channel_name) {}

void Channel::addClient(int client_fd, const std::string &nickname) {
	clients[client_fd] = nickname;
}

void Channel::removeClient(int client_fd) {
	clients.erase(client_fd);
}

void Channel::broadcastMessage(int sender_fd, const std::string &message, const std::string &serverName) {
	std::string nickname = "gpinilla";
	std::string channel = "#kaka";

	std::string fullMessage = ":gpinilla!gpinilla@localhost PRIVMSG #tarda :Hola\r\n";

	// std::string sender_nick = clients[sender_fd]; // Obtiene el nickname del remitente
	// std::string fullMessage = ":" + sender_nick + "!user@localhost PRIVMSG " + name + " :" + message + serverName + "\r\n";

	for (std::map<int, std::string>::iterator it = clients.begin(); it != clients.end(); ++it) {
		if (it->first != sender_fd) {
			int bytes;
			//std::string fullMessage = ":" + serverName + " PRIVMSG " + clients[sender_fd] + " :" + message + "\r\n";
			std::cout << "fullmessage: " <<fullMessage << std::endl;
			bytes = send(it->first, fullMessage.c_str(), fullMessage.size(), 0);
			std::cout << "Bytes sent: " << bytes << std::endl;
		}
	}
}	

bool Channel::isEmpty() const {
	return clients.empty();
}