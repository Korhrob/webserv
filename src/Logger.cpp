#include "Logger.hpp"

#include <string>
#include <iostream>
#include <ostream>
#include <fstream>

void	Logger::log(const std::string& msg)
{
	if (m_enabled)
		m_log_file << msg << std::endl;
	#ifdef LOG_STDOUT
	std::cout << msg << std::endl;
	#endif
}

void	Logger::logError(const std::string& msg)
{
	if (m_enabled)
		m_log_error << msg << std::endl;
	#ifdef LOG_STDOUT
		std::cout << "Error: " << msg << std::endl;
	#endif
}
