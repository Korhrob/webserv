#pragma once

#include <string>

class Request
{
	private:
		std::string		m_method;
		std::string		m_target;
		std::string		m_version;

		std::string		m_start; // version, status code, status text
		std::string		m_headers;
		std::string		m_body;

	public:


};