#pragma once

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>
#include <memory>

//typedef std::shared_ptr<ConfigNode>	t_node_ptr;
typedef	std::map<std::string, std::vector<std::string>>	t_string_map;

class ConfigNode
{
	std::string							m_name; // technically not required
	t_string_map						m_directives;
	std::map<std::string, std::shared_ptr<ConfigNode>>	m_children;

public:
	ConfigNode();
	ConfigNode(const std::string& name);
	~ConfigNode();

	void							addDirective(std::string key, std::vector<std::string> value);
	void							addChild(std::string key, std::shared_ptr<ConfigNode> node);
	const std::vector<std::string>&	findDirective(const std::string& key);

};