#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>

struct ErrorPage
{
	std::unordered_set<int>	m_codes;
	std::string				m_page;
};

class ConfigNode;

using NodeMap = std::map<std::string, std::shared_ptr<ConfigNode>>;
using DirectiveMap = std::unordered_map<std::string, std::vector<std::string>>;

class ConfigNode
{
	std::string		m_name; // technically not required
	DirectiveMap	m_directives;
	NodeMap			m_children;

	std::vector<ErrorPage>	m_error_pages;

public:
	ConfigNode();
	ConfigNode(const std::string& name);
	~ConfigNode();

	void								addDirective(std::string key, std::vector<std::string>& value);
	void								addErrorPage(std::vector<std::string>& value);
	void								addChild(std::string key, std::shared_ptr<ConfigNode> node);
	const std::vector<std::string>&		findDirective(const std::string& key);
	// findDirectiveInChildren;
	const std::shared_ptr<ConfigNode>	findNode(const std::string& key);
	const std::shared_ptr<ConfigNode>	findClosestMatch(const std::string& key);
	const std::string&					getErrorPage(int error_code);
	bool								tryGetDirective(const std::string&key, std::vector<std::string>& out);
	const std::string&					getName() { return m_name; };

	// create exceptions

};