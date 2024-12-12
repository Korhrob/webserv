
#include "Server.hpp"
#include "ILog.hpp"

#include <iostream>
#include <signal.h>
#include <atomic>

std::atomic<bool> is_running(false);

void	handleSigint(int sig)
{
	(void) sig;
	// graceful close for ctrl + C
	is_running = false;
}

void	program()
{
	Server tcpServer("0.0.0.0");

	is_running = tcpServer.startServer();

	while (is_running)
	{
		tcpServer.update();
	}

	// tcpServer.closeServer();
}

int	main()
{
	signal(SIGINT, handleSigint);
	program();

	log("Clean Exit!");

	return 0;
}