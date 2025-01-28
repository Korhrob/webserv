
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

Server::Server() : m_pollfd(1), m_listener(m_pollfd[0])
{
}

Server::Server(std::shared_ptr<ConfigNode> node) : m_server_node(node), m_pollfd(m_max_sockets + 1), m_listener(m_pollfd[0])
{
	Logger::log(node->getName());

	m_address = "0.0.0.0"; // get from server_block;
	m_port = std::stoul(node->findDirective("listen").front());
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
	//closeServer();

	if (m_socket >= 0)
		close(m_socket);

	for (auto& pollfd : m_pollfd)
	{
		if (pollfd.fd > 0)
			close(pollfd.fd);
	}

	m_pollfd.clear();
	m_clients.clear();
}

bool	Server::startServer()
{
	// already checked before start server, could add validate server block function
	// if (!m_config.isValid())
	// {
	// 	Logger::logError("Error in config file");
	// 	return false;
	// }

	Logger::log("Starting server on " + m_address + ":" + std::to_string(m_port) + "...");

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket <= 0)
	{
		Logger::logError("Server failed to open socket!");
		return false;
	}

    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
		Logger::logError("setsockopt failed!");
        perror("setsockopt failed");
        close(m_socket);
        return false;
    }

	m_socket_addr.sin_family = AF_INET;
	m_socket_addr.sin_addr.s_addr = INADDR_ANY;
	m_socket_addr.sin_port = htons(m_port);

	if (bind(m_socket, (struct sockaddr*)&m_socket_addr, sizeof(m_socket_addr)) < 0)
	{
		Logger::logError("Bind failed");
		closeServer();
		return false;
	}

	m_listener.fd = m_socket;

	Logger::log("Server started on " + m_address + ":" + std::to_string(m_port));
	startListen();

	return true;
}

// technically not required
void	Server::closeServer()
{
	Logger::log("Closing " + m_server_node->getName() + "...");

	if (m_socket >= 0)
		close(m_socket);

	for (auto& pollfd : m_pollfd)
	{
		if (pollfd.fd > 0)
			close(pollfd.fd);
	}

	Logger::log("Server closed!");
}

void	Server::startListen()
{
	if (listen(m_socket, m_max_backlog) < 0)
	{
		Logger::logError("Listen failed");
		closeServer();
	}

	Logger::log("Server is listening...");
}

void	Server::handleClient(std::shared_ptr<Client> client)
{
	Response	http_response(client, m_server_node);

	if (http_response.getStatus() == STATUS_FAIL)
	{
		Logger::logError("Client disconnected or error occured");
		client->disconnect();
		return;
	}

	if (http_response.getStatus() == STATUS_BLANK)
		return ;

	// CGI

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
			Logger::log("chunk encoding");
		}

		client->respond("0\r\n\r\n");
		Logger::log("end chunk encoding");
	}
}

bool	Server::tryRegisterClient(t_time time)
{
	
	t_sockaddr_in client_addr = {};
	m_addr_len = sizeof(client_addr);
	int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &m_addr_len);

	if (client_fd < 0)
	{
		Logger::logError("Accept failed");
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
		Logger::logError("Failed to connect client");

	return success;
}

void	Server::handleClients()
{
	auto time = std::chrono::steady_clock::now();

	for (auto& client : m_clients)
	{
		if (client->isAlive() && client->timeout(time))
		{
			Logger::log("Client time out!");
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

	int	r = poll(m_pollfd.data(), m_pollfd.size(), 5000);

	// could handle timeouts here

	if (r == -1)
		return;

	handleClients();
	handleEvents();

	// for (int i = m_max_sockets + 1; i < m_pollfd.size(); i++)
	// {

	// }

}

