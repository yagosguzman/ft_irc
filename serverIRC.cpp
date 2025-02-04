#include "serverIRC.hpp"

serverIRC::serverIRC() {
	setupServerSocket(194);
	password = "";
};
serverIRC::serverIRC(int port, std::string pass) {
	setupServerSocket(port);
	password = pass;
};
serverIRC::~serverIRC() {
	::close(serverFd);
	for (std::map<const int, sockaddr_in>::iterator it = clients.begin(); it != clients.end(); it++) {
		::close(it->first);
	};
};

const std::string& serverIRC::getServerName() const {
    return _serverName;
};

void serverIRC::setupServerSocket(int port) {
    _serverName = "barcelona";
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) 
        throw std::runtime_error("Error: failing making server socket");
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
        throw std::runtime_error("Error: bind failure");

    if (listen(serverFd, 10) == -1)
		throw std::runtime_error("Error: listen failure");

    std::cout << "IRCserver listening port: " << port << std::endl;

    struct pollfd server_poll_fd;
    server_poll_fd.fd = serverFd;
    server_poll_fd.events = POLLIN;
    pollFds.push_back(server_poll_fd);
};

// Acepta nuevos clientes y los añade a la lista de `poll_fds`
void serverIRC::acceptNewClient() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(serverFd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd == -1)
		throw std::runtime_error("Error: accept client failure");

    std::cout << "Nuevo cliente conectado: " << client_fd << std::endl;
    clients[client_fd] = client_addr;
    struct pollfd client_poll_fd;
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN;
    pollFds.push_back(client_poll_fd);
};

// Maneja los mensajes de un cliente
void serverIRC::handleClientMessages(int client_fd) {
    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cout << "Cliente " << client_fd << " desconectado." << std::endl;
        ::close(client_fd);
        clients.erase(client_fd);
        for (std::vector<struct pollfd>::iterator it = pollFds.begin(); it != pollFds.end(); ) {
			if (it->fd == client_fd) {
				it = pollFds.erase(it); // erase devuelve el siguiente iterador válido
			} else {
				++it; // Avanzamos solo si no eliminamos
			}
		}
        return;
    }
    buffer[bytes_received] = '\0';
    std::cout << "Mensaje de " << client_fd << ": " << buffer << std::endl;
}

// Bucle principal del servidor que usa `poll()`
void serverIRC::run() {
    while (true) {
        int poll_count = poll(pollFds.data(), pollFds.size(), -1);
        if (poll_count == -1)
            throw std::runtime_error("Error en poll");

        for (size_t i = 0; i < pollFds.size(); i++) {
            if (pollFds[i].revents & POLLIN) {
                if (pollFds[i].fd == serverFd) {
                    acceptNewClient();
                } else {
                    handleClientChannelMessages(pollFds[i].fd);
                }
            }
        }
    }
}

void serverIRC::joinChannel(int client_fd, const std::string &channel_name, const std::string &nickname) {
    if (channels.find(channel_name) == channels.end()) {
        channels[channel_name] = Channel(channel_name);
    }

    channels[channel_name].addClient(client_fd, nickname);
    std::string welcome_msg = ":" + channel_name + " PRIVMSG " + nickname + " :" + "Bienvenido a " + channel_name + "\r\n";
    send(client_fd, welcome_msg.c_str(), welcome_msg.size(), 0);
}

void serverIRC::sendMessageToChannel(int client_fd, const std::string &channel_name, const std::string &message) {
    if (channels.find(channel_name) == channels.end()) {
        std::string error_msg = "El canal " + channel_name + " no existe.\n";
        send(client_fd, error_msg.c_str(), error_msg.size(), 0);
        return;
    }
    channels[channel_name].broadcastMessage(client_fd, message, getServerName());
}


void serverIRC::handleClientChannelMessages(int client_fd) {
    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cout << "Cliente " << client_fd << " desconectado." << std::endl;
        ::close(client_fd);
        clients.erase(client_fd);
        return;
    }
    buffer[bytes_received] = '\0';
    std::string input(buffer);

    typedef std::istringstream IssType;
    IssType iss(input);
    std::string command;
    iss >> command;
    std::cout << command << std::endl;
    if (command == "JOIN") {
        std::string channel_name, nickname;
        iss >> channel_name >> nickname;
        joinChannel(client_fd, channel_name, nickname);
    } else if (command == "PRIVMSG") {
        std::string channel_name;
        iss >> channel_name;
        std::string message;
        getline(iss, message);
        sendMessageToChannel(client_fd, channel_name, message);
    } else {
        std::string error_msg = "Comando no reconocido.\n";
        send(client_fd, error_msg.c_str(), error_msg.size(), 0);
    }
}
