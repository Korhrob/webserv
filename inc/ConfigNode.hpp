#pragma once

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>

typedef	std::map<std::string, std::vector<std::string>>	t_string_map;

class ConfigNode
{
	t_string_map						m_directives;
	std::map<std::string, ConfigNode>	m_children;

public:
	ConfigNode() {};
	~ConfigNode() {};

	t_string_map				getMap() { return m_directives; };
	std::vector<std::string>	getDirective(std::string key) { return m_directives[key]; }
	void						addDirective(std::string key, std::vector<std::string> value) { m_directives[key] = value; };

};