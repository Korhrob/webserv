#include "ConfigNode.hpp"
#include "Const.hpp"
#include "Logger.hpp"
#include "ConfigException.hpp"

#include <exception>
#include <string>
#include <algorithm>
#include <limits>
#include <cctype>

ConfigNode::ConfigNode() {};
ConfigNode::ConfigNode(const std::string& name, bool is_server) : m_name(name), m_is_server(is_server)
{
	(void)m_is_server;
}

ConfigNode::~ConfigNode()
{
	for (auto [key, node] : m_children)
	{
		if (node)
			delete node;
	}
};

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
		auto it = std::find(VALID_DIRECTIVES.begin(), VALID_DIRECTIVES.end(), key);

		if (it == VALID_DIRECTIVES.end())
			throw ConfigException::unknownDirective(key);
	}

	m_directives[key] = directives;
}

//	400 /error/400.html
void	ConfigNode::addErrorPage(std::vector<std::string>& directives)
{
	if (directives.size() < 2)
		throw ConfigException::invalidErrorPage();

	std::unordered_set<int>	codes; // 400
	for (std::size_t i = 0; i < (directives.size() - 1); i++)
	{
		int j = std::stoi(directives[i]);
		if (j < 0)
			throw ConfigException::outOfRange("error_page");
		codes.insert(j);
	}

	if (isNumerical(directives.back())) ///error/400.html
		throw ConfigException::invalidErrorPage();

	for (auto& page : m_error_pages)
	{
		for (auto& code : codes)
		{
			//page.m_codes.erase(code);
			if (page.m_codes.count(code))
				throw ConfigException::duplicateErrorPage();
		}

		if (page.m_page == directives.back())
		{
			emplaceCodes(page, codes);
			return;
		}
	}

	ErrorPage error_page {};

	emplaceCodes(error_page, codes);
	error_page.m_page = directives.back();

	m_error_pages.push_back(error_page);

}

void	ConfigNode::addChild(std::string key, ConfigNode* node)
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

ConfigNode*	ConfigNode::findNode(const std::string& key)
{
	auto it = m_children.find(key);

	if (it != m_children.end())
		return it->second;

	for (const auto& child : m_children)
	{
		ConfigNode* temp = child.second->findNode(key);
		if (temp != nullptr)
			return temp;
	}
	return nullptr;
}

ConfigNode*	ConfigNode::findClosestMatch(const std::string& key)
{
	auto it = m_children.find(key);

	if (it != m_children.end())
		return it->second;

	ConfigNode* closest_match = nullptr;
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
	if (m_error_pages.empty())
		return EMPTY_STRING;
	for (auto& page : m_error_pages)
	{
		if (page.m_codes.empty())
			continue;
		if (page.m_codes.count(error_code))
			return page.m_page;
	}
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
		unsigned long	size = std::stoul(directives.front(), &len);

		if (len != directives.front().length())
			throw ConfigException::nonNumerical(directives.front());
		if (size > std::numeric_limits<int>::max())
			throw ConfigException::outOfRange("keepalive_timeout");
	}
	catch (std::exception& e)
	{
		throw ConfigException::outOfRange(directives.front());
	}
}

void	ConfigNode::handleReturn(std::vector<std::string>& directives)
{
	if (directives.size() > 2)
		throw ConfigException::tooManyDirectives("return");
	try
	{
		size_t			len;
		unsigned long	size = std::stoul(directives.front(), &len);

		if (len != directives.front().length() && size > std::numeric_limits<int>::max())
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
			throw ConfigException::duplicateErrorPage();
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

bool	ConfigNode::isNumerical(const std::string& str)
{
	unsigned int i = 0;
	for (const char& c : str)
	{
		if (isdigit(c))
			++i;
	}
	return (i == str.length());
}