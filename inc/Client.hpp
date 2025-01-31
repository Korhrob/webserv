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

struct	multipart {
	std::string				name;
	std::string				filename;
	std::string				contentType;
	std::vector<char>		content;
	std::vector<multipart>	nestedData;
};

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;
typedef std::map<std::string, std::string>		mapStr;
using formMap = std::unordered_map<std::string, std::vector<std::string>>;

// TODO: move inline function to their own .cpp file

class Client
{
	private:
		mapStr											m_env;
		std::string										m_body;
		bool											m_alive;
		struct pollfd&									m_pollfd; // shortcut
		t_sockaddr_in									m_addr;
		unsigned int									m_files_sent;
		t_time											m_last_activity;
		bool											m_close_connection = false;

	public:
		Client(struct pollfd& pollfd) : m_pollfd(pollfd)
		{
			m_pollfd.fd = -1;
			m_pollfd.events = POLLIN | POLLOUT;
			m_pollfd.revents = 0;
			m_alive = false;
		}
		~Client() {}

		bool	isAlive() { return m_alive; }
		bool	incoming() { return m_pollfd.revents & POLLIN; }
		bool	outgoing() { return m_pollfd.revents & POLLOUT; }

		int		fd() { return m_pollfd.fd; }
		struct pollfd& getPollfd() { return m_pollfd; }

		bool	connect(int fd, t_sockaddr_in sock_addr, t_time time)
		{
			// set up non blocking fd
			int flags = fcntl(fd, F_GETFL, 0);
    		fcntl(fd, F_SETFL, flags | O_NONBLOCK);

			// check fcntl errors

			// reinitialize client information
			m_pollfd.fd = fd;
			m_pollfd.events = POLLIN | POLLOUT;
			m_pollfd.revents = 0;
			m_alive = true;

			m_addr = sock_addr;
			m_files_sent = 0;
			m_last_activity = time;

			Logger::getInstance().log("Client connected!" + std::to_string(m_pollfd.fd));

			return true;
		}

		void	disconnect()
		{
			Logger::getInstance().log("Client disconnected!" + std::to_string(m_pollfd.fd));
			close(m_pollfd.fd);
			m_pollfd.fd = -1;
			m_pollfd.revents = 0;
			m_alive = false;

		}

		void	update(t_time time)
		{
			m_last_activity = time;
		}

		int	respond(const std::string& response)
		{
			#ifdef MSG_NOSIGNAL
				int bytes_sent = send(m_pollfd.fd, response.c_str(), response.size(), MSG_NOSIGNAL);
			#else
				int bytes_sent = send(m_pollfd.fd, response.c_str(), response.size(), 0);
			#endif
			Logger::getInstance().log("-- BYTES SENT " + std::to_string(bytes_sent) + "--\n\n");
			m_files_sent++;
			m_pollfd.revents = POLLOUT; 

			return (bytes_sent > 0);
		}

		bool	timeout(t_time now)
		{
			if (m_pollfd.revents & POLLIN)
				return false;

			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_activity).count();

			return (diff > CLIENT_TIMEOUT);
		}

		void	resetEvents()
		{
			m_pollfd.revents = 0;
		}

		void	setCloseConnection()
		{
			m_close_connection = true;
		}

		// Env functions
		void setEnv(std::string envp);
		void setEnvValue(std::string envp, std::string value);
		void unsetEnv();
		mapStr getEnv();
		std::string getEnvValue(std::string envp);
		void populateEnv(std::vector<multipart> info);
		void freeEnv(char** env);
		char** mallocEnv();
		std::vector<char*> createEnv();

		void setBody(std::string string);
		std::string getBody();
};