#pragma once

#include <string>
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr

#include <string.h>
#include <sstream>

#include "ILog.hpp"
#include "Packet.hpp"

typedef struct sockaddr_in t_sockaddr_in;

class Server : public ILog
{

	private:
		std::string			m_address;
		int					m_port;
		int					m_socket;
		int					m_new_socket;
		long				m_inc_msg;
		t_sockaddr_in		m_socket_addr;
		t_sockaddr_in		m_client_addr;
		unsigned int		m_addr_len = sizeof(t_sockaddr_in);
		unsigned int		m_client_addr_len = sizeof(t_sockaddr_in);

		std::string			m_server_msg;


		static const size_t	BufferSize;

	public:
		Server(const std::string& ip, int port) 
		{
			m_address = ip;
			m_port = port;
		}
		~Server()
		{
			close(m_socket);
			close(m_new_socket);
		}

		int	startServer()
		{
			log("Starting Server on " + m_address + ":" + std::to_string(m_port) + "...");
			m_socket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_socket <= 0)
			{
				logError("Server failed to open socket!");
				return 0;
			}

			m_socket_addr.sin_family = AF_INET;
			m_socket_addr.sin_addr.s_addr = INADDR_ANY;
			m_socket_addr.sin_port = htons(m_port);

			if (bind(m_socket, (struct sockaddr*)&m_socket_addr, sizeof(m_socket_addr)) < 0)
			{
				logError("Bind failed");
				closeServer();
				return 0;
			}

			log("Server started on " + m_address + ":" + std::to_string(m_port));
			startListen();
			return 1;
		}

		void	closeServer()
		{
			log("Closing server...");
			close(m_socket);
			close(m_new_socket);
			log("Server closed!");
			exit(0);
		}

		void	startListen()
		{
			if (listen(m_socket, 5) < 0)
			{
				logError("Listen failed");
				closeServer();
			}

			log("Server is listenning...");
		}

		// temporary client handling, could and probably should handle in a child process
		// and dont close until client disconnects or timesout;
		// also should make a generic function to receive packets
		void	handleClient(int client_socket)
		{
			char	buffer[1024];
			int		bytes_read = read(client_socket, buffer, 1024);

			if (bytes_read > 8)
			{
				char *ptr = buffer;

				int	id = 0;
				int	len = 0;
				std::string	msg;

				memcpy(&id, ptr, 4);
				ptr += 4;

				memcpy(&len, ptr, 4);
				ptr += 4;

				// check if bytes read totals to header_size + len

				id = ntohl(id);
				len = ntohl(len);

				std::cout << "Msg received: " 
							<< "\n\tID: " << id 
							<< "\n\tLen: " << len
							<< "\n\tMsg: " << std::string(ptr)
							<< std::endl;

				send(client_socket, buffer, bytes_read, 0);
				log("Msg sent back to client\n");
			}
			else
			{
				logError("Received Invalid message");
			}

			close(client_socket);
		}

		void	update()
		{
			m_new_socket = accept(m_socket, (struct sockaddr*)&m_client_addr, &m_client_addr_len);
			if (m_new_socket < 0)
			{
				logError("Accept failed");
				return ;
			}

			log("Client connected!");
			handleClient(m_new_socket);

		}
};