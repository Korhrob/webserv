#pragma once

#include "ConfigNode.hpp"

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>

typedef	std::map<std::string, std::vector<std::string>>	t_string_map;

class Config
{

private:
	bool								m_valid;
	std::map<std::string, ConfigNode*>	m_nodes;

public:
	Config();
	~Config();

	std::string					trim(const std::string& str);
	bool						parse(std::ifstream& stream);
	std::vector<std::string> 	parseDirective(std::string& line, const int &line_nbr);
	unsigned int				getPort();
	std::vector<std::string>	findDirective(const std::string& key);
};