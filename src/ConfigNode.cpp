#include "ConfigNode.hpp"
#include "Const.hpp"
#include "Logger.hpp"
#include "Exception.hpp"

#include <memory>
#include <exception>
#include <string>
#include <algorithm>

ConfigNode::ConfigNode() {};
ConfigNode::ConfigNode(const std::string& name) : m_name(name) {}
ConfigNode::~ConfigNode() {};

void	ConfigNode::addDirective(std::string key, std::vector<std::string>& directives)
{
	auto k = m_directives.find(key);

	if (k != m_directives.end())
		throw ConfigException::duplicateKey(key);

	if (directives.empty())
		throw ConfigException::emptyDirective(key);

	auto it = m_handler.find(key);

	if (it != m_handler.end())
	{
		(this->*it->second)(directives);
	}
	else
	{
		// legacy code, in the future remove find
		auto it = std::find(VALID_DIRECTIVES.begin(), VALID_DIRECTIVES.end(), key);

		if (it == VALID_DIRECTIVES.end())
			throw ConfigException::unknownDirective(key);
	}

	// only happens if no throw occurs
	m_directives[key] = directives;
}

void	ConfigNode::addErrorPage(std::vector<std::string>& value)
{
	if (value.size() < 2)
	{
		Logger::logError("error_page missing code(s) or path");
		return;
	}

	ErrorPage error_page;

	for (auto it = value.begin(); it != value.end() - 1; it++)
	{
		try
		{
			int error_code = std::stoul(*it);

			error_page.m_codes.emplace(error_code);
			// could check if empalce succeeded or we have a duplicate
		}
		catch (const std::invalid_argument& e)
		{
			Logger::logError("invalid error code");
			return;
		}
		catch (const std::out_of_range& e)
		{
			Logger::logError("error code out of range");
			return;
		}
	}

	error_page.m_page = value.back();
	m_error_pages.push_back(error_page);
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

const std::string&	ConfigNode::getErrorPage(int error_code)
{
	for (auto& page : m_error_pages)
	{
		if (page.m_codes.find(error_code) != page.m_codes.end())
			return page.m_page;
	}
	return EMPTY_STRING;
}

void	ConfigNode::handleListen(std::vector<std::string>& directives)
{
	try {
		int	port = std::stoul(directives.front());

		if (port < 0 || port > 65535)
			throw ConfigException::portRange(directives.front());
	}
	catch (std::exception& e)
	{
		throw ConfigException::invalidPort(directives.front());
	}
}

void	ConfigNode::handleMethod(std::vector<std::string>& directives)
{
	for (auto& directive : directives)
	{
		auto it = std::find(VALID_METHODS.begin(), VALID_METHODS.end(), directive);
		if (it == VALID_METHODS.end())
			throw ConfigException::invalidMethod(directive);
	}
}