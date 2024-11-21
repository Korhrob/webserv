
#include "Server.hpp"
#include <string>

Server::Server(const std::string& ip, int port) 
{
	// should read config file and initialize all server variables
	m_address = ip;
	m_port = port;
}

Server::~Server()
{
	// always close all open sockets on exit
	if (m_socket >= 0)
		close(m_socket);
	if (m_new_socket >= 0)
		close(m_new_socket);
}

int	Server::startServer()
{
	log("Starting Server on " + m_address + ":" + std::to_string(m_port) + "...");
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket <= 0)
	{
		logError("Server failed to open socket!");
		return 0;
	}

	m_socket_addr.sin_family = AF_INET;
	m_socket_addr.sin_addr.s_addr = INADDR_ANY;
	m_socket_addr.sin_port = htons(m_port);

	if (bind(m_socket, (struct sockaddr*)&m_socket_addr, sizeof(m_socket_addr)) < 0)
	{
		logError("Bind failed");
		closeServer();
		return 0;
	}

	log("Server started on " + m_address + ":" + std::to_string(m_port));
	startListen();
	return 1;
}

void	Server::closeServer()
{
	log("Closing server...");
	close(m_socket);
	close(m_new_socket);
	log("Server closed!");
	exit(0);
}

void	Server::startListen()
{
	// max number of client 5 should come from config

	if (listen(m_socket, 5) < 0)
	{
		logError("Listen failed");
		closeServer();
	}

	log("Server is listenning...");
}

void	Server::handleClient(int client_socket)
{
	char	buffer[1024];
	int		bytes_read = read(client_socket, buffer, 1024);

	// - parsing would happen here -
	// handle http header
	// build response -> send

	if (bytes_read > 8)
	{
		char *ptr = buffer;

		int	id = 0;
		int	len = 0;
		std::string	msg;

		memcpy(&id, ptr, 4);
		ptr += 4;

		memcpy(&len, ptr, 4);
		ptr += 4;

		// check if bytes read totals to header_size + len

		id = ntohl(id);
		len = ntohl(len);

		std::cout << "Msg received: " 
					<< "\n\tID: " << id 
					<< "\n\tLen: " << len
					<< "\n\tMsg: " << std::string(ptr)
					<< std::endl;

		send(client_socket, buffer, bytes_read, 0);
		log("Msg sent back to client\n");
	}
	else
	{
		logError("Received Invalid message");
	}

	close(client_socket);
}

void	Server::update()
{
	// should check poll() and non blocking i/o

	m_new_socket = accept(m_socket, (struct sockaddr*)&m_client_addr, &m_client_addr_len);
	if (m_new_socket < 0)
	{
		logError("Accept failed");
		return ;
	}

	log("Client connected!");
	handleClient(m_new_socket);

}