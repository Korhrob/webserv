
#include "Client.hpp"
#include "Server.hpp"
// #include "Parse.hpp"
#include "Const.hpp"
#include "HttpHandler.hpp"

#include <string>
#include <algorithm> // min
#include <sstream>
#include <fstream>
#include <poll.h>
#include <fcntl.h>

Server::Server() : m_events(16)
{
}

Server::~Server()
{
	for (auto& poll_event : m_events)
	{
		if (poll_event.data.fd > 0)
			close(poll_event.data.fd);
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

	for (auto& [server, node] : m_config.getNodeMap())
	{
		std::vector<std::string> listen;

		// should validatate, but we are pretty sure it exists
		if (!node->tryGetDirective("listen", listen))
		{
			Logger::logError("missing listen directive, skipping server block");
			continue;
		}

		if (listen.empty())
		{
			Logger::logError("listen directive is empty, skipping server block");
			continue;
		}

		// port is already validated during config parsing
		unsigned int port = std::stoul(listen.front());
		Logger::log("Starting server on " + server + ":" + std::to_string(port) + "...");

		auto it = std::find(m_listeners.begin(), m_listeners.end(), port);
		if (it == m_listeners.end()) //(m_listeners.find(port) == m_listeners.end())
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

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = fd;

	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		Logger::logError("epoll_ctrl failed");
		close(fd);
		return false;
	}

	m_listeners.push_back(fd);
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
	t_sockaddr_in client_addr = {};
	m_addr_len = sizeof(client_addr);
	int client_fd = accept(fd, (struct sockaddr*)&client_addr, &m_addr_len);

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
	client->connect(client_fd, client_addr, m_time); // bool success = 

	//Logger::log("register client " + std::to_string(client_fd));
	m_clients[client_fd] = client;
	// m_client_count++;
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
			m_timeout_list.push_back(client);
		}
	}

	// there has to be a cleaner way
	for (auto it = m_timeout_list.begin(); it != m_timeout_list.end();)
	{
		auto client = *it;
		if (m_time >= client->getDisconnectTime())
		{
			//Logger::log("remove client " + std::to_string(client->fd()));
			m_clients.erase(client->fd());
			client->disconnect();
			it = m_timeout_list.erase(it);
		}
		else
		{
			++it;
		}
	}

}

/// @brief Handle parsing request and sending response
/// @param fd client file descriptor
void	Server::handleRequest(int fd)
{
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
		//Logger::log("client " + std::to_string(client->fd()) + " EOF");

		// manually removed from client collection after receiving EOF
		client->disconnect();
		m_clients.erase(client->fd());

		return ;
	}

	client->update(m_time);

	// CGI
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

	auto event_count = epoll_wait(m_epoll_fd, m_events.data(), m_events.size(), CLIENT_TIMEOUT);

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
