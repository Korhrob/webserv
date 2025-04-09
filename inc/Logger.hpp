#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include "Const.hpp"

class Logger {

	private:
		static std::ofstream	m_log_file;
		static std::ofstream	m_log_error;
		static bool				m_enabled;

	public:
		Logger() {};
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		~Logger() {};

	static void init()
	{
		m_log_file.open("logs/log", std::ios::out | std::ios::app);
		m_log_error.open("logs/error", std::ios::out | std::ios::app);
		#ifdef LOG_ENABLE
			m_enabled = LOG_ENABLE;
		#endif
	}

	static void	log(const std::string& msg);
	static void	logError(const std::string& msg);
	
};
