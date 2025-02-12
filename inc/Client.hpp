#pragma once

#include <string>
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr

#include <string.h> // strlen
#include <poll.h> // pollfd
#include <fcntl.h> // fnctl
#include <chrono> // time
#include <sstream>
#include <unordered_map>
#include <fstream> // ofstream
#include <variant>
#include <vector>
#include <map>

#include "Logger.hpp" // log,  logError
#include "Const.hpp"

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;

// TODO: move inline function to their own .cpp file

class Client
{
	private:
		bool											m_alive;
		int												m_fd;
		size_t											m_pollfd_index;
		t_sockaddr_in									m_addr;
		unsigned int									m_files_sent;
		t_time											m_last_activity;
		bool											m_close_connection = false;
		t_time											m_disconnect_time;

	public:

		Client(size_t index) : m_pollfd_index(index)
		{
			// m_pollfd.fd = -1;
			// m_pollfd.events = POLLIN | POLLOUT;
			// m_pollfd.revents = 0;
			// m_alive = false;
		}
		~Client() {}

		bool	isAlive() { return m_alive; }
		// bool	incoming() { return m_pollfd.revents & POLLIN; }
		// bool	outgoing() { return m_pollfd.revents & POLLOUT; }
		// bool	fileIsOpen() { return m_file.is_open(); }

		int		getIndex() { return m_pollfd_index; }
		void	setIndex(int i) { m_pollfd_index = i; }
		int		fd() { return m_fd; }
		struct pollfd& getPollfd(std::vector<struct pollfd>& pollfd) { return pollfd[m_pollfd_index]; }

		bool	connect(int fd, t_sockaddr_in sock_addr, t_time time)
		{
			// set up non blocking fd (MACOS)
			// int flags = fcntl(fd, F_GETFL, 0);
    		// fcntl(fd, F_SETFL, flags | O_NONBLOCK);
			// check fcntl errors

			m_fd = fd;
			m_alive = true;

			m_addr = sock_addr;
			m_files_sent = 0;
			m_last_activity = time;

			Logger::log("Client id " + std::to_string(m_pollfd_index) + ", fd " + std::to_string(m_fd) + " connected!");

			return true;
		}

		void	disconnect()
		{
			Logger::log("Client id " + std::to_string(m_pollfd_index) + ", fd " + std::to_string(m_fd) + " disconnected!");
			if (m_fd >= 0)
				close(m_fd);
			// m_pollfd.fd = -1;
			// m_pollfd.revents = 0;
			m_alive = false;

			Logger::getInstance().log("Client disconnected!");
		}

		void	update(t_time time)
		{
			m_last_activity = time;
		}

		// rename send
		int	respond(const std::string& response)
		{
			// if connection is already closed
			if (m_fd < 0)
				return 0;

			#ifdef MSG_NOSIGNAL
				int bytes_sent = send(m_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
			#else
				int bytes_sent = send(m_fd, response.c_str(), response.size(), 0);
			#endif
			Logger::log("-- BYTES SENT " + std::to_string(bytes_sent) + " --\n\n");
			m_files_sent++;
			// m_pollfd.revents = POLLOUT; 

			return (bytes_sent > 0);
		}

		bool	timeout(t_time now)
		{
			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_activity).count();

			return (diff > CLIENT_TIMEOUT);
		}

		void	setAlive(bool b)
		{
			m_alive = b;
		}

		void	setDisconnectTime(t_time& time)
		{
			m_disconnect_time = time;
		}

		t_time	getDisconnectTime()
		{
			return m_disconnect_time;
		}

};