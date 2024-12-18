
#include "Client.hpp"
#include "Server.hpp"
#include "Parse.hpp"
#include "Response.hpp"
#include "Const.hpp"

#include <string>
#include <algorithm> // min
#include <sstream>
#include <fstream>
#include <poll.h>
#include <fcntl.h>

Server::Server(const std::string& ip, int port) : m_pollfd(m_max_sockets + 1), m_listener(m_pollfd[0])
{
	// should read config file and initialize all server variables here
	// read more about config files for NGINX for reference

	m_address = ip;
	m_port = port;
	m_sock_count = 0;

	//m_pollfd.resize(m_max_sockets + 1);
	m_pollfd[0] = { m_socket, POLLIN, 0 };

	for (int i = 0; i < m_max_sockets; i++)
	{
		auto client = std::make_shared<Client>(m_pollfd[i + 1]);
		m_clients.push_back(client);
	}

}

Server::~Server()
{
	closeServer();

	m_pollfd.clear();
	m_clients.clear();
}

bool	Server::startServer()
{
	if (!m_config.isValid())
	{
		logError("Error in config file");
		return false;
	}

	log("Starting server on " + m_address + ":" + std::to_string(m_port) + "...");

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket <= 0)
	{
		logError("Server failed to open socket!");
		return false;
	}

    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(m_socket);
        return false;
    }

	m_socket_addr.sin_family = AF_INET;
	m_socket_addr.sin_addr.s_addr = INADDR_ANY;
	m_socket_addr.sin_port = htons(m_port);

	if (bind(m_socket, (struct sockaddr*)&m_socket_addr, sizeof(m_socket_addr)) < 0)
	{
		logError("Bind failed");
		closeServer();
		return false;
	}

	m_listener.fd = m_socket;

	log("Server started on " + m_address + ":" + std::to_string(m_port));
	startListen();

	return true;
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

	log("Server is listening...");
}

void	Server::handleClient(std::shared_ptr<Client> client)
{
	Response	http_response(client);

	if (http_response.getStatus() == STATUS_FAIL)
	{
		logError("Client disconnected or error occured");
		client->disconnect();
		return;
	}

	if (http_response.getStatus() == STATUS_BLANK)
		return ;

	if (http_response.getSendType() == TYPE_SINGLE)
	{
		client->respond(http_response.str());
		//m_sockets[id].revents = POLLOUT;
	}
	else if (http_response.getSendType() == TYPE_CHUNK)
	{
		client->respond(http_response.header());
		//m_sockets[id].revents = POLLOUT;

		std::ifstream file;
		try
		{
			file = std::ifstream(http_response.path(), std::ios::binary);
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return;
		}
		
		char buffer[PACKET_SIZE];
		while (true) {

			file.read(buffer, PACKET_SIZE);
			std::streamsize count = file.gcount();

			if (count <= 0)
				break;

			if (count < PACKET_SIZE)
				buffer[count] = '\0';

			std::ostringstream	oss;
			oss << std::hex << count << "\r\n";
			oss.write(buffer, count);
			oss << "\r\n";
			client->respond(oss.str());
			log("chunk encoding");
		}

		client->respond("0\r\n\r\n");
		log("end chunk encoding");
	}
}

bool	Server::tryRegisterClient(t_time time)
{
	t_sockaddr_in client_addr = {};
	int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &m_addr_len);

	if (client_fd < 0)
	{
		logError("Accept failed");
		return false;
	}

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
		logError("Failed to connect client");

	return success;
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

	if (!(m_listener.revents & POLLIN))
		return;

	if (m_sock_count >= m_max_sockets)
		return;

	tryRegisterClient(time);

	m_listener.revents = 0;
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

Config&	Server::getConfig()
{
	return m_config;
}
