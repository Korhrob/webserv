
#include "Server.hpp"
#include <string>

Server::Server(const std::string& ip, int port) 
{
	// should read config file and initialize all server variables here
	// read more about config files for NGINX

	m_address = ip;
	m_port = port;
}

Server::~Server()
{
	// always close all open sockets on exit

	closeServer();
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

    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(m_socket);
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
	if (m_socket >= 0)
		close(m_socket);
	if (m_new_socket >= 0)
		close(m_new_socket);
	usleep(10000);
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

std::string	parseRequest(int client_socket)
{
	char	buffer[1024];
	memset(buffer, 0, 1024);
	int		bytes_read = read(client_socket, buffer, 1024);

	// - parsing would happen here, this only temporary -
	// read http request
	// parse tags
	// save response tags / varables to a class/struct

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

		if (id < 10) // custom ID message, we wont use this for http
		{
			std::cout << "Msg received: " 
						<< "\n\tID: " << id 
						<< "\n\tLen: " << len
						<< "\n\tMsg: " << std::string(ptr)
						<< std::endl;
		} else {
			std::cout << buffer << std::endl;
		}
	}
	else
	{
		logError("Received Invalid message: ");
	}

	// for now, return the received message
	return std::string(buffer);
}

void	Server::handleClient(int client_socket)
{
	// temporary string for response
	std::string	request = parseRequest(client_socket);
	
	// temporary response
	std::string response = request; // buildResponse();

	if (response.size() > 0)
	{
		send(client_socket, response.c_str(), response.size(), 0);
		log("Sent response\n");
	}

	// should only close if requests did not contain keep-alive
	close(client_socket);
}

void	Server::update()
{
	// should read about poll() and non blocking i/o

	m_new_socket = accept(m_socket, (struct sockaddr*)&m_client_addr, &m_client_addr_len);
	if (m_new_socket < 0)
	{
		logError("Accept failed");
		return ;
	}

	log("Accepted client!");
	handleClient(m_new_socket);

}