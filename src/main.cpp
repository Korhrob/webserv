
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

void	program(const std::string& path)
{
	Logger::init();
	Server tpc_server(path);

	// if creation fails return

	is_running = tpc_server.startServer();
	
	while (is_running)
	{
		tpc_server.update();
	}
}

int	main(int argc, char** argv)
{
	if (argc > 2)
	{
		Logger::logError("too many arguments");
		Logger::log("run program with ./server [config path]");
		return 1;
	}

	signal(SIGINT, handleSigint);

	std::string path = "config.conf";
	if (argc == 2)
		path = std::string(argv[1]);

	program(path);

	Logger::log("clean exit!");

	return 0;
}