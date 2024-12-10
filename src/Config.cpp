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
	std::string		node_name;
	std::string		line;
	size_t			line_nbr = -1;

	while(std::getline(stream, line))
	{
		line_nbr++;

		// remove comments
		// trim

		if (line.empty())
			continue;

		if (line.back() == '{')
		{
			// parse new node here
			node_name = line.substr(0, line.find('{'));
			node_name = trim(node_name);
			m_nodes[node_name] = ConfigNode();
			//log("new block " + node_name);
			continue;
		}
		else if (line.back() == '}')
		{
			// push node to this node
			//log("end block");
			continue;
		}

		//log("directive " + line);

		std::vector<std::string> directives = parseDirective(line, line_nbr);

		if (directives.empty())
			return false;

		std::string	name = directives[0];
		directives.erase(directives.begin());
		m_nodes[node_name].addDirective(name, directives);

	} 

	log("parsing config done!");

	for (const auto& pair : m_nodes)
	{
		log("-- NODE: [" + pair.first + "] --");
		ConfigNode	n = pair.second;
		for (const auto& a : n.getMap())
		{
			log("key: " + a.first);
			for (const auto& b : a.second)
			{
				log("value: " + b);
			}
		}
	}

	return true;
}

unsigned int	Config::getPort()
{
	if (m_nodes.find("server") == m_nodes.end())
	{
		logError("couldn't find server in config!!");
		return 8080; // default
	}

	ConfigNode	node = m_nodes["server"];

	// if node doesnt contain "listen", return default port

	try
	{
		unsigned int	value = std::stoul(node.getDirective("listen")[0]);
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
		logError("missing ; on line " + std::to_string(line_nbr) + "!");
		return result;
	}
	else
	{
		line = line.substr(0, line.length() - 1);
	}

	// split by separators ' ' and '\t'

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