
#include "Server.hpp"

#include <iostream>
#include <signal.h>
#include <atomic>

std::atomic<bool> is_running(false);

void	handleSigint(int sig)
{
	// graceful close for ctrl + C
	is_running = false;
	exit(0);
}

int	main()
{
	signal(SIGINT, handleSigint);
	Server tcpServer("0.0.0.0", 8080);

	tcpServer.startServer();
	is_running = true;

	while (is_running)
	{
		tcpServer.update();
	}

	tcpServer.closeServer();

	std::cout << "Clean exit" << std::endl;

	return 0;
}