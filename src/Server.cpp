
#include "Server.hpp"
#include <string>

#include <poll.h>
#include <fcntl.h>

Server::Server(const std::string& ip, int port) 
{
	// should read config file and initialize all server variables here
	// read more about config files for NGINX

	m_address = ip;
	m_port = port;
	m_sock_count = 1;
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

	m_sockets[0].fd = m_socket;
	m_sockets[0].events = POLLIN;
	m_sockets[0].revents = 0;

	return 1;
}

void	Server::closeServer()
{
	log("Closing server...");
	if (m_socket >= 0)
		close(m_socket);
	if (m_new_socket >= 0)
		close(m_new_socket);
	for (int i = 0; i < m_sock_count; i++)
	{
		if (m_sockets[i].fd >= 0)
			close(m_sockets[i].fd);
	}
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

bool	Server::parseRequest(int id, std::string *request)
{
	char	buffer[1024];
	memset(buffer, 0, 1024);

	// see recv instead of read, also use size_t
	size_t	bytes_read = recv(m_sockets[id].fd, buffer, 1024, 0);

	if (bytes_read > 0)
	{
		*request = std::string(buffer);
		std::cout << buffer << std::endl;
		return true;
	}

	return false;
}

void	Server::handleClient(int id)
{

	std::string	request;
	std::string response;

	if (!parseRequest(id, &request))
	{
		logError("Client disconnected or error occured");
		close(m_sockets[id].fd);
		m_sockets[id] = m_sockets[--m_sock_count];
		return;
	}

	m_sockets[id].revents = POLLOUT;

	// temporary response
	if (request.find("GET / ") != std::string::npos)
	{
		response = "HTTP/1.1 200 OK\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Connection: keep-alive\r\n";  // Allow persistent connection
		response += "\r\n";
		response += "<html>initial response</html>";
	} else {
		response = "HTTP/1.1 200 OK\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Connection: keep-alive\r\n";
		response += "\r\n";
		response += "some other type of file";
	}

	m_files_sent[id]++;
	int bytes_sent = send(m_sockets[id].fd, response.c_str(), response.size(), 0);
	std::cout << "\n\n-- BYTES SENT " << bytes_sent << "--\n\n" << std::endl;


	// this implementation of server doesnt work without closing the socket after each send
	close(m_sockets[id].fd);
	m_sockets[m_sock_count--].fd = -1;
}

void	Server::update()
{
	// should read about poll() and non blocking i/o

	int	r = poll(m_sockets, m_sock_count, 1000);

	if (r == -1)
		return;

	if (m_sockets[0].revents & POLLIN)
	{
		std::cout << "-- server new POLLIN --" << std::endl;

		t_sockaddr_in client_addr = {};
		int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &m_addr_len);
		//int new_client = accept(m_socket, NULL, NULL);

		if (client_fd >= 0)
		{
			// find empty socket to bind

			// set non blocking fd
			int flags = fcntl(client_fd, F_GETFL, 0);
    		fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

			m_files_sent[m_sock_count] = 0;
			m_sockets[m_sock_count].fd = client_fd;
			m_sockets[m_sock_count].events = POLLIN;
			m_sock_count++;
		}
		else
		{
			// failed or no space
		}
		m_sockets[0].revents = 0;
	}

	for (int i = 1; i < m_sock_count; i++)
	{
		//std::cout << "-- check fd " << i << std::endl;
		if (m_sockets[i].fd != -1)
		{
			if (m_sockets[i].revents & POLLIN)
			{
				handleClient(i);
			}
			else if (m_sockets[i].revents & POLLOUT)
			{
				std::cout << "handle pollout" << std::endl;
				m_sockets[i].revents = 0;
			}
		}
	}

}