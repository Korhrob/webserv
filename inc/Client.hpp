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

class Client
{
	private:
		int				m_fd;
		t_time			m_last_activity;
		//bool			m_close_connection = false;
		t_time			m_disconnect_time;
		ClientState		m_state;
		int				m_last_response;
		t_ms			m_timeout_duration;
		int				m_port;
		HttpRequest		m_request;
		HttpResponse	m_response;


	public:

		Client(int fd) : m_fd(fd)
		{

		}
		~Client() {}

		int	fd() { return m_fd; }

		int	port() { return m_port; }

		// can handle all of these in constructor
		bool	connect(int fd, t_time& time, int port)
		{
			// technically not required, can remove
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
				close(m_fd);

			m_state = ClientState::DISCONNECTED;
			Logger::log("Client fd " + std::to_string(m_fd) + " disconnected!");
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

			m_last_response = send(m_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
			Logger::log("-- BYTES SENT " + std::to_string(m_last_response) + " --\n\n");

			if (m_last_response <= 0)
			{
				m_state = ClientState::DISCONNECTED;
				Logger::log(m_last_response == 0 ? "remote closed connection" : "critical error");
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

		void				handleRequest(Config& config, std::vector<char>& request);
		const HttpResponse&	remoteClosedConnection();
		int					closeConnection();
		e_state				requestState();
		e_type				sendType();
		std::string			response();
		std::string			header();
		std::string			path();
};