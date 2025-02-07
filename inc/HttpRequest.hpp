#pragma once

#include "Client.hpp"

#include <vector>
#include <string>
#include <unordered_map>

enum	e_phase
{
	REQUEST,
	CHUNKED,
	MULTIPART,
	COMPLETE
};

struct	multipart {
	std::string				name;
	std::string				filename;
	std::string				contentType;
	std::vector<char>		content;
	std::vector<multipart>	nestedData;
};

using queryMap = std::unordered_map<std::string, std::vector<std::string>>;

class HttpRequest {
	private:
		e_phase											m_phase;
		std::vector<char>								m_request;
		std::string										m_method;
		std::string										m_target;
		queryMap										m_queryData;
		std::unordered_map<std::string, std::string>	m_headers;
		std::ofstream									m_unchunked;
		std::vector<multipart>							m_multipartData;

		void			readRequest(int fd);
		void			parseRequest();
		void			parseRequestLine(std::istringstream& request);
		void			parseURI();
		void			decodeURI();
		void			parseQueryString();
		void			parseHeaders(std::istringstream& request);
		void			parseBody(std::vector<char>::iterator endOfHeaders);
		void			parseChunked();
		size_t			getChunkSize(std::string& hex);
		void			parseMultipart(std::string boundary, std::vector<multipart>& multipartData);
		size_t			getContentLength();
		std::string		getBoundary(std::string& contentType);
		void			ParseMultipartHeaders(std::string& headerString, multipart& part);

	public:
		HttpRequest();
		~HttpRequest();

		HttpRequest(const HttpRequest&) = delete;
		HttpRequest&	operator=(const HttpRequest&) = delete;

		void	getRequest(int fd);

		const std::string&				getHost();
		const std::string&				getTarget();
		const std::string&				getMethod();
		const std::vector<multipart>&	getMultipartData();
};
