
#include "Client.hpp"
#include "Server.hpp"
#include "Parse.hpp"
#include <string>
#include <algorithm> // min
#include <sstream> //

#include <poll.h>
#include <fcntl.h>

Server::Server(const std::string& ip, int port) 
{
	// should read config file and initialize all server variables here
	// read more about config files for NGINX for reference

	m_address = ip;
	m_port = port;
	m_sock_count = 1;

	m_pollfd.resize(m_max_sockets + 1);

	m_pollfd[0] = { m_socket, POLLIN, 0 };
	m_listener = &m_pollfd[0];

	for (int i = 0; i < m_max_sockets; i++)
	{
		auto client = std::make_shared<Client>(&m_pollfd[i + 1]);
		m_clients.push_back(client);
	}

	log("Server constructor done");
	std::cout << "m_pollfd size " << m_pollfd.size() << std::endl;
}

Server::~Server()
{
	closeServer();

	m_pollfd.clear();
	m_clients.clear();
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

	m_listener->fd = m_socket;

	log("Server started on " + m_address + ":" + std::to_string(m_port));
	startListen();

	return 1;
}

void	Server::closeServer()
{
	log("Closing server...");

	if (m_socket >= 0)
		close(m_socket);

	for (auto& pollfd : m_pollfd)
	{
		if (pollfd.fd > 0)
			close(pollfd.fd);
	}

	usleep(10000); // wait a little bit for close() before exit
	log("Server closed!");
}

void	Server::startListen()
{
	if (listen(m_socket, m_max_backlog) < 0)
	{
		logError("Listen failed");
		closeServer();
	}

	log("Server is listenning...");
}

bool	Server::parseRequest(std::shared_ptr<Client> client, std::string *header, std::string *response)
{
	log("parseRequest");

	// read a bit about max request size for HTTP/1.1
	char	buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	size_t	bytes_read = recv(client->fd(), buffer, BUFFER_SIZE, 0);

	std::cout << "-- BYTES READ " << bytes_read << "--\n\n" << std::endl;

	if (bytes_read <= 0)
		return false;

	std::string temp = std::string(buffer);
	std::cout << buffer << std::endl;

	// check request method, like GET and its target, if no target default to index.html
	size_t pos = temp.find('\n');

	if (pos == std::string::npos)
	{
		// no new line found, faulty request
		logError("invalid request header, no nl");
		return false;
	}

	std::string first_line = temp.substr(0, pos);
	std::vector<std::string> info;
	size_t start = 0;
	size_t end = first_line.find(' ');

	while (end != std::string::npos) {
		info.push_back(first_line.substr(start, end - start));
		start = end + 1;
		end = first_line.find(' ', start);
	}

	if (info.size() != 2)
	{
		logError("Invalid request header");
		return false;
	}

	// check method type
	if (info[0] == "GET")
	{
		*response = getBody(info[1]);
		*header = getHeader(response->size());
		return true;
	}

	logError("Method not implemented!!");
	return false;
}

void	Server::handleClient(std::shared_ptr<Client> client)
{
	std::string	header = "";
	std::string body = "";

	if (!parseRequest(client, &header, &body))
	{
		logError("Client disconnected or error occured");
		client->disconnect();
		return;
	}

	std::string	response = header + body;

	if (response.size() <= 0)
	{
		logError("Invalid response");
		return;
	}

	if (body.size() < MAX_RESPONSE_SIZE)
	{
		log("send single");
		client->respond(response);
		//m_sockets[id].revents = POLLOUT;
	}
	else
	{
		log("send chunk");
		client->respond(header);
		//m_sockets[id].revents = POLLOUT;

		size_t start = 0;
		size_t length = BUFFER_SIZE; // CHUNK_SIZE
		while (start < body.size())
		{
			size_t		size = std::min((size_t)BUFFER_SIZE, body.size() - start);
			std::ostringstream	oss;
			oss << std::hex << size << "\r\n";
			oss << body.substr(start, size);
			oss << "\r\n";

			client->respond(oss.str());
			start += size;
		}

		// chunk ending
		client->respond("0\r\n\r\n");
	}
}

bool	Server::tryRegisterClient(t_time time)
{
	t_sockaddr_in client_addr = {};
	int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &m_addr_len);

	if (client_fd >= 0)
	{
		bool	success = false;

		for (auto& client : m_clients)
		{
			if (client->isAlive())
				continue;
			
			success = client->connect(client_fd, client_addr, time);
			m_sock_count++;
			break;
		}
		if (!success)
		{
			// failed to connect
			logError("Failed to connect client");
			return false;
		}
	}
	else
	{
		// accept failed
		logError("Accept failed");
		return false;
	}

	return true;
}

void	Server::handleClients()
{
	auto time = std::chrono::steady_clock::now();

	for (auto& client : m_clients)
	{
		if (client->isAlive() && client->timeout(time))
		{
			log("Client time out!");
			client->disconnect();
			m_sock_count--;
		}
	}

	if (!(m_listener->revents & POLLIN))
		return;

	if (m_sock_count >= m_max_sockets)
		return;

	tryRegisterClient(time);

	m_listener->revents = 0;
}

void	Server::handleEvents()
{
	auto now = std::chrono::steady_clock::now();

	for (auto& client : m_clients)
	{
		if (!client->isAlive())
			continue;

		if (client->incoming())
		{
			handleClient(client);
			client->update(now);
			client->resetEvents();
		}
	}

}

void	Server::update()
{
	int	r = poll(m_pollfd.data(), m_max_sockets, 5000);

	// could handle timeouts here

	if (r == -1)
		return;

	handleClients();
	handleEvents();

}
