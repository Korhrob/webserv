#pragma once

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>

typedef	std::map<std::string, std::vector<std::string>>	t_string_map;

class ConfigNode
{
	std::string							m_name;
	t_string_map						m_directives;
	std::map<std::string, ConfigNode*>	m_children;

public:
	ConfigNode();
	ConfigNode(const std::string& name);
	~ConfigNode();

	void						addDirective(std::string key, std::vector<std::string> value);
	void						addChild(std::string key, ConfigNode* node);
	std::vector<std::string>	findDirective(const std::string& key);

};