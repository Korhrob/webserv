#pragma once

#include "Client.hpp"
#include "Config.hpp"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>

enum	e_parsing
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
		Config&											m_config;
		e_parsing										m_parsing;
		std::vector<char>								m_request;
		std::string										m_method;
		std::string										m_target;
		std::string										m_path;
		std::unordered_map<std::string, std::string>	m_headers;
		queryMap										m_queryData;
		std::vector<char>								m_unchunked;
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
		// std::ofstream	getFileStream(std::string filename);

	public:
		HttpRequest(Config& config, int fd);
		
};
