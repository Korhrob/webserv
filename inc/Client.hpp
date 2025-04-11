#pragma once

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <poll.h>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <variant>
#include <vector>
#include <map>
#include <sys/epoll.h>
#include <signal.h>

#include "Logger.hpp"
#include "Const.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;
typedef	std::chrono::milliseconds t_ms;

// TODO: move inline function to their own .cpp file
// TODO: add enum for client state

enum ClientState
{
	CONNECTED,
	TIMEDOUT,
	DISCONNECTED
};

enum CGIState
{
	CGI_OPEN,
	CGI_CLOSED
};

enum ChildState
{
	P_RUNNING,
	P_CLOSED
};

class Server;

class Client
{
	private:
		int				m_epollfd;
		int				m_fd;
		Server&			m_server;
		t_time			m_last_activity;
		t_time			m_disconnect_time;
		ClientState		m_state;
		int				m_last_response;
		t_ms			m_timeout_duration;
		int				m_port;
		HttpRequest		m_request;
		HttpResponse	m_response;
		int				m_cgi_pid;
		CGIState		m_cgi_state;
		ChildState		m_child_state;

	public:

		Client() = delete;
		Client(int fd, Server& server) : m_fd(fd), m_server(server), m_state(ClientState::DISCONNECTED), m_cgi_pid(0)
		{

		}
		~Client() {}

		int	fd() { return m_fd; }

		int	port() { return m_port; }

		// can handle all of these in constructor
		bool	connect(int epollfd, int fd, t_time& time, int port)
		{
			m_epollfd = epollfd;
			m_fd = fd;
			m_last_activity = time;
			m_state = ClientState::CONNECTED;
			m_last_response = 0;
			m_port = port;

			Logger::log("Client id " + std::to_string(fd) + ", fd " + std::to_string(m_fd) + " connected!");

			return true;
		}

		void	disconnect()
		{
			if (m_state == ClientState::DISCONNECTED)
				return;

			if (m_fd >= 0)
			{
				epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_fd, NULL);
				close(m_fd);
			}

			if (m_cgi_pid > 0)
			{
				kill(m_cgi_pid, SIGTERM);
				m_cgi_pid = 0;
			}

			m_state = ClientState::DISCONNECTED;
			//Logger::log("Client fd " + std::to_string(m_fd) + " disconnected!");
		}

		void	update(t_time& time)
		{
			m_last_activity = time;
			m_state = ClientState::CONNECTED;
		}

		int	respond(const std::string& response)
		{
			if (m_state != ClientState::CONNECTED)
				return 0;

			// if (m_state == ClientState::DISCONNECTED)
			// 	return 0;

			m_last_response = send(m_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
			Logger::log("-- BYTES SENT " + std::to_string(m_last_response) + " --\n\n");

			if (m_last_response <= 0)
			{
				m_state = ClientState::DISCONNECTED;
				Logger::logError(m_last_response == 0 ? "remote closed connection" : "critical error");
			}

			return m_last_response;
		}

		int respondChunked(const std::string& target)
		{
			std::ifstream file;
			try
			{
				file = std::ifstream(target, std::ios::binary);

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
					respond(oss.str());
			
					if (m_last_response <= 0)
						break;
				}
				respond("0\r\n\r\n");
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
				m_last_response = -1;
			}
			
			return m_last_response;
		}

		bool	timeout(const t_time& now)
		{
			return ((now - m_last_activity) > m_timeout_duration);
		}

		void	setTimeoutDuration(unsigned long seconds)
		{
			m_timeout_duration = t_ms(seconds * 1000);
		}

		t_ms	getTimeoutDuration()
		{
			return m_timeout_duration;
		}

		ClientState	getClientState() { return m_state; }
		void	setClientState(ClientState state) { m_state = state; } 

		void	setDisconnectTime(t_time& time)
		{
			m_disconnect_time = time;
		}

		t_time	getDisconnectTime()
		{
			return m_disconnect_time;
		}

		int	getResponseState()
		{
			return m_last_response;
		}

		int		getCPID()
		{
			return m_cgi_pid;
		}

		void	resetCPID()
		{
			m_cgi_pid = 0;
		}

		void	setCGIState(CGIState state)
		{
			m_cgi_state = state;
		}

		CGIState getCGIState()
		{
			return m_cgi_state;
		}

		void	setChildState(ChildState state)
		{
			m_child_state = state;
		}

		ChildState getChildState()
		{
			return m_child_state;
		}

		void				handleRequest(Config& config, std::vector<char>& request);
		void				cgiResponse();
		void				appendResponseBody(const std::string& str);
		int					closeConnection();
		e_state				requestState();
		e_type				sendType();
		std::string			response();
		std::string			header();
		std::string			path();
};