#pragma once

#include <string>
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr

#include <string.h> // strlen

#include <sstream>
#include "ILog.hpp" // log,  logError
#include "Packet.hpp" // PacketID

typedef struct sockaddr_in t_sockaddr_in;

class Client
{

	private:
		std::string			m_address;
		int					m_port;
		int					m_socket;
		t_sockaddr_in		m_server_addr;
		unsigned int		m_server_addr_len = sizeof(t_sockaddr_in);

		bool				m_alive;

	public:
		Client(std::string ip, int port)
		{
			m_address = ip;
			m_port = port;
		}
		~Client()
		{
			close(m_socket);
		}

		bool	isAlive() { return m_alive; }

		int	startClient()
		{
			log("Starting  Client...");
			m_socket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_socket <= 0)
			{
				logError("Client failed to open socket!");
				return 0;
			}

			m_server_addr.sin_family = AF_INET;
			m_server_addr.sin_port = htons(m_port);

			if (inet_pton(AF_INET, m_address.c_str(), &m_server_addr.sin_addr) <= 0)
			{
				logError("Invalid address");
				close(m_socket);
				return 0;
			}

			if (connect(m_socket, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr)) < 0)
			{
				logError("Connection to server failed");
				closeClient(); // could retry a few times
				return 0;
			}

			log("Connected to server!");
			m_alive = true;

			return 1;
		}

		void	update()
		{
			if (!m_alive)
				return;

			char	buffer[1024];
			int		bytes_read = read(m_socket, buffer, 1024);

			if (bytes_read < 0)
			{
				logError("Invalid Package");
				closeClient();
				return;
			}

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

				// check if bytes_read totals to header_size + len

				id = ntohl(id);
				len = ntohl(len);

				std::cout << "Msg received: " 
							<< "\n\tID: " << id 
							<< "\n\tLen: " << len
							<< "\n\tMsg: " << std::string(ptr)
							<< std::endl;

			}

			// temporary kill
			m_alive = false;

		}

		void	closeClient()
		{
			log("Closing client...");
			m_alive = false;
			close(m_socket);
			log("Client closed!");
		}
		
		// should make a generic method to send packets
		void	sendMessage(PacketID packet_id, const std::string& message)
		{
			if (!m_alive)
				return;

			const int	id_size = 4;
			const int	len_size = 4;
			int			header_size = id_size + len_size; // id + len
			int			msg_size = message.size() + 1;

			// note: not very safe, should probably use a const buffer
			char *packet = new char[header_size + msg_size];

			// write id to packet
			int	id = htonl(packet_id);
			memcpy(packet, &id, id_size);
			std::cout << "wrote id " << packet_id << " to packet (size " << id_size << ")" << std::endl;

			// write msg length to packet
			int	len = htonl(msg_size);
			memcpy(packet + id_size, &len, len_size);
			std::cout << "wrote len " << msg_size << " to packet (size " << len_size << ")" << std::endl;

			// write content to packet
			memcpy(packet + header_size, message.c_str(), msg_size);
			std::cout << "wrote msg " << message.c_str() << " to packet  (size " << msg_size << ")" << std::endl;

			send(m_socket, packet, header_size + msg_size, 0);
			log("sent message to " + m_address + ":" + std::to_string(m_port));

			// check if send returns < 0 (fail)

			delete[] packet; // see note on declaration
		}
};