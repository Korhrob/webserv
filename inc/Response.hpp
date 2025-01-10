#pragma once

#include "Client.hpp"
#include "Config.hpp"

#include <memory>
#include <string>
#include <unordered_map>

enum	e_parse
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
		std::vector<char>								m_request;
		std::string										m_method;
		std::string										m_path;
		std::string										m_version;
		int												m_code = 200;
		std::string										m_msg = "OK";
		e_status										m_status;
		std::unordered_map<std::string, std::string>	m_headers;
		std::string										m_header;
		std::string										m_body;
		e_type											m_send_type;
		size_t											m_size;
		formMap											m_queryData;
		e_parse											m_parsing = REQUEST;
		std::vector<char>								m_unchunked;
		std::vector<multipart>							m_multipartData;

		void			readRequest(int fd);
		void			parseRequest();
		void			parseRequestLine(std::istringstream& request);
		void			parseHeaders(std::istringstream& request);
		void			validateMethod();
		void			validateVersion();
		void			validateURI();
		void			validateHost();
		void			validateConnection();
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
