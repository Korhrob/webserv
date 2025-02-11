#pragma once

#include "HttpRequest.hpp"

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
		bool											m_close;

		std::string	getStatusLine();
		std::string	getHeaders();
		std::string	getBody(const std::string& path);
		
		void	setHeaders();
		
	public:
		HttpResponse(int code, const std::string& msg);
		HttpResponse(int code, const std::string& msg, const std::string& path);

		~HttpResponse() = default;
		HttpResponse&	operator=(const HttpResponse&) = default;

		HttpResponse() = delete;
		HttpResponse(const HttpResponse&) = delete;


		std::string	getResponse();
		std::string	getHeader();
		e_type		getSendType();
		bool		closeConnection();
};
