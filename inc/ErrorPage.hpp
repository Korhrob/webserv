#pragma once

#include <string>
#include <unordered_set>

struct ErrorPage
{
	std::unordered_set<int>	m_codes;
	std::string				m_page;
};