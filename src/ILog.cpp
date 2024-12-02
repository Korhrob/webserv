#include "ILog.hpp"

#include <string>
#include <iostream>

void	log(const std::string& msg)
{
	std::cout << msg << std::endl;
}

void	logError(const std::string& msg)
{
	std::cout << "Error: " << msg << std::endl;
}