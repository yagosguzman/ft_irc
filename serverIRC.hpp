#ifndef SERVERIRC_HPP
# define SERVERIRC_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

class	serverIRC {

	private:
		int	serverFd;
		struct sockaddr_in serverAddr;
		std::vector<struct pollfd> pollFds;
		std::map<const int, sockaddr_in> clients;
		int port;
		std::string password;

		void setupServerSocket(int port);
		void acceptNewClient();
		void handleClientMessages(int client_fd);

	public:
		serverIRC();
		serverIRC(int port, std::string pass);
		~serverIRC();

		void run();
		const int& getPort() const;
		bool checkPass(std::string inputPass);
		void start();
		void close();
		void addClient();
		void kick(std::string nClient);
		void invite(std::string nClient, int channel);
		void topic(); // Change or view the channel topic
		void mode(char mode); // Change the channel’s mode:
			//· i: Set/remove Invite-only channel
			//· t: Set/remove the restrictions of the TOPIC command to channel operators
			//· k: Set/remove the channel key (password)
			//· o: Give/take channel operator privilege
			//· l: Set/remove the user limit to channel



};

#endif