
#include "Client.hpp"
#include "Packet.hpp"

#include <iostream>
#include <string>

int	main() // Debug Client!!
{
	Client client("127.0.0.1", 8080);

	if (!client.startClient())
		return 1;

	std::string buffer = "";
	std::cout << "Write something to send to client: " << std::endl;
	while (buffer.empty())
		std::getline(std::cin, buffer);
	client.sendMessage(PacketID::Message, buffer);

	while (client.isAlive())
	{
		client.update();
		usleep(10000);
	}

	client.closeClient();

	return 0;
}