#include "serverIRC.hpp"

serverIRC::serverIRC() {
	setupServerSocket(194);
	_password = "";
};
serverIRC::serverIRC(int port, std::string pass) {
	setupServerSocket(port);
	_password = pass;
};
serverIRC::~serverIRC() {
	::close(_serverFd);
	for (std::map<const int, sockaddr_in>::iterator it = _clients.begin(); it != _clients.end(); it++) {
		::close(it->first);
	};
};

void serverIRC::setupServerSocket(int port) {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1) 
        throw std::runtime_error("Error: failing making server socket");
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(port);

    if (bind(_serverFd, (struct sockaddr*)&_serverAddr, sizeof(_serverAddr)) == -1)
        throw std::runtime_error("Error: bind failure");

    if (listen(_serverFd, 10) == -1)
		throw std::runtime_error("Error: listen failure");

    std::cout << "IRCserver listening port: " << port << std::endl;

    struct pollfd server_poll_fd;
    server_poll_fd.fd = _serverFd;
    server_poll_fd.events = POLLIN;
    _pollFds.push_back(server_poll_fd);
};

// Acepta nuevos clientes y los añade a la lista de `poll_fds`
void serverIRC::acceptNewClient() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(_serverFd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd == -1)
		throw std::runtime_error("Error: accept client failure");

    std::cout << "Nuevo cliente conectado: " << client_fd << std::endl;
    _clients[client_fd] = client_addr;
    struct pollfd client_poll_fd;
    client_poll_fd.fd = client_fd;
    client_poll_fd.events = POLLIN;
    _pollFds.push_back(client_poll_fd);
};

// Maneja los mensajes de un cliente
void serverIRC::handleClientMessages(int client_fd) {
    char buffer[1024];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cout << "Cliente " << client_fd << " desconectado." << std::endl;
        ::close(client_fd);
        _clients.erase(client_fd);
        for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ) {
			if (it->fd == client_fd) {
				it = _pollFds.erase(it); // erase devuelve el siguiente iterador válido
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
        int poll_count = poll(_pollFds.data(), _pollFds.size(), -1);
        if (poll_count == -1) {
            perror("Error en poll");
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < _pollFds.size(); i++) {
            if (_pollFds[i].revents & POLLIN) {
                if (_pollFds[i].fd == _serverFd) {
                    acceptNewClient();
                } else {
                    handleClientMessages(_pollFds[i].fd);
                }
            }
        }
    }
}