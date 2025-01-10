#pragma once

#include "Client.hpp"
#include "Config.hpp"

#include <memory>
#include <string>
#include <unordered_map>

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
		std::vector<char>								m_request;
		// e_method										m_method;
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
		// bool											m_connection;
		Config&											m_config;
		bool											m_complete = false;
		std::vector<char>								m_content;
		std::vector<multipart>							m_multipartData;
		bool											m_isCgi = false;

		void			readRequest(int fd);
		void			parseRequest();
		void			parseRequestLine(std::istringstream& request);
		void			parseHeaders(std::istringstream& request);
		void			validateMethod();
		void			validateVersion();
		void			validateURI();
		void			validateHost();
		void			parseQueryString();
		void			decode(std::string& str);
		void			parseUrlencoded(std::shared_ptr<Client> client, std::istringstream& body);
		bool			headerFound(const std::string& header);
		void			parseMultipart();
		void			unchunkContent();
		int				getChunkSize(std::string& hex);
		size_t			getContentLength();
		void			displayQueryData(); // debug
		std::string		getBoundary();
		void			ParseMultipartHeaders(std::string& headerString, multipart& part);
		void			displayMultipart();
		void			validateCgi();

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
