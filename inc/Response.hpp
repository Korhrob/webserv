#pragma once

#include "Client.hpp"
#include "Config.hpp"

#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

enum	e_parsing
{
	REQUEST,
	CHUNKED,
	MULTIPART,
	COMPLETE
};

enum	e_type
{
	TYPE_BLANK,
	TYPE_SINGLE,
	TYPE_CHUNK,
	TYPE_NONE
};

enum	e_status
{
	STATUS_BLANK,
	STATUS_OK,
	STATUS_FAIL,
	STATUS_CGI
};

struct	multipart {
	std::string				name;
	std::string				filename;
	std::string				contentType;
	std::vector<char>		content;
	std::vector<multipart>	nestedData;
};

using queryMap = std::unordered_map<std::string, std::vector<std::string>>;

class Response
{
	private:
		std::shared_ptr<Client>							m_client;
		Config&											m_config;
		e_parsing										m_parsing;
		int												m_code;
		std::string										m_msg;
		e_status										m_status;
		std::string										m_header;
		std::string										m_body;
		size_t											m_size;
		std::vector<char>								m_request;
		std::string										m_method;
		std::string										m_target;
		std::string										m_path;
		std::string										m_version;
		std::unordered_map<std::string, std::string>	m_headers;
		queryMap										m_queryData;
		std::vector<char>								m_unchunked;
		std::vector<multipart>							m_multipartData;
		e_type											m_send_type;

		void			readRequest(int fd);
		void			parseRequest();
		void			parseRequestLine(std::istringstream& request);
		void			validateVersion();
		void			validateMethod();
		void			validateURI();
		void			decodeURI();
		void			parseQueryString();
		void			validateCgi();
		void			parseHeaders(std::istringstream& request);
		void			validateHost();
		void			handleGet();
		void			handlePost(std::vector<char>::iterator& endOfHeaders);
		void			handleDelete();
		void			parseChunked();
		size_t			getChunkSize(std::string& hex);
		void			parseMultipart(std::string boundary, std::vector<multipart>& multipartData);
		size_t			getContentLength();
		std::string		getBoundary(std::string& contentType);
		void			ParseMultipartHeaders(std::string& headerString, multipart& part);
		std::ofstream	getFileStream(std::string filename);
		bool			headerFound(const std::string& header);
		void			getDirective(std::string dir, std::vector<std::string>& out);
		void			displayQueryData(); // debug
		void			displayMultipart(std::vector<multipart>& multipartData); // debug
		// void			parseUrlencoded(std::shared_ptr<Client> client, std::istringstream& body);

	public:
		Response(std::shared_ptr<Client> client, Config& config);

		e_status	getStatus() { return m_status; }
		e_type		getSendType() { return m_send_type; }

		std::string	str();
		std::string	header();
		std::string	body();
		std::string	path();
		size_t		size();
};