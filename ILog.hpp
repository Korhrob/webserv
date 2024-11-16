#pragma once

#include <string>
#include <iostream>

class ILog
{

	public:
		virtual void	log(const std::string& msg)
		{
			std::cout << msg << std::endl;
		}

		virtual void	logError(const std::string& msg)
		{
			std::cout << "Error: " << msg << std::endl;
		}

};