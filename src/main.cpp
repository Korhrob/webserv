
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
	Server tcpServer("0.0.0.0", 8080);

	g_serv = &tcpServer;
	tcpServer.startServer();

	while (1)
	{
		tcpServer.update();
	}

	tcpServer.closeServer();

	return 0;
}