#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <signal.h>

#include "Client.hpp"

class CGI
{
	private:

	public:
		CGI();
		~CGI();
		int runCGI(std::string script, Client client);

};

