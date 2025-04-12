#pragma once

#include "HttpRequest.hpp"
#include "ConfigNode.hpp"

#include <string>
#include <unordered_map>

enum	e_type
{
	TYPE_SINGLE,
	TYPE_CHUNKED
};

class HttpResponse {
	private:
		int												m_code;
		std::string										m_msg;
		std::unordered_map<std::string, std::string>	m_headers;
		std::string										m_body;
		e_type											m_type;
		std::string										m_targetUrl;
		int												m_close;
		t_ms											m_timeout;

		void				setBody(const std::string& path);
		const std::string	readBody(const std::string& path);
		const std::string	statusLine();
		const std::string	headers();
		
	public:
		HttpResponse() : m_code(0), m_msg(""), m_body(""),  m_type(TYPE_SINGLE), m_targetUrl(""), m_close(0), m_timeout(0) {};
		HttpResponse(int code, const std::string& msg, const std::string& path, const std::string& targetUrl, int close, t_ms timeout);
		HttpResponse(const std::string& body, t_ms timeout);
		
		~HttpResponse() = default;
		
		HttpResponse&	operator=(const HttpResponse&) = default;
		HttpResponse(const HttpResponse&) = default;
		
		const std::string	response();
		const std::string	header();
		const std::string&	body();
		e_type				sendType();
		int					closeConnection();
		
		void				setHeaders();
		void				appendBody(const std::string& str);
};
