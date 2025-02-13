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
#include <queue> // priority_queue

#include "Logger.hpp"
#include "Client.hpp"
#include "Const.hpp"
#include "Config.hpp"

using t_sockaddr_in = struct sockaddr_in;
using t_time = std::chrono::steady_clock::time_point;

		struct TimeoutClient
		{
			t_time					time;
			std::shared_ptr<Client>	client;

			bool operator>(const TimeoutClient& other) const
			{
				return time > other.time;
			}
		};

using Queue = std::priority_queue<TimeoutClient, std::vector<TimeoutClient>, std::greater<>>;
using ClientMap = std::unordered_map<int, std::shared_ptr<Client>>;

class Server
{

	private:
		Config							m_config;
		unsigned int					m_addr_len = sizeof(t_sockaddr_in);
		int								m_max_backlog = 128; // check config for override

		t_time							m_time;

		// new stuff
		int								m_epoll_fd;
		std::vector<epoll_event>		m_events;
		std::vector<int>				m_listeners; // TODO: remove
		std::unordered_map<int, int>	m_port_map; // fd -> port (used to check listeners)
		ClientMap						m_clients;
		Queue							m_timeout_queue;


	public:

		Server(); // pass string for config name
		~Server();

		bool	startServer();
		void	closeServer();
		int		createListener(int port);
		void	addClient(int fd);
		void	removeClient(std::shared_ptr<Client> client);
		void	handleEvents(int event_count);
		void	handleTimeouts();
		void	handleRequest(int fd);
		void	update();
};