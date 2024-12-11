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
	ConfigNode() {};
	ConfigNode(const std::string& name) : m_name(name) {};
	~ConfigNode() {};

	std::string					getName() { return m_name; }
	t_string_map				getMap() { return m_directives; };
	std::vector<std::string>	getDirective(std::string key) { return (m_directives.find(key) == m_directives.end()) ? std::vector<std::string>() : m_directives[key]; }
	void						addDirective(std::string key, std::vector<std::string> value) { m_directives[key] = value; };
	void						addChild(std::string key, ConfigNode* node) { m_children[key] = node; }
	int							childCount() { return m_children.size(); } // debug
	int							directiveCount() { return m_directives.size(); } // debug
	std::map<std::string, ConfigNode*> getChildren() { return m_children; }

	std::vector<std::string>	findDirective(const std::string& key);


};