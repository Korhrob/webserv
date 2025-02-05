#pragma once

#include "ConfigNode.hpp"

#include <string>
#include <fstream>
#include <istream>
#include <map>
#include <vector>
#include <memory>

class Config
{

private:
	bool						m_valid;
	NodeMap						m_nodes;
	int							m_server_count;

public:
	Config();
	Config(const std::string& file);
	~Config();

	std::string							trim(const std::string& str);
	bool								parse(std::ifstream& stream);
	bool								isValid();
	std::vector<std::string> 			parseDirective(std::string& line, const int &line_nbr);
	unsigned int						getPort(const std::string& server_name);

	const std::vector<std::string>&		findDirective(const std::string& key);
	const std::shared_ptr<ConfigNode>	findNode(const std::string& key);
	const std::shared_ptr<ConfigNode>	findServerNode(const std::string& host);
	bool								tryGetDirective(const std::string&key, std::vector<std::string>& out);

	int									getServerCount();
	const NodeMap&						getNodeMap();

};