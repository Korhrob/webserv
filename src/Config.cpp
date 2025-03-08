#include "Config.hpp"
#include "ConfigNode.hpp"
#include "Const.hpp"
#include "Logger.hpp"

#include <string>
#include <fstream>
#include <ostream>
#include <vector>
#include <memory>
#include <algorithm> // std::find

#include <utility> // std::pair

Config::Config() : m_valid(false), m_server_count(0)
{
	std::ifstream	file("config.conf");

	if (!file.is_open() || !file.good())
		return;

	m_valid = parse(file);
	file.close();
}

Config::Config(const std::string& filename) : m_valid(false), m_server_count(0)
{
	std::ifstream	file(filename);

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

			if (node_name == "server")
			{
				if (!tree.empty())
				{
					Logger::logError("unexpected server node on line " + std::to_string(line_nbr) + ":");
					Logger::logError(line);
					return false;
				}
				node_name += "_" + std::to_string(m_server_count++);
			}

			// if node name starts with "location", we treat node as the path
			// ex. "location /" becomes just "/"

			// TODO:
			// should only allow certain name(s) like location or route
			// also improve search for cases like 'qwertyu /location/location'

			if (node_name.find("location") != std::string::npos)
			{
				node_name = node_name.substr(9);
				node_name = trim(node_name);
				//Logger::log("location node = [" + node_name + "]");
			}

			// Test this
			if (!tree.empty() && tree.back()->findNode(node_name) != nullptr)
			{
				Logger::logError("duplicate node " + node_name + " on line " + std::to_string(line_nbr) + ":");
				Logger::logError(line);
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
				Logger::logError("unexpected end of block on line " + std::to_string(line_nbr) + ":");
				Logger::logError(line);
				return false;
			}
			tree.pop_back();
			continue;
		}

		// if not back != '{' and tree.empty, error
		// if back == '}' and tree.empty, error

		if (tree.empty())
		{
			Logger::logError("unexpected directive on line " + std::to_string(line_nbr) + ":");
			Logger::logError(line);
			return false;
		}

		// should while loop until ';'

		std::vector<std::string> directives = parseDirective(line, line_nbr);

		if (directives.empty())
			return false;

		std::string	name = directives[0];

		directives.erase(directives.begin());

		try {
			if (name == "error_page")
				tree.back()->addErrorPage(directives);
			else
				tree.back()->addDirective(name, directives);
		}
		catch (std::exception& e)
		{
			Logger::logError(std::string(e.what()) + " on line " + std::to_string(line_nbr));
			return false;
		}

	}

	if (!tree.empty())
	{
		Logger::logError("unexpected end of file, missing } somewhere");
		return false;
	}

	if (m_server_count == 0)
	{
		Logger::logError("missing server node");
		return false;
	}

	for (auto& [key, node] : m_nodes)
	{
		std::vector<std::string> temp;
		for (auto& str : MANDATORY_DIRECTIVES)
		{
			if (!node->tryGetDirective(str, temp))
			{
				Logger::logError(key + " missing mandatory directive '" + str + "'");
				return false;
			}
		}
	}

	// create default error rules for each server

	return true;
}

std::vector<std::string> 	Config::parseDirective(std::string& line, const int &line_nbr)
{
	std::string	temp = "";
	std::vector<std::string> result;

	if (line.back() != ';')
	{
		Logger::logError("missing ; on line " + std::to_string(line_nbr) + ":");
		Logger::logError(line);
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

int	Config::getServerCount()
{
	return m_server_count;
}

const NodeMap&	Config::getNodeMap()
{
	return m_nodes;
}

/// @brief find server block with host 
/// @param host ex. "127.0.0.1:8080"
/// @return server block or nullptr
const std::shared_ptr<ConfigNode>	Config::findServerNode(const std::string& host)
{
	size_t pos = host.find(":");
	std::string host_name = host.substr(0, pos);
	std::string port = host.substr(pos + 1);

	// should return default or first server_block for a specific port

	for (auto& [key, node] : m_nodes)
	{
		std::vector<std::string> server_port;	

		if (!node->tryGetDirective("listen", server_port)) // no listen directive
			continue;

		auto port_it = std::find(server_port.begin(), server_port.end(), port);

		if (port_it == server_port.end()) // port doesnt match this block
			continue;

		std::vector<std::string> server_name;

		if (!node->tryGetDirective("server_name", server_name)) // no server_name directive
			continue;

		auto it = std::find(server_name.begin(), server_name.end(), host_name);

		if (it == server_name.end()) // server_name doesnt match
			continue;

		return node;
	}

	return m_default_node[std::stoi(port)];
}