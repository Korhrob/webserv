#pragma once

#include <string>
#include <iostream>

static void	log(const std::string& msg)
{
	std::cout << msg << std::endl;
}

static void	logError(const std::string& msg)
{
	std::cout << "Error: " << msg << std::endl;
}