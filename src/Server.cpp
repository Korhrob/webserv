
#include "Client.hpp"
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
	if (listen(m_socket, 5) < 0)
	{
		logError("Listen failed");
		closeServer();
	}

	log("Server is listenning...");
}

std::string	Server::get()
{
	log("GET method");

	std::string	response;

	// get requested filepath

	return response;
}

std::string	Server::post()
{
	log("POST method");

	std::string	response;

	/// ???

	return response;
}

bool	Server::parseRequest(std::shared_ptr<Client> client, std::string *request)
{
	log("parseRequest");

	// read a bit about max request size for HTTP/1.1
	char	buffer[1024];
	memset(buffer, 0, 1024);

	// see recv instead of read, also use size_t
	size_t	bytes_read = recv(client->fd(), buffer, 1024, 0);

	std::cout << "-- BYTES READ " << bytes_read << "--\n\n" << std::endl;

	if (bytes_read > 0)
	{
		*request = std::string(buffer);

		// first line
		// check request method, like GET and its target, if no target default to index.html
		
		// other important info, such as Connection: close, so we know to close the connection

		std::cout << buffer << std::endl;
		return true;
	}

	return false;
}

void	Server::handleClient(std::shared_ptr<Client> client)
{

	std::string	request;
	std::string response;

	if (!parseRequest(client, &request))
	{
		logError("Client disconnected or error occured");
		client->disconnect();
		return;
	}

	//m_sockets[id].revents = POLLOUT;
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
		// response += "Connection: keep-alive\r\n";  

		// empty line
		response += "\r\n";
		
		// body
		response += body;
	} else {
		body = "some other type of file";

		response = "HTTP/1.1 200 OK\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		//response += "Connection: keep-alive\r\n";
		response += "\r\n";
		response += body;
	}

	client->respond(response);
}

void	Server::handleClients()
{
	auto now = std::chrono::steady_clock::now();

	for (auto& client : m_clients)
	{
		if (client->isAlive() && client->timeout(now))
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

	log("new listener event");

	// accept new connection
	t_sockaddr_in client_addr = {};
	int client_fd = accept(m_socket, (struct sockaddr*)&client_addr, &m_addr_len);

	if (client_fd >= 0)
	{
		bool	success = false;

		for (auto& client : m_clients)
		{
			if (client->isAlive())
				continue;
			
			success = client->connect(client_fd, client_addr, now);
			m_sock_count++;
			break;
		}
		if (!success)
		{
			// failed to connect
			logError("Failed to connect client");
		}
	}
	else
	{
		// accept failed
		logError("Accept failed");
	}

	m_listener->revents = 0;
}

void	Server::handleEvents()
{
	auto now = std::chrono::steady_clock::now();

	// listen for all poll events
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