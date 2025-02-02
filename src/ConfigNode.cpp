#include "ConfigNode.hpp"
#include "Const.hpp"

#include <memory>

ConfigNode::ConfigNode() {};
ConfigNode::ConfigNode(const std::string& name) : m_name(name) {}
ConfigNode::~ConfigNode() {};

void	ConfigNode::addDirective(std::string key, std::vector<std::string> value)
{
	m_directives[key] = value;
}

void	ConfigNode::addChild(std::string key, std::shared_ptr<ConfigNode> node)
{
	m_children[key] = node;
}

const std::vector<std::string>&	ConfigNode::findDirective(const std::string& key)
{
	auto it = m_directives.find(key);
	if (it != m_directives.end())
		return it->second;

	// if (m_directives.find(key) != m_directives.end())
	// 	return m_directives.at(key);

	for (const auto& child : m_children)
	{
		const std::vector<std::string>& temp = child.second->findDirective(key);
		if (!temp.empty())
			return temp;
	}
	return EMPTY_VECTOR;
}

const std::shared_ptr<ConfigNode>	ConfigNode::findNode(const std::string& key)
{
	auto it = m_children.find(key);
	if (it != m_children.end())
		return it->second;

	for (const auto& child : m_children)
	{
		const std::shared_ptr<ConfigNode> temp = child.second->findNode(key);
		if (temp != nullptr)
			return temp;
	}
	return nullptr;
}

const	std::shared_ptr<ConfigNode>	ConfigNode::findClosestMatch(const std::string& key)
{
	auto it = m_children.find(key);
	if (it != m_children.end())
		return it->second;

	std::shared_ptr<ConfigNode> closest_match = nullptr;
	size_t highest = 0;

	for (auto& [index, child] : m_children)
	{
		if (index.size() > key.size())
			continue;
		
		auto a = index.begin();
		auto b = key.begin();
		size_t count = 0;
	
		while (a != index.end() && b != key.end() && *a == *b)
		{
			a++;
			b++;
			count++;
		}
		if (count > highest)
		{
			closest_match = child;
			highest = count;
		}
	}
	return closest_match;
}

bool	ConfigNode::tryGetDirective(const std::string&key, std::vector<std::string>& out)
{
	out = findDirective(key);
	return !out.empty();
}