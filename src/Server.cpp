
#include "Client.hpp"
#include "Server.hpp"
#include "Parse.hpp"
#include "Response.hpp"
#include "Const.hpp"
#include "HttpHandler.hpp"

#include <string>
#include <algorithm> // min
#include <sstream>
#include <fstream>
#include <poll.h>
#include <fcntl.h>

Server::Server()
{
	//m_pollfd.reserve(128);
}

Server::~Server()
{
	for (auto& pollfd : m_pollfd_vector)
	{
		if (pollfd.fd > 0)
			close(pollfd.fd);
	}

	m_pollfd.clear();
	m_clients.clear();
}

bool	Server::startServer()
{
	if (!m_config.isValid())
	{
		Logger::logError("Error in config file");
		return false;
	}

	for (auto& [server, node] : m_config.getNodeMap())
	{
		int port = m_config.getPort(server);
		Logger::log("Starting server on " + server + ":" + std::to_string(port) + "...");

		if (m_listeners.find(port) == m_listeners.end())
			createListener(port);
		
	}

	return true;
}

int	Server::createListener(int port)
{
	t_sockaddr_in socket_addr;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd <= 0)
	{
		Logger::logError("Server failed to open socket!");
		return false;
	}

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
		Logger::logError("setsockopt failed!");
        perror("setsockopt failed");
        close(fd);
        return false;
    }

	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = INADDR_ANY;
	socket_addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr)) < 0)
	{
		Logger::logError("Bind " + std::to_string(port) + " failed");
		close(fd);
		closeServer();
		return false;
	}

	if (listen(fd, m_max_backlog) < 0)
	{
		Logger::logError("Listen " + std::to_string(port) + " failed");
		close(fd);
		closeServer();
		return false;
	}

	// safe to use 1 2 3 4 5... here
	size_t index = m_pollfd.size();
	pollfd pfd { fd, POLLIN, 0 };
	m_pollfd_vector.push_back(pfd);
	m_pollfd[index] = index;
	m_listeners[port] = index;

	m_socket_addr.push_back(socket_addr); // not sure if required

	Logger::log("Listen port " + std::to_string(port) + " fd " + std::to_string(pfd.fd));

	return true;
}

// technically not required
void	Server::closeServer()
{
	Logger::log("Closing server...");

	for (auto& pollfd : m_pollfd_vector)
	{
		if (pollfd.fd > 0)
			close(pollfd.fd);
	}

	Logger::log("Server closed!");
}

bool	Server::tryRegisterClient(t_time time)
{
	// if full, should try to disconnect some idle clients
	// if (m_sock_count >= m_max_sockets)
	// 	return;

	for (auto& [port, index] : m_listeners)
	{

		if (m_pollfd_vector[index].revents & POLLIN) { }
		else
		{
			continue;
		}

		//m_pollfd_vector[index].revents = 0;

		// move to addClient()
		t_sockaddr_in client_addr = {};
		m_addr_len = sizeof(client_addr);
		int client_fd = accept(m_pollfd_vector[index].fd, (struct sockaddr*)&client_addr, &m_addr_len);

		if (client_fd < 0)
		{
			Logger::logError("Accept client failed");
			return false;
		}

		int	fd_index = m_pollfd_vector.size();
		struct pollfd npfd { client_fd, POLLIN, 0 };
		m_pollfd_vector.push_back(npfd);

		std::shared_ptr<Client> client = std::make_shared<Client>(fd_index);
		bool success = client->connect(client_fd, client_addr, time);

		if (success)
		{
			m_pollfd[client_fd] = fd_index;
			m_clients[client_fd] = client;
			m_sock_count++;
		}
		else
		{
			Logger::logError("Failed to connect client");
			m_pollfd_vector.pop_back();
		}

	}

	return true;
}

void	Server::handleClients()
{
	auto time = std::chrono::steady_clock::now();

	std::vector<std::shared_ptr<Client>> client_list;

	for (auto& [index, client] : m_clients)
	{
		if (client->isAlive() && client->timeout(time))
		{
			Logger::log("Client " + std::to_string(client->getIndex()) + " timed out!");
			//removeClient(client);
			//client->respond("HTTP/1.1 408 Request Timeout\r\nConnection: close\r\n\r\n");
			client_list.push_back(client);
			m_sock_count--;
		}
	}

	for (auto& client : client_list)
		removeClient(client);

	tryRegisterClient(time);
}

void	Server::handleRequest(std::shared_ptr<Client> client)
{
	HttpHandler	httpHandler(client->fd());
	HttpResponse httpResponse = httpHandler.handleRequest(m_config);

	if (httpResponse.getStatusCode() == 408)
	{
		//client->update(m_time);
		m_disconnect.push_back(client);
		return ;
	}

	// CGI

	if (httpResponse.getSendType() == TYPE_SINGLE)
	{
		Logger::log(httpResponse.getResponse());
		respond(client, httpResponse.getResponse());
	}
	else if (httpResponse.getSendType() == TYPE_CHUNKED)
	{
		respond(client, httpResponse.getHeader());

		std::ifstream file;
		try
		{
			file = std::ifstream(httpHandler.getTarget(), std::ios::binary);
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
			respond(client, oss.str());
			Logger::log("chunk encoding");
		}

		respond(client, "0\r\n\r\n");
		Logger::log("end chunk encoding");
	}
}

void	Server::handleEvents()
{
	for (auto& [index, client] : m_clients)
	{
		if (!client->isAlive())
			continue;

		if (clientEvent(client, POLLIN))
		{
			handleRequest(client);
			client->update(m_time);
		}
	}

	if (m_disconnect.empty())
		return;

	for (auto& client : m_disconnect)
		removeClient(client);

	m_disconnect.clear();

}

void	Server::update()
{

	m_time = std::chrono::steady_clock::now();

	int	r = poll(m_pollfd_vector.data(), m_pollfd.size(), 1000);

	if (r == -1)
		return;

	handleClients();
	handleEvents();

}

bool	Server::clientEvent(std::shared_ptr<Client> client, int event)
{
	return m_pollfd_vector[client->getIndex()].revents & event;
}

int		Server::respond(std::shared_ptr<Client> client, const std::string& msg)
{
	client->respond(msg);
	m_pollfd_vector[client->getIndex()].revents = POLLOUT;
	return 1;
}

void	Server::removeClient(std::shared_ptr<Client> client)
{
	if (!client->isAlive())
		return;

	client->respond("HTTP/1.1 408 Request Timeout\r\nConnection: close\r\n\r\n");
	client->disconnect();

	if (m_clients.find(client->fd()) != m_clients.end())
		m_clients.erase(client->fd());

	auto it = m_pollfd.find(client->fd());
	if (it != m_pollfd.end())
	{
		m_pollfd_vector.erase(m_pollfd_vector.begin() + it->second);
		rebuildPollVector();
	}

}

void	Server::rebuildPollVector()
{
	m_pollfd.clear();
	for (size_t i = 0; i < m_pollfd_vector.size(); ++i)
		m_pollfd[m_pollfd_vector[i].fd] = i;
}

// void asd(Config config)
// {
// 	ConfigNode server_block = config.getServerBlock("127.0.0.1:8080");

// 	server_block.findDirective()


// }