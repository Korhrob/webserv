
#include "Client.hpp"
#include "Server.hpp"
#include "Const.hpp"
#include "HttpHandler.hpp"

#include <string>
#include <algorithm> // min
#include <sstream>
#include <fstream>
#include <fcntl.h>

Server::Server() : m_events(EPOLL_POOL)
{
}

Server::~Server()
{
	for (auto& [fd, client] : m_clients)
	{
		if (fd > 0)
			close(fd);
	}
	for (auto& [fd, port] : m_port_map)
	{
		if (fd > 0)
			close(fd);
	}
}

bool	Server::startServer()
{
	if (!m_config.isValid())
	{
		Logger::logError("Error in config file");
		return false;
	}

	m_epoll_fd = epoll_create1(0);
	if (m_epoll_fd == -1)
	{
		Logger::logError("failed to create epoll instance");
		return false;
	}

	for (auto& [name, node] : m_config.getNodeMap())
	{
		std::vector<std::string> listen;

		// should validatate, but we are pretty sure it exists
		if (!node->tryGetDirective("listen", listen))
		{
			Logger::logError("missing listen directive, skipping server block");
			continue;
		}

		// should always exist at this point, but just incase
		if (listen.empty())
		{
			Logger::logError("listen directive is empty, skipping server block");
			continue;
		}

		// port is already validated during config parsing
		unsigned int port = std::stoul(listen.front());
		Logger::log("Starting server on " + name + ":" + std::to_string(port) + "...");

		if (!m_port_map.count(port))
		{
			createListener(port);
			m_config.setDefault(port, node);
		} 
		else if (listen.back() == "default_server")
		{
			// override current default
			m_config.setDefault(port, node);
		}
		
	}

	return true;
}

int	Server::createListener(int port)
{
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

	// int flags = fcntl(fd, F_GETFL, 0);
	// if (flags == -1)
	// {
	// 	Logger::logError("fctl get failed!");
	// 	close(fd);
	// 	return false;
	// }

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) // flags | 
	{
		Logger::logError("fcntl set failed!");
		close(fd);
		return false;
	}

	t_sockaddr_in socket_addr { };
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = INADDR_ANY;
	socket_addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr)) < 0)
	{
		Logger::logError("Bind " + std::to_string(port) + " failed!");
		close(fd);
		closeServer();
		return false;
	}

	if (listen(fd, SOMAXCONN) == -1)
	{
		Logger::logError("Listen " + std::to_string(port) + " failed");
		close(fd);
		closeServer();
		return false;
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = fd;

	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		Logger::logError("epoll_ctrl failed");
		close(fd);
		return false;
	}

	m_port_map[fd] = port;

	Logger::log("Listen port " + std::to_string(port) + " fd " + std::to_string(fd));

	return true;
}

/// @brief Handle any logic before deconstruction
void	Server::closeServer()
{
	Logger::log("Closing server...");
	// ...
	Logger::log("Server closed!");
}

/// @brief Handle epoll events
void	Server::handleEvents(int event_count)
{
	for (int i = 0; i < event_count; i++)
	{
		int fd = m_events[i].data.fd;

		if (m_port_map.count(fd))
			addClient(fd);
		else
			handleRequest(fd);
	}

}

/// @brief Accept new client
/// @param fd listener file descriptor
void	Server::addClient(int fd)
{
	t_sockaddr_in client_addr = { };
	m_addr_len = sizeof(client_addr);
	int client_fd = accept4(fd, (struct sockaddr*)&client_addr, &m_addr_len, O_NONBLOCK);

	if (client_fd < 0)
	{
		Logger::logError("error connecting client, accept");
		return;
	}

	// setup non blocking

	epoll_event	event;
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = client_fd;

	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
	{
		Logger::logError("error connecting client, epoll_ctl");
		close(client_fd);
		return;
	}

	std::shared_ptr<Client> client = std::make_shared<Client>(client_fd);
	client->connect(client_fd, m_time);
	m_clients[client_fd] = client;

}

/// @brief Handle client timeouts
void	Server::handleTimeouts()
{
	auto grace = m_time + std::chrono::seconds(2);

	for (auto& [fd, client] : m_clients)
	{
		if (client->getClientState() == ClientState::CONNECTED && client->timeout(m_time))
		{
			Logger::log("Client " + std::to_string(client->fd()) + " timed out!");
			client->setClientState(ClientState::TIMEDOUT);
			client->respond(RESPONSE_TIMEOUT);
			client->respond(EMPTY_STRING);
			client->setDisconnectTime(grace);
			m_timeout_queue.push({grace, client});
			m_timeout_set.insert(client);
		}
	}

	int max = 20;
	while (!m_timeout_queue.empty() && max > 0 && m_time >= m_timeout_queue.top().time)
	{
		auto c = m_timeout_queue.top();
		m_timeout_queue.pop();
		m_timeout_set.erase(c.client);
		removeClient(c.client);
		--max;
	}

	// could adjust epoll timeout rate

}

/// @brief Handle parsing request and sending response
/// @param fd client file descriptor
void	Server::handleRequest(int fd)
{
	// technically should never happen
	if (m_clients.find(fd) == m_clients.end())
	{
		Logger::logError("client " + std::to_string(fd) + " doesnt exist in map");
		return;
	}

	HttpHandler	httpHandler; // move to class to prevent constant instances
	HttpResponse httpResponse = httpHandler.handleRequest(fd, m_config);
	std::shared_ptr<Client> client = m_clients.at(fd);

	if (httpResponse.closeConnection())
	{
		Logger::log("== CLOSE CONNECTION ==");
		removeClient(client);
		return ;
	}

	updateClient(client);

	Logger::log("== SEND RESPONSE ==");

	if (httpResponse.getSendType() == TYPE_SINGLE)
	{
		//Logger::log(httpResponse.getResponse());
		client->respond(httpResponse.getResponse());
	}
	else if (httpResponse.getSendType() == TYPE_CHUNKED)
	{
		client->respond(httpResponse.getHeader());

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
			client->respond(oss.str());
			Logger::log("CHUNK");
		}

		client->respond("0\r\n\r\n");
		Logger::log("END CHUNK");
	}
}

/// @brief Server update logic
void	Server::update()
{
	m_time = std::chrono::steady_clock::now();

	auto event_count = epoll_wait(m_epoll_fd, m_events.data(), m_events.size(), 200);

	if (event_count == -1)
		return;

	if (event_count == static_cast<int>(m_events.size()))
	{
		Logger::log("== RESIZE EVENT COLLECTION ==");
		m_events.resize(m_events.size() * 2);
	}

	handleEvents(event_count);
	handleTimeouts();

}

void	Server::removeClient(std::shared_ptr<Client> client)
{
	m_clients.erase(client->fd());
	client->disconnect();
}

void	Server::updateClient(std::shared_ptr<Client> client)
{
	client->update(m_time);

	// remove client from timeout pool
	if (m_timeout_set.find(client) != m_timeout_set.end())
	{
		Queue new_queue;

		while (!m_timeout_queue.empty())
		{
			auto top = m_timeout_queue.top();
			m_timeout_queue.pop();
			if (top.client != client)
				new_queue.push(top);
		}

		m_timeout_queue = new_queue;
		m_timeout_set.erase(client);
	}	
}