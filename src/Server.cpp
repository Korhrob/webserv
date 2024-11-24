
#include "Server.hpp"
#include <string>

#include <poll.h>
#include <fcntl.h>

Server::Server(const std::string& ip, int port) 
{
	// should read config file and initialize all server variables here
	// read more about config files for NGINX for reference

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

	for (int i = 1; i < m_max_sockets; i++)
	{
		m_sockets[i].fd = -1;
		m_sockets[i].events = POLLIN & POLLOUT;
	}

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
	usleep(10000); // wait a little bit for close() before exit
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
	// read a bit about max request size for HTTP/1.1
	char	buffer[1024];
	memset(buffer, 0, 1024);

	// see recv instead of read, also use size_t
	size_t	bytes_read = recv(m_sockets[id].fd, buffer, 1024, 0);

	std::cout << "-- BYTES READ " << bytes_read << "--\n\n" << std::endl;

	if (bytes_read > 0)
	{
		*request = std::string(buffer);

		// request method, like GET and its target, if no target default to index.html

		// connection type, so we know if client no longer needs to keep connection alive
		// Connection: close
		
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
	std::string body;

	// temporary response
	if (request.find("GET / ") != std::string::npos)
	{
		// read the contents of GET request for body
		body = "<html>initial response</html>";

		// standard success first line
		response = "HTTP/1.1 200 OK\r\n";

		// content type could be based on file extensions
		response += "Content-Type: text/html\r\n";

		// always get the size of body, if size is too large consider 'chunking' the data
		// important to get content-length right, as client will hang and wait indefinitely
		response += "Content-Length: " + std::to_string(body.size()) + "\r\n";

		// allow persistent connection, usually handled by client
		//response += "Connection: keep-alive\r\n";  

		// empty line
		response += "\r\n";
		
		// body
		response += body;
	} else {
		body = "some other type of file";

		response = "HTTP/1.1 200 OK\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		response += "Connection: keep-alive\r\n";
		response += "\r\n";
		response += body;
	}

	m_files_sent[id]++;
	int bytes_sent = send(m_sockets[id].fd, response.c_str(), response.size(), 0);
	std::cout << "-- BYTES SENT " << bytes_sent << "--\n\n" << std::endl;
}

void	Server::update()
{
	// should read about poll() and non blocking i/o

	int	r = poll(m_sockets, m_sock_count, 5000);

	if (r == -1)
		return;

	auto now = std::chrono::steady_clock::now();

	// check for events in listenning socket (server socket)
	if (m_sockets[0].revents & POLLIN)
	{
		// accept new connection
		t_sockaddr_in client_addr = {};
		int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &m_addr_len);

		if (client_fd >= 0)
		{
			// find empty socket to bind
			int i = 1;
			for (i = 1; i < m_max_sockets; i++)
			{
				if (m_sockets[i].fd <= 0)
					break;
			}
			// NOTE: should also check if we even have space for new clients
			// if not, we could also decline the connection

			// set up non blocking fd
			int flags = fcntl(client_fd, F_GETFL, 0);
    		fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

			// reinitialize client information
			m_files_sent[i] = 0;
			m_sockets[i].fd = client_fd;
			m_sockets[i].events = POLLIN;
			m_last_activity[i] = now;
			m_sock_count++;

			std::cout << "New connection in slot " << i << std::endl;
		}
		else
		{
			// accept failed
		}
		m_sockets[0].revents = 0;
	}

	// listen for all poll events
	for (int i = 1; i < m_max_sockets; i++)
	{
		// only for sockets with a connection (open fd)
		if (m_sockets[i].fd != -1)
		{
			if (m_sockets[i].revents & POLLIN) // incoming
			{
				handleClient(i);
				m_last_activity[i] = now;
			}
			else if (m_sockets[i].revents & POLLOUT) // outgoing
			{
				// there might be specific cases where you want to do something after sending data
				std::cout << "-- handle pollout --" << std::endl;
				m_sockets[i].revents = 0;
				m_last_activity[i] = now;
			} 
			else // no events, check for timeout so we can free up sockets for new connections
			{
				std::cout << "No events for client " << i << std::endl;
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_activity[i]).count() > 5000)
				{
					std::cout << "Client " << i << " timeout\n\n" << std::endl;
					close(m_sockets[i].fd);
					m_sockets[i].fd = -1;
					m_sock_count--;
				}
			}
		}
	}

}