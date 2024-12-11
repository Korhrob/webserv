#include "ConfigNode.hpp"

ConfigNode::ConfigNode() {};
ConfigNode::ConfigNode(const std::string& name) : m_name(name) {}
ConfigNode::~ConfigNode() {};

void	ConfigNode::addDirective(std::string key, std::vector<std::string> value)
{
	m_directives[key] = value;
}

void	ConfigNode::addChild(std::string key, ConfigNode* node)
{
	m_children[key] = node;
}

std::vector<std::string>	ConfigNode::findDirective(const std::string& key)
{
	if (m_directives.find(key) != m_directives.end())
		return m_directives[key];
	for (const auto& child : m_children)
	{
		std::vector<std::string> temp = child.second->findDirective(key);
		if (!temp.empty())
			return temp;
	}
	return std::vector<std::string>();
}
