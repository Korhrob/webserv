#include "ConfigNode.hpp"

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