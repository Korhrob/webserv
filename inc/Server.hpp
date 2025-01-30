#pragma once

#include <string>
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr
#include <poll.h> // pollfd
#include <string.h>
#include <sstream>
#include <chrono> // time
#include <vector> // vector
#include <memory> // unique ptr
#include <map>

#include "Logger.hpp"
#include "Client.hpp"
#include "Const.hpp"
#include "Config.hpp"

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;

class Server
{

	private:
		Config				m_config;
		unsigned int		m_addr_len = sizeof(t_sockaddr_in);

		int											m_max_backlog = 128;
		int											m_sock_count;
		int											m_max_sockets = 128;	// events { worker_connections}

		std::vector<struct pollfd>					m_pollfd_vector;
		std::map<size_t, size_t>					m_pollfd; // index, pollfd_vector index
		std::map<size_t, std::shared_ptr<Client>>	m_clients; /// index
		std::vector<t_sockaddr_in>					m_socket_addr;
		std::map<int, size_t>						m_listeners; // port, index
		std::vector<std::shared_ptr<Client>>		m_disconnect;


	public:
		// Server(const std::string& ip, int port); // depracated
		Server();
		~Server();

		bool	startServer();
		void	closeServer();
		int		createListener(int port);
		bool	tryRegisterClient(t_time time);
		void	handleClient(std::shared_ptr<Client> client);
		void	handleClients();
		void	handleEvents();
		void	update();
		bool	clientEvent(std::shared_ptr<Client> client, int event);
		int		respond(std::shared_ptr<Client> client, const std::string& response);
		void	removeClient(std::shared_ptr<Client> client);
		void	rebuildPollVector();
};