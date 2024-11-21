
#include "Server.hpp"

#include <iostream>
#include <signal.h>

static Server* g_serv;

void	handleSigint(int sig)
{
	// graceful close for ctrl + C
	g_serv->closeServer();
	exit(0);
}

int	main()
{
	signal(SIGINT, handleSigint);
	Server tcpServer("127.0.0.1", 8080);

	g_serv = &tcpServer;
	tcpServer.startServer();

	while (1)
	{
		tcpServer.update();
		usleep(10000);
	}

	tcpServer.closeServer();

	return 0;
}