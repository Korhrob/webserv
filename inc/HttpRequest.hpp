#pragma once

#include "Client.hpp"

#include <vector>
#include <string>
#include <unordered_map>

enum	e_body
{
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
		int												m_fd;
		e_body											m_body;
		std::vector<char>								m_request;
		std::string										m_method;
		std::string										m_target;
		queryMap										m_queryData;
		std::unordered_map<std::string, std::string>	m_headers;
		int												m_unchunked;
		std::vector<multipart>							m_multipartData;
		size_t											m_contentLength;

		void			getBodyType(size_t maxSize);
		void			readRequest();
		void			parseRequestLine(std::istringstream& request);
		void			parseURI();
		void			decodeURI();
		void			parseQueryString();
		void			parseHeaders(std::istringstream& request);
		void			parseChunked(size_t maxSize);
		size_t			getChunkSize(std::string& hex);
		void			parseMultipart(std::string boundary, std::vector<multipart>& multipartData);
		size_t			getContentLength();
		std::string		getBoundary(std::string& contentType);
		void			parseMultipartHeaders(std::string& headerString, multipart& part);
		std::string		uniqueId();

	public:
		HttpRequest(int fd);
		~HttpRequest();

		HttpRequest() = delete;
		HttpRequest(const HttpRequest&) = delete;
		HttpRequest&	operator=(const HttpRequest&) = delete;

		void	parseRequest();
		void	parseBody(size_t maxSize);

		const std::string&				getHost();
		const std::string&				getTarget();
		const std::string&				getMethod();
		const std::vector<multipart>&	getMultipartData();
		bool							getCloseConnection();
};
