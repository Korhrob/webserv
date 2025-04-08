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

		void		setBody(const std::string& path);
		void		setHeaders();
		std::string	readBody(const std::string& path);
		std::string	statusLine();
		std::string	headers();
		
	public:
		HttpResponse(int code, const std::string& msg, const std::string& path, const std::string& targetUrl, int close, t_ms timeout);
		HttpResponse(const std::string& body, t_ms td);

		HttpResponse() = default;
		~HttpResponse() = default;

		HttpResponse&	operator=(const HttpResponse&) = default;
		HttpResponse(const HttpResponse&) = default;

		std::string	response();
		std::string	header();
		e_type		sendType();
		int			closeConnection();
};
