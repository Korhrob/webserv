
#include "Server.hpp"
#include "Logger.hpp"
#include "Config.hpp"

#include <iostream>
#include <signal.h>
#include <atomic>
#include <exception>
#include <filesystem>

std::atomic<bool> is_running(false);

void	handleSigInt(int sig)
{
	(void) sig;
	is_running = false;
}

void	program(const std::string& path)
{
	Logger::init();
	Server tpc_server(path);

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
		Logger::log("run program with ./server default.conf");
		return 1;
	}

	signal(SIGINT, handleSigInt);

	std::string path = "config.conf";
	if (argc == 2)
		path = std::string(argv[1]);

	if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
		Logger::logError("invalid config file!");
		return 1;
	}

	program(path);

	Logger::log("clean exit!");

	return 0;
}