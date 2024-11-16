
#include "Client.hpp"
#include "Packet.hpp"

#include <iostream>
#include <string>

int	main()
{
	Client client("127.0.0.1", 8080);

	if (!client.startClient())
		return 1;

	std::string buffer;
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