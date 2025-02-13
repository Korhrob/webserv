#pragma once

#include <string> // std::string
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr
#include <string.h>
#include <sstream>
#include <chrono> // time
#include <vector> // vector
#include <memory> // shared_ptr
#include <unordered_map>
#include <sys/epoll.h> // epoll

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

		t_time										m_time;
		std::vector<t_sockaddr_in>					m_socket_addr;
		std::vector<std::shared_ptr<Client>>		m_timeout_list;

		// new stuff
		int													m_epoll_fd;
		std::vector<epoll_event>							m_events;
		std::vector<int>									m_listeners; // TODO: remove
		std::unordered_map<int, int>						m_port_map; // fd -> port
		std::unordered_map<int, std::shared_ptr<Client>>	m_clients;


	public:
		Server(); // pass string for config name
		~Server();

		bool	startServer();
		void	closeServer();
		int		createListener(int port);
		void	addClient(int fd);
		void	handleEvents(int event_count);
		void	handleTimeouts();
		void	handleRequest(int fd);
		void	update();
};