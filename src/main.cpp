
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
	std::vector<std::shared_ptr<Server>> instances;
	Config config("config.conf");

	if (!config.isValid())
		return ;

	Logger::init();

	NodeMap	nodeMap = config.getNodeMap();
	//is_running = false;

	// is_running = tpc_server.startServer();

	for (auto server_block : nodeMap)
	{
		// introduce try catch block and throw exceptions
		std::shared_ptr<Server> tpc_server = std::make_shared<Server>(server_block.second);
		is_running = tpc_server->startServer();

		if (!is_running)
			break;
			
		instances.push_back(tpc_server);
	}

	//Logger::log("number of instances started " + std::to_string(config.getServerCount()));

	while (is_running)
	{
		for (std::shared_ptr<Server> tpc_server : instances)
			tpc_server->update();
	}

	// for (Server& tpc_server : server_instances)
	// 	tpc_server.closeServer();
}

int	main()
{
	signal(SIGINT, handleSigint);
	program();

	Logger::log("Clean Exit!");

	return 0;
}