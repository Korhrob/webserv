
#include "Server.hpp"

#include <iostream>

int	main()
{
	Server tcpServer("127.0.0.1", 8080);

	tcpServer.startServer();

	while (1)
	{
		tcpServer.update();
		usleep(10000);

		if (std::cin.eof())
			break;
	}

	tcpServer.closeServer();

	return 0;
}