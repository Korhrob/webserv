#include "ConfigNode.hpp"
#include "Const.hpp"
#include "Logger.hpp"
#include "ConfigException.hpp"

#include <memory>
#include <exception>
#include <string>
#include <algorithm>
#include <limits>
#include <cctype>

ConfigNode::ConfigNode() {};
ConfigNode::ConfigNode(const std::string& name, bool is_server) : m_name(name), m_is_server(is_server)
{
	(void)m_is_server; // annika
}

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
		// legacy code, in the future throw exception without find
		auto it = std::find(VALID_DIRECTIVES.begin(), VALID_DIRECTIVES.end(), key);

		if (it == VALID_DIRECTIVES.end())
			throw ConfigException::unknownDirective(key);
	}

	// only happens if no throw occurs
	m_directives[key] = directives;
}

void	ConfigNode::addErrorPage(std::vector<std::string>& directives)
{
	if (directives.size() < 2)
		throw ConfigException::invalidErrorPage();

	std::unordered_set<int>	codes;
	for (std::size_t i = 0; i < (directives.size() - 1); i++)
	{
		// stoi is caught from earlier try block
		codes.insert(std::stoi(directives[i]));
	}

	unsigned int i = 0;
	for (char& c : directives.back())
	{
		if (isdigit(c))
			++i;
	}
	if (i == directives.back().length())
		throw ConfigException::invalidErrorPage();

	for (auto& page : m_error_pages)
	{
		// for every existing page, remove the codes we are about to add
		for (auto& code : codes)
			page.m_codes.erase(code);

		if (page.m_page == directives.back())
		{
			emplaceCodes(page, codes);
			return;
		}
	}

	ErrorPage error_page;

	emplaceCodes(error_page, codes);
	error_page.m_page = directives.back();

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
		if (a == index.end() && count > highest)
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

const std::string&	ConfigNode::findErrorPage(int error_code)
{
	for (auto& page : m_error_pages)
	{
		if (page.m_codes.find(error_code) != page.m_codes.end())
			return page.m_page;
	}
	// get default error pages
	return EMPTY_STRING;
}

void	ConfigNode::handleListen(std::vector<std::string>& directives)
{
	try
	{
		size_t			len;
		unsigned long	port = std::stoul(directives.front(), &len);

		if (len != directives.front().length())
			throw ConfigException::nonNumerical(directives.front());
		if (port > 65535)
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

void	ConfigNode::handleAutoIndex(std::vector<std::string>& directives)
{
	if (directives.size() > 1)
		throw ConfigException::tooManyDirectives("autoindex");
	else if (directives.front() == "on")
		m_autoindex = true;
	else if (directives.front() != "off")
		throw ConfigException::emptyDirective("autoindex");
}

void	ConfigNode::handleBodySize(std::vector<std::string>& directives)
{
	if (directives.size() > 1)
		throw ConfigException::tooManyDirectives("client_max_body_size");
	try
	{
		size_t			len;
		unsigned long	size = std::stoul(directives.front(), &len);

		if (len != directives.front().length())
			throw ConfigException::nonNumerical(directives.front());
		if (size > std::numeric_limits<size_t>::max())
			throw ConfigException::outOfRange(directives.front());
	}
	catch (std::exception& e)
	{
		throw ConfigException::outOfRange(directives.front());
	}
}

void	ConfigNode::handleTimeout(std::vector<std::string>& directives)
{
	if (directives.size() > 1)
		throw ConfigException::tooManyDirectives("keepalive_timeout");
	try
	{
		size_t			len;
		unsigned int	size = std::stoul(directives.front(), &len);
		(void)size; // annika

		if (len != directives.front().length())
			throw ConfigException::nonNumerical(directives.front());
	}
	catch (std::exception& e)
	{
		throw ConfigException::outOfRange(directives.front());
	}
}

void	ConfigNode::emplaceCodes(ErrorPage& error_page, std::unordered_set<int>& codes)
{
	for (auto& code : codes)
	{
		auto result = error_page.m_codes.emplace(code);
		if (!(result.second))
		{
			Logger::logError("duplicate error code " + std::to_string(code));
			return;
		}
	}
}

void	ConfigNode::addDefaultDirective(const std::string& key, std::vector<std::string> directives)
{
	std::vector<std::string> temp;

	if (tryGetDirective(key, temp))
		return;

	addDirective(key, directives);
	Logger::log("added default directive " + key);
}

void	ConfigNode::addDefaultErrorPages()
{
	std::unordered_set<int> temp;
	for (auto& ecode : ERROR_CODES)
	{
		if (findErrorPage(ecode) != EMPTY_STRING)
			return;
		temp.insert(ecode);
	}

	ErrorPage error_page;

	emplaceCodes(error_page, temp);
	error_page.m_page = "500.html";
	m_error_pages.push_back(error_page);
}