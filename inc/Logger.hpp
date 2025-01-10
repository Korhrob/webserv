#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include "Const.hpp"

class Logger {

	private:
		std::ofstream	m_log_file;
		std::ofstream	m_log_error;
		bool			m_enabled;

	public:
		Logger() : m_log_file("logs/log", std::ios::out | std::ios::app), m_log_error("logs/error", std::ios::out | std::ios::app) {
			#ifdef LOG_ENABLE
				m_enabled = LOG_ENABLE;
			#endif
		}
		~Logger() {};

	static Logger&	getInstance() {
		static Logger instance;
		return instance;
	}

	void	log(const std::string& msg);
	void	logError(const std::string& msg);
	
};
