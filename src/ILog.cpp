#include "ILog.hpp"

#include <string>
#include <iostream>
#include <ostream>
#include <fstream>

void	log(const std::string& msg)
{
	std::cout << msg << std::endl;
}

void	logError(const std::string& msg)
{
	std::cout << "Error: " << msg << std::endl;
}

void	logFile(const std::string& msg, const std::string& filename)
{
	std::fstream file("logs/" + filename, std::ios::out | std::ios::app);

	if (file.is_open())
		file << msg << std::endl;

}