
#include "Server.hpp"
#include "Logger.hpp"

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
	Server tcpServer("0.0.0.0", 8080);

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

	Logger::getInstance().log("Clean Exit!");

	return 0;
}