#include "Config.hpp"
#include "ConfigNode.hpp"
#include "Const.hpp"
#include "ILog.hpp"

#include <string>
#include <fstream>
#include <ostream>
#include <vector>
#include <memory>
#include <algorithm> // std::find

Config::Config() : m_valid(false)
{
	std::ifstream	file("config.conf");

	if (!file.is_open() || !file.good())
		return;

	m_valid = parse(file);
	file.close();

}

Config::~Config()
{
}

bool	Config::isValid()
{
	return m_valid;
}

bool	Config::parse(std::ifstream& stream)
{
	std::vector<std::shared_ptr<ConfigNode>>	tree;
	std::shared_ptr<ConfigNode>	temp;
	std::string					node_name;
	std::string					line;
	size_t						line_nbr = 0;

	while(std::getline(stream, line))
	{
		line_nbr++;

		size_t	comment = line.find("#");

		if (comment != std::string::npos) 
			line = line.substr(0, comment);

		line = trim(line);

		if (line.empty())
			continue;

		if (line.back() == '{')
		{
			node_name = line.substr(0, line.find('{'));
			node_name = trim(node_name);

			// if node name starts with "location", we treat node as the path
			// ex. "location /" becomes "/"
			if (node_name.find("location") != std::string::npos)
			{
				node_name = node_name.substr(9);
				node_name = trim(node_name);
				log("location node = [" + node_name + "]");
			}

			if (findNode(node_name) != nullptr)
			{
				logError("duplicate node " + node_name + " on line " + std::to_string(line_nbr) + ":");
				logError(line);
				return false;
			}

			temp = std::make_shared<ConfigNode>(node_name);

			if (tree.size() > 0)
				tree.back()->addChild(node_name, temp);
			else
				m_nodes[node_name] = temp;

			tree.push_back(temp);
			continue;
		}
		else if (line.back() == '}')
		{
			if (tree.empty())
			{
				logError("unexpected end of block on line " + std::to_string(line_nbr) + ":");
				logError(line);
				return false;
			}
			tree.pop_back();
			continue;
		}
		// if not back != '{' and tree.empty, error
		// if back == '}' and tree.empty, error

		if (tree.empty())
		{
			logError("unexpected directive on line " + std::to_string(line_nbr) + ":");
			logError(line);
			return false;
		}

		// should while loop until ';'

		std::vector<std::string> directives = parseDirective(line, line_nbr);

		if (directives.empty())
			return false;

		std::string	name = directives[0];

		auto it = std::find(VALID_DIRECTIVES.begin(), VALID_DIRECTIVES.end(), name);

		if (it == VALID_DIRECTIVES.end())
		{
			logError(name + " is not a valid directive, line " + std::to_string(line_nbr) + ":");
			logError(line);
			return false;
		}

		if (!tree.back()->findDirective(name).empty())
		{
			logError("duplicate directive " + name + " on line " + std::to_string(line_nbr) + ":");
			logError(line);
			return false;
		}

		directives.erase(directives.begin());
		tree.back()->addDirective(name, directives);

	}

	if (!tree.empty())
	{
		logError("unexpected end of file, missing } somewhere");
		return false;
	}

	std::shared_ptr<ConfigNode>	serverNode = findNode("server");
	if (serverNode == nullptr)
	{
		logError("missing server node");
		return false;
	}

	std::vector<std::string> d;
	for (auto& str : MANDATORY_DIRECTIVES)
	{
		if (!serverNode->tryGetDirective(str, d))
		{
			logError("missing mandatory directive '" + str + "'");
			return false;
		}
	}

	//log("Config OK");

	// some test functions
	// if (findNode("/") != nullptr)
	// {
	// 	log("location / (root) found!");
		
	// }

	// std::vector<std::string> t;

	// if (tryGetDirective("root", t)) {
	// 	log("found root");
	// }

	return true;
}

unsigned int	Config::getPort()
{
	std::vector<std::string> directive = findDirective("listen");

	if (directive.empty())
	{
		logError("server block does not contain listen directive");
		return 8080; // default
	}

	try
	{
		unsigned int	value = std::stoul(directive.front());
		return value;
	}
	catch (const std::invalid_argument& e)
	{
		logError("Invalid argument");
	}
	catch (const std::out_of_range& e)
	{
		logError("Out of range");
	}

	logError("using default port 8080");
	return 8080; // default
}

std::vector<std::string> 	Config::parseDirective(std::string& line, const int &line_nbr)
{
	std::string	temp = "";
	std::vector<std::string> result;

	if (line.back() != ';')
	{
		logError("missing ; on line " + std::to_string(line_nbr) + ":");
		logError(line);
		return result;
	}
	else
	{
		line = line.substr(0, line.length() - 1);
	}

	// split
	for (char& ch : line)
	{
		if (WHITESPACE.find(ch) != std::string::npos)
		{
			if (!temp.empty())
			{
				result.push_back(temp);
				temp.clear();
			}
		}
		else
		{
			temp += ch;
		}
	}

	if (!temp.empty())
		result.push_back(temp);
	
	return result;
}

std::string	Config::trim(const std::string& str)
{
	if (str.empty())
		return str;
	
	size_t	first = str.find_first_not_of(WHITESPACE);

	if (first == std::string::npos)
		return "";

	size_t	last = str.find_last_not_of(WHITESPACE);

	return str.substr(first, last - first + 1);
}

const std::vector<std::string>&	Config::findDirective(const std::string& key)
{
	for (const auto& node : m_nodes)
	{
		const std::vector<std::string>& temp = node.second->findDirective(key);
		if (!temp.empty())
			return temp;
	}
	return EMPTY_VECTOR;
}

const std::shared_ptr<ConfigNode>	Config::findNode(const std::string& key)
{
	if (m_nodes.find(key) != m_nodes.end())
		return m_nodes[key];
	for (const auto& node : m_nodes)
	{
		const std::shared_ptr<ConfigNode> temp = node.second->findNode(key);
		if (temp != nullptr)
			return temp;
	}
	return nullptr;
}

/// @brief assign directive[key] to out and return true if it isn't empty
/// @param key directive key
/// @param out output directive
/// @return true or false
bool	Config::tryGetDirective(const std::string&key, std::vector<std::string>& out)
{
	out = findDirective(key);
	if (!out.empty())
		return true;
	return false;
}