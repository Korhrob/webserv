#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <exception>

struct ErrorPage
{
	std::unordered_set<int>	m_codes;
	std::string				m_page;
};

class ConfigNode;

using NodeMap = std::map<std::string, std::shared_ptr<ConfigNode>>;
using DirectiveMap = std::unordered_map<std::string, std::vector<std::string>>;

class ConfigNode : public std::exception
{
	std::string		m_name; // technically not required
	DirectiveMap	m_directives;
	NodeMap			m_children;
	bool			m_autoindex;

	std::vector<ErrorPage>	m_error_pages;

	const std::unordered_map<std::string, void(ConfigNode::*)(std::vector<std::string>& d)> m_handler = {
		{ "listen", &ConfigNode::handleListen },
		{ "method", &ConfigNode::handleMethod },
		{ "autoindex", &ConfigNode::handleAutoIndex }
	};

	void	handleListen(std::vector<std::string>& d);
	void	handleMethod(std::vector<std::string>& d);
	void	handleAutoIndex(std::vector<std::string>& d);
	// ...


public:
	ConfigNode();
	ConfigNode(const std::string& name);
	~ConfigNode();

	void								addDirective(std::string key, std::vector<std::string>& value);
	void								addErrorPage(std::vector<std::string>& value);
	void								addChild(std::string key, std::shared_ptr<ConfigNode> node);
	const std::vector<std::string>&		findDirective(const std::string& key);
	const std::shared_ptr<ConfigNode>	findNode(const std::string& key);
	const std::shared_ptr<ConfigNode>	findClosestMatch(const std::string& key);
	const std::string&					findErrorPage(int error_code);
	bool								tryGetDirective(const std::string&key, std::vector<std::string>& out);
	const std::string&					getName() { return m_name; };
	void								emplaceCodes(ErrorPage& error_page, std::unordered_set<int>& codes);
	bool								autoindexOn() { return m_autoindex; }

};