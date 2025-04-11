
#include "Client.hpp"
#include "Server.hpp"
#include "Const.hpp"
#include "CGI.hpp"

#include <string>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cerrno>
#include <cstring>

Server::Server(const std::string& path) : m_config(path), m_events(EPOLL_POOL)
{
}

Server::~Server()
{
	//Logger::log("closing server...");
	for (auto& fd : m_cgi_fd)
	{
		if (fd >= 0)
		{
			epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
			close(fd);
		}
	}

	for (auto& [fd, client] : m_clients)
	{
		if (client)
		{
			client->disconnect();
			delete client;
		}
	}

	for (auto& [fd, port] : m_port_map)
	{
		if (fd >= 0) {
			epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
			shutdown(fd, SHUT_RDWR);
			close(fd);
		}
	}

	if (m_epoll_fd >= 0)
		close(m_epoll_fd);

}

bool	Server::startServer()
{
	if (!m_config.isValid())
	{
		Logger::logError("invalid config file");
		return false;
	}

	m_epoll_fd = epoll_create1(0);
	if (m_epoll_fd == -1)
	{
		Logger::logError("failed to create epoll instance");
		return false;
	}
	Logger::log("server_fd: " + std::to_string(m_epoll_fd));

	for (auto& [name, node] : m_config.getNodeMap())
	{
		std::vector<std::string> listen;

		// should validatate, but we are pretty sure it exists
		if (!node->tryGetDirective("listen", listen))
		{
			Logger::logError("missing listen directive, skipping server block");
			continue;
		}

		// should always exist at this point
		if (listen.empty())
		{
			Logger::logError("listen directive is empty, skipping server block");
			continue;
		}

		// port is already validated during config parsing
		unsigned int port = std::stoi(listen.front());
		Logger::log("starting server on " + name + ":" + std::to_string(port) + "...");

		if (!m_port_map.count(port))
		{
			if (!createListener(port))
				return false;
			m_config.setDefault(port, node);
		} 
		else if (listen.back() == "default_server")
		{
			m_config.setDefault(port, node);
		}
		
	}

	return true;
}

int	Server::createListener(int port)
{
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (fd <= 0)
	{
		Logger::logError("server failed to open socket!");
		return false;
	}

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
		Logger::logError("setsockopt failed!");
        close(fd);
        return false;
    }

	t_sockaddr_in socket_addr { };
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = INADDR_ANY;
	socket_addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr)) < 0)
	{
		Logger::logError("bind " + std::to_string(port) + " failed!");
		close(fd);
		return false;
	}

	if (listen(fd, SOMAXCONN) == -1)
	{
		Logger::logError("listen " + std::to_string(port) + " failed");
		close(fd);
		return false;
	}

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = fd;

	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		Logger::logError("epoll_ctl failed");
		close(fd);
		return false;
	}

	m_port_map[fd] = port;

	Logger::log("listen port " + std::to_string(port) + " fd " + std::to_string(fd));

	return true;
}

/// @brief Handle epoll events
void	Server::handleEvents(int event_count)
{
	for (int i = 0; i < event_count; i++)
	{
		int fd = m_events[i].data.fd;

		if (m_port_map.count(fd))
			addClient(fd);
		else if (m_client_fd.count(fd) && m_events[i].events & EPOLLIN)
			handleRequest(fd);
		else if (m_cgi_fd.count(fd) && m_events[i].events & EPOLLIN)
			handleCgiResponse(fd);
	}

}

/// @brief Accept new client
/// @param fd listener file descriptor
void	Server::addClient(int fd)
{
	t_sockaddr_in client_addr = { };
	m_addr_len = sizeof(client_addr);
	int client_fd = accept4(fd, (struct sockaddr*)&client_addr, &m_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

	if (client_fd < 0)
	{
		Logger::logError("error connecting client, accept");
		return;
	}

	int port = m_port_map[fd];
	Client* client = new Client(client_fd, *this);
	client->connect(fd, client_fd, m_time, port);

	if (m_clients.size() >= m_max_clients)
	{
		Logger::logError("rejected incoming connection, server is full");
		client->respond(RESPONSE_BUSY);
		close(client_fd);
		delete client;
		return;
	}

	epoll_event	event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = client_fd;

	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
	{
		Logger::logError("error connecting client, epoll_ctl");
		close(client_fd);
		delete client;
		return;
	}

	m_clients[client_fd] = client;
	m_client_fd.insert(client_fd);
	
	auto node = m_config.findServerNode(":" + std::to_string(port));

	std::vector<std::string> timeout;
	node->tryGetDirective("keepalive_timeout", timeout);
	client->setTimeoutDuration(std::stoul(timeout.front()));
}

/// @brief Handle client timeouts
void	Server::handleTimeouts()
{
	for (auto& [fd, client] : m_clients)
	{
		if (client->getClientState() == ClientState::CONNECTED && client->timeout(m_time))
		{
			Logger::log("client " + std::to_string(client->fd()) + " is idle");
			client->setClientState(ClientState::TIMEDOUT);

			auto time = m_time + client->getTimeoutDuration() + std::chrono::seconds(2);
			client->setDisconnectTime(time);
			m_timeout_queue.push({time, client});
			m_timeout_set.insert(client);
		}
	}

	int max = 20;
	while (!m_timeout_queue.empty() && max > 0 && m_time >= m_timeout_queue.top().time)
	{
		auto c = m_timeout_queue.top();
		m_timeout_queue.pop();
		m_timeout_set.erase(c.client);
		checkResponseState(c.client);
		--max;
	}

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

	Client* client = m_clients.at(fd);
	
	std::vector<char>	vec;
	char				buffer[PACKET_SIZE];
	ssize_t				bytes_read;
	
	while ((bytes_read = recv(fd, buffer, PACKET_SIZE, 0)) > 0)
	{
		Logger::log("-- BYTES READ " + std::to_string(bytes_read) + " --");
		vec.insert(vec.end(), buffer, buffer + bytes_read);
	}

	if (bytes_read == 0 && vec.empty())
	{
		Logger::log("== REMOTE CLOSE CONNECTION ==");
		removeClient(client, 0);
		return;
	}
	
	if (bytes_read == -1 && vec.empty())
	{
		//Logger::logError(std::strerror(errno));
		Logger::logError("EAGAIN (bytes_read == -1)");
		return;
	}

	if (!vec.empty())
	{
		//Logger::log(vec.data());
		client->handleRequest(m_config, vec);
		updateClient(client);
	}

	if (client->requestState() == COMPLETE)
	{
		Logger::log(client->header());
		Logger::log("== SEND RESPONSE ==");
		
		if (client->sendType() == TYPE_SINGLE)
		{
			client->respond(client->response());
		}
		else if (client->sendType() == TYPE_CHUNKED)
		{
			client->respond(client->header());
			client->respondChunked(client->path());
		}

		if (client->closeConnection())
		{
			Logger::log("== CLOSE CONNECTION ==");
			removeClient(client, 0);
		}
		else
		{
			updateClient(client);
		}
	}
}

void Server::handleCgiResponse(int fd)
{
	// shouldn't really happen
	if (m_cgi_map.find(fd) == m_cgi_map.end())
	{
		Logger::logError("cgi fd " + std::to_string(fd) + " doesnt exist in map");
		return;
	}

	int client_fd = m_cgi_map[fd];

	// shouldn't really happen
	if (m_clients.find(client_fd) == m_clients.end())
	{
		Logger::log("client " + std::to_string(client_fd) + " doesnt exist in map");
		return;
	}

	Client* client = m_clients.at(client_fd);

	std::string			str;
	char				buffer[BUFFER_SIZE];
	ssize_t				bytes_read;

	while ((bytes_read = recv(fd, buffer, BUFFER_SIZE, 0)) > 0)
	{
		Logger::log("-- CGI BYTES READ " + std::to_string(bytes_read) + " --");
		str.append(buffer, bytes_read);
	}

	Logger::log("-- CGI READ DONE --");

	if (bytes_read == 0 && client->getCGIState() == CGI_OPEN)
	{
		Logger::log("CGI EOF");
		client->setCGIState(CGI_CLOSED);
		//kill(client->getCPID(), SIGKILL);
	}

	if (client->getChildState() == P_RUNNING)
	{
		int	status;
		int cpid = client->getCPID();
		int result = waitpid(cpid, &status, WNOHANG); // check nonblocking
	
		if (result == cpid)
		{
			Logger::log("CGI process exited");
			client->setChildState(P_CLOSED);
			client->resetCPID();
		} else {
			Logger::log("waiting for pid " + std::to_string(cpid));
		}
	}

	if (bytes_read == -1 && str.empty())
	{
		//Logger::logError(std::strerror(errno));
		Logger::log("CGI EAGAIN (bytes_read == -1)");
		return;
	}

	if (!str.empty())
	{
		client->appendResponseBody(str);
		updateClient(client);
	}


	if (client->getCGIState() == CGI_CLOSED && client->getChildState() == P_CLOSED)
	{
		Logger::log("-- CGI DONE, RESPOND TO CLIENT " + std::to_string(client_fd) + " --");

		updateClient(client);
		client->cgiResponse();
		client->respond(client->response());

		//removeClient(client); // test
		removeCGI(fd);
	}

}

bool	Server::checkResponseState(Client* client)
{
	if (client->getClientState() > 0)
	{
		removeClient(client, 1);
		return true;
	}
	return false;
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

void	Server::removeClient(Client* client, int type)
{
	updateClient(client);
	m_clients.erase(client->fd());
	m_client_fd.erase(client->fd());
	Logger::log("Client fd " + std::to_string(client->fd()) + " disconnected!");

	for (auto [cgifd, clientfd] : m_cgi_map)
	{
		if (clientfd == client->fd())
		{
			type = 2;
			removeCGI(cgifd);
			break;
		}
	}

	if (type == 1) // server enforced
	{
		client->respond(client->errorResponse(408, "Request Timeout"));
	}
	else if (type == 2) // cgi timeout
	{
		client->respond(client->errorResponse(500, "Internal Server Error"));
	}

	client->disconnect();
	delete client;
}

void	Server::removeCGI(int fd)
{
	m_cgi_map.erase(fd);
	m_cgi_fd.erase(fd);
	epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

void	Server::updateClient(Client* client)
{
	client->update(m_time);

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