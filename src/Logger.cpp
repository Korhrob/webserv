#include "Logger.hpp"

#include <string>
#include <iostream>
#include <ostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <ctime>

std::ofstream Logger::m_log_file;    // Define static log file stream
std::ofstream Logger::m_log_error;  // Define static error file stream
bool Logger::m_enabled = true;      // Define and initialize m_enabled (default: true)

void	Logger::log(const std::string& msg)
{
	std::time_t	now_time = std::time(nullptr);
	std::tm*	local_time = std::localtime(&now_time);

	if (m_enabled)
		m_log_file << std::put_time(local_time, "[%H:%M:%S] ") << msg << std::endl;
	#ifdef LOG_STDOUT
	std::cout << std::put_time(local_time, "[%H:%M:%S] ") << msg << std::endl;
	#endif
}

void	Logger::logError(const std::string& msg)
{
	std::time_t	now_time = std::time(nullptr);
	std::tm*	local_time = std::localtime(&now_time);

	if (m_enabled)
		m_log_error << std::put_time(local_time, "[%H:%M:%S] ") << msg << std::endl;
	#ifdef LOG_STDOUT
		std::cout << std::put_time(local_time, "[%H:%M:%S] ") << "Error: " << msg << std::endl;
	#endif
}
