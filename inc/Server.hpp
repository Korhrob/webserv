#pragma once

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sstream>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <queue>
#include <unordered_set>

#include "Logger.hpp"
#include "Client.hpp"
#include "Const.hpp"
#include "Config.hpp"

using t_sockaddr_in = struct sockaddr_in;
using t_time = std::chrono::steady_clock::time_point;

struct TimeoutClient
{
	t_time					time;
	Client*					client;

	bool operator>(const TimeoutClient& other) const
	{
		return time > other.time;
	}
};

using Queue = std::priority_queue<TimeoutClient, std::vector<TimeoutClient>, std::greater<>>;
using Set = std::unordered_set<Client*>;
using ClientMap = std::unordered_map<int, Client*>;

class Server
{

	private:
		Config							m_config;
		unsigned int					m_addr_len = sizeof(t_sockaddr_in);
		const std::size_t 				m_max_clients = 1024;

		t_time							m_time;

		int								m_epoll_fd;
		std::vector<epoll_event>		m_events;
		std::unordered_map<int, int>	m_port_map; // listen fd -> listen port
		std::unordered_map<int, int>	m_cgi_map;  // cgi fd -> client fd
		ClientMap						m_clients;
		Queue							m_timeout_queue;
		Set								m_timeout_set;

		std::unordered_set<int>			m_client_fd;
		std::unordered_set<int>			m_cgi_fd;

		int								m_cleanup;

	public:

		Server(const std::string& path);
		~Server();

		bool	startServer();
		int		createListener(int port);
		void	addClient(int fd);
		void	removeClient(Client* client, int type);
		void	removeCGI(int fd);
		void	updateClient(Client* client);
		void	handleEvents(int event_count);
		void	handleTimeouts();
		void	handleRequest(int fd);
		void	handleCgiResponse(int fd);
		bool	checkResponseState(Client* client);
		void	update();
		void	mapClientCgi(int cgi_fd, int client_fd)
		{
			m_cgi_map[cgi_fd] = client_fd;
			m_cgi_fd.insert(cgi_fd);
		};
		void	setCleanFlag(int i) { m_cleanup = i; }

		int	getEpollFd() { return m_epoll_fd; };
};