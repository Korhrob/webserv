
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
	std::vector<Server> instances;
	Config config("config.conf");

	if (!config.isValid())
		return ;

	NodeMap	nodeMap = config.getNodeMap();
	is_running = false;

	for (auto server_block : nodeMap)
	{
		// introduce try catch block and throw exceptions
		Logger::getInstance().log(server_block.first);
		Server tpc_server(server_block.second);
		is_running = tpc_server.startServer();

		if (!is_running)
			break;
			
		instances.push_back(tpc_server);
	}

	//Logger::getInstance().log("number of instances started " + std::to_string(config.getServerCount()));

	while (is_running)
	{
		for (Server& tpc_server : instances)
			tpc_server.update();
	}

	// for (Server& tpc_server : server_instances)
	// 	tpc_server.closeServer();
}

int	main()
{
	signal(SIGINT, handleSigint);
	program();

	Logger::getInstance().log("Clean Exit!");

	return 0;
}