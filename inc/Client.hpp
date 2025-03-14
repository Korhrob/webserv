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

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;

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

	public:

		Client(int fd) : m_fd(fd)
		{

		}
		~Client() {}

		int		fd() { return m_fd; }

		// can handle all of these in constructor
		bool	connect(int fd, t_time& time)
		{
			// technically not required, can remove
			m_fd = fd;
			m_last_activity = time;
			m_state = ClientState::CONNECTED;
			m_last_response = 0;

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

		bool	timeout(t_time& now)
		{
			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_activity).count();

			return (diff > CLIENT_TIMEOUT);
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

};