
#include "Server.hpp"
#include "Logger.hpp"
#include "Config.hpp"

#include <iostream>
#include <signal.h>
#include <atomic>
#include <exception>

std::atomic<bool> is_running(false);

void	handleSigint(int sig)
{
	(void) sig;
	// graceful close for ctrl + C
	is_running = false;
}

void	program()
{
	Logger::init();

	Server tpc_server;

	// if creation fails return

	is_running = tpc_server.startServer();
	
	while (is_running)
	{
		tpc_server.update();
	}
}

int	main()
{
	signal(SIGINT, handleSigint);
	program();

	Logger::log("Clean Exit!");

	return 0;
}