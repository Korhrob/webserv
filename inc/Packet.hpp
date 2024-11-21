#pragma once

// test stuff

enum PacketID
{
	Connect,
	Message,
	Disconnect
};

// actual class

#include <string>

class Packet
{

	private:
		std::string		m_http_header;
		std::string		m_body;



	public:

		int		sendTo(int socket);

};