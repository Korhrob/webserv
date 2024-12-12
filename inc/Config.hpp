#pragma once

#include "ConfigNode.hpp"

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>
#include <memory>

typedef	std::map<std::string, std::vector<std::string>>	t_string_map;

class Config
{

private:
	bool												m_valid;
	std::map<std::string, std::shared_ptr<ConfigNode>>	m_nodes;

	// quick acces to often used directives

public:
	Config();
	~Config();

	std::string					trim(const std::string& str);
	bool						parse(std::ifstream& stream);
	bool						isValid();
	std::vector<std::string> 	parseDirective(std::string& line, const int &line_nbr);
	unsigned int				getPort();
	const std::vector<std::string>&	findDirective(const std::string& key);

};