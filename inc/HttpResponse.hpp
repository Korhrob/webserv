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
		bool											m_close;
		t_ms											m_timeout;

		std::string	getStatusLine();
		std::string	getHeaders();
		std::string	getBody(const std::string& path);
		
		void	setBody(const std::string& path);
		void	setHeaders();
		
	public:
		HttpResponse(int code, const std::string& msg, const std::string& path, const std::string& targetUrl, bool close, t_ms timeout);
		HttpResponse(const std::string& msg, const std::string& body);

		~HttpResponse() = default;
		HttpResponse&	operator=(const HttpResponse&) = default;

		HttpResponse() = delete;

		// copy constructor is actually usefull now - robert
		HttpResponse(const HttpResponse&) = default;

		std::string	getResponse();
		std::string	getHeader();
		e_type		getSendType();
		bool		getCloseConnection();
};
