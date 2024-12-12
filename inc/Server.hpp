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

#include "ILog.hpp"
#include "Client.hpp"
#include "Const.hpp"
#include "Config.hpp"

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;

class Server
{

	private:
		std::string			m_address;
		int					m_port;
		int					m_socket;
		t_sockaddr_in		m_socket_addr;
		unsigned int		m_addr_len = sizeof(t_sockaddr_in);
		struct pollfd*		m_listener;	// shortcut (TODO: change to unique ptr)

		int											m_max_backlog = 8;
		int											m_sock_count;
		int											m_max_sockets = 16;	// get from config
		std::vector<struct pollfd>					m_pollfd;
		std::vector<std::shared_ptr<Client>>		m_clients;
		
		Config				m_config;

	public:
		Server(const std::string& ip);
		~Server();

		bool	startServer();
		void	closeServer();
		void	startListen();
		bool	tryRegisterClient(t_time time);
		void	handleClient(std::shared_ptr<Client> client);
		void	handleClients();
		void	handleEvents();
		void	update();
};