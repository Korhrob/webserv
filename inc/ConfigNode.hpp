#pragma once

#include <string>
#include <vector>
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

using NodeMap = std::map<std::string, ConfigNode*>;
using DirectiveMap = std::unordered_map<std::string, std::vector<std::string>>;

class ConfigNode : public std::exception
{
	std::string		m_name;
	DirectiveMap	m_directives;
	NodeMap			m_children;
	bool			m_autoindex;
	bool			m_is_server;

	std::vector<ErrorPage>	m_error_pages;

	const std::unordered_map<std::string, void(ConfigNode::*)(std::vector<std::string>& d)> m_handler = {
		{ "listen", &ConfigNode::handleListen },
		{ "method", &ConfigNode::handleMethod },
		{ "autoindex", &ConfigNode::handleAutoIndex },
		{ "client_max_body_size", &ConfigNode::handleBodySize },
		{ "keepalive_timeout", &ConfigNode::handleTimeout },
		{ "return", &ConfigNode::handleReturn },
	};

	void	handleListen(std::vector<std::string>& d);
	void	handleMethod(std::vector<std::string>& d);
	void	handleAutoIndex(std::vector<std::string>& d);
	void	handleBodySize(std::vector<std::string>& d);
	void	handleTimeout(std::vector<std::string>& d);
	void	handleReturn(std::vector<std::string>& d);

public:
	ConfigNode();
	ConfigNode(const std::string& name, bool is_server);
	~ConfigNode();

	void								addDefaultDirective(const std::string& key, std::vector<std::string> directives);
	void								addDirective(std::string key, std::vector<std::string>& value);
	void								addDefaultErrorPages();
	void								addErrorPage(std::vector<std::string>& value);
	void								addChild(std::string key, ConfigNode* node);
	const std::vector<std::string>&		findDirective(const std::string& key);
	ConfigNode*							findNode(const std::string& key);
	ConfigNode*							findClosestMatch(const std::string& key);
	const std::string&					findErrorPage(int error_code);
	bool								tryGetDirective(const std::string&key, std::vector<std::string>& out);
	const std::string&					getName() { return m_name; };
	void								emplaceCodes(ErrorPage& error_page, std::unordered_set<int>& codes);
	bool								autoindexOn() { return m_autoindex; }
	bool								isNumerical(const std::string& str);

};