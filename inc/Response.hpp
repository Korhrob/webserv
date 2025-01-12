#pragma once

#include "Client.hpp"
#include "Config.hpp"

#include <memory>
#include <string>
#include <unordered_map>

enum	e_parsing
{
	REQUEST,
	CHUNKED,
	MULTIPART,
	COMPLETE
};

enum	e_method
{
	GET,
	POST,
	DELETE
	//...
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
	std::string			name;
	std::string			filename;
	std::string			contentType;
	std::vector<char>	content;
};

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
		std::string										m_path;
		std::string										m_version;
		std::unordered_map<std::string, std::string>	m_headers;
		formMap											m_queryData;
		std::vector<char>								m_unchunked;
		std::vector<multipart>							m_multipartData;
		e_type											m_send_type;

		void			readRequest(int fd);
		void			parseRequest();
		void			parseRequestLine(std::istringstream& request);
		void			parseHeaders(std::istringstream& request);
		void			validateMethod();
		void			validateVersion();
		void			validateURI();
		void			validateHost();
		void			parseQueryString();
		void			decodeURI(std::string& str);
		bool			headerFound(const std::string& header);
		void			parseMultipart();
		void			parseChunked();
		size_t			getChunkSize(std::string& hex);
		size_t			getContentLength();
		std::string		getBoundary();
		void			ParseMultipartHeaders(std::string& headerString, multipart& part);
		void			validateCgi();
		void			displayQueryData(); // debug
		void			displayMultipart(); // debug
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
