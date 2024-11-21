#pragma once

#include <string>
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr
#include <poll.h>

#include <string.h>
#include <sstream>

#include "ILog.hpp"
#include "Packet.hpp"
#include "WebClient.hpp"

typedef struct sockaddr_in t_sockaddr_in;

class Server
{

	private:
		std::string			m_address;
		int					m_port;
		int					m_socket;
		int					m_new_socket;
		long				m_inc_msg;
		t_sockaddr_in		m_socket_addr;
		unsigned int		m_addr_len = sizeof(t_sockaddr_in);

		std::string			m_server_msg;

		
		struct pollfd		m_sockets[16];
		int					m_sock_count;
		int					m_max_sockets = 16;
		int					m_files_sent[16];
		

	public:
		Server(const std::string& ip, int port);
		~Server();

		int		startServer();
		void	closeServer();
		void	startListen();
		void	handleClient(int id);
		bool	parseRequest(int id, std::string *request);
		void	update();
};