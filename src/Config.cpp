#include "Config.hpp"
#include "ConfigNode.hpp"
#include "Const.hpp"
#include "ILog.hpp"

#include <string>
#include <fstream>
#include <ostream>
#include <vector>

Config::Config() : m_valid(false)
{
	std::ifstream	file("config");

	if (!file.good())
		return;

	m_valid = parse(file);
	file.close();

}

Config::~Config()
{
}

bool	Config::parse(std::ifstream& stream)
{
	std::vector<ConfigNode*>	tree;
	ConfigNode*					temp;
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

			temp = new ConfigNode(node_name);

			if (tree.size() > 0)
				tree.back()->addChild(node_name, temp);
			else
				m_nodes[node_name] = temp;

			tree.push_back(temp);
			continue;
		}
		// if not back != '{' and tree.empty, error
		else if (line.back() == '}')
		{
			tree.pop_back();
			continue;
		}

		//log("directive " + line);

		if (tree.size() <= 0)
		{
			logError("directive outside of node on line " + std::to_string(line_nbr) + ":");
			logError(line);
			return false;
		}

		std::vector<std::string> directives = parseDirective(line, line_nbr);

		if (directives.empty())
			return false;

		std::string	name = directives[0];
		directives.erase(directives.begin());
		tree.back()->addDirective(name, directives);

		//log("node [" + tree.back()->getName() + "] : " + name);

	} 

	log("parsing config done: ");
	//log("num base nodes: " + m_nodes.size());

	// really ugly way to print nodes, print entire tree in a clean manner
	for (auto& pair : m_nodes)
	{
		ConfigNode*	n = pair.second;
		logFile("-- NODE: [" + n->getName() + "] --", "configLog");
		logFile("directive count: " + std::to_string(n->directiveCount()), "configLog");
		logFile("child count: " + std::to_string(n->childCount()), "configLog");
		for (auto& a : n->getMap())
		{
			logFile("key: [" + a.first + "]", "configLog");
			for (const auto& b : a.second)
			{
				logFile("value: [" + b + "]", "configLog");
			}
		}
		for (auto& c : n->getChildren())
		{
			ConfigNode* child = c.second;
			logFile("child node: [" + child->getName() + "]", "configLog");
			logFile("directive count: " + std::to_string(child->directiveCount()), "configLog");
			logFile("child count: " + std::to_string(child->childCount()), "configLog");
			for (auto& d : child->getMap())
			{
				logFile("key: [" + d.first + "]", "configLog");
				for (const auto& b : d.second)
				{
					logFile("value: [" + b + "]", "configLog");
				}
			}
		}
		logFile("--", "configLog");
	}

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
	size_t	first = str.find_first_not_of(WHITESPACE);

	if (first == std::string::npos)
		return "";

	size_t	last = str.find_last_not_of(WHITESPACE);

	return str.substr(first, last - first + 1);
}

std::vector<std::string>	Config::findDirective(const std::string& key)
{
	for (const auto& node : m_nodes)
	{
		std::vector<std::string> temp = node.second->findDirective(key);
		if (!temp.empty())
			return temp;
	}
	return std::vector<std::string>();
}