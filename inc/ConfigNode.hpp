#pragma once

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>
#include <memory>

class ConfigNode;

using NodeMap = std::map<std::string, std::shared_ptr<ConfigNode>>;
using DirectiveMap = std::map<std::string, std::vector<std::string>>;

class ConfigNode
{
	std::string		m_name; // technically not required
	DirectiveMap	m_directives;
	NodeMap			m_children;

public:
	ConfigNode();
	ConfigNode(const std::string& name);
	~ConfigNode();

	void								addDirective(std::string key, std::vector<std::string> value);
	void								addChild(std::string key, std::shared_ptr<ConfigNode> node);
	const std::vector<std::string>&		findDirective(const std::string& key);
	const std::shared_ptr<ConfigNode>	findNode(const std::string& key);

};