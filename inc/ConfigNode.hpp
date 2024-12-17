#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class ConfigNode;

using NodeMap = std::unordered_map<std::string, std::shared_ptr<ConfigNode>>;
using DirectiveMap = std::unordered_map<std::string, std::vector<std::string>>;

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
	bool								tryGetDirective(const std::string&key, std::vector<std::string>& out);

};