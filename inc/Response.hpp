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

class Response
{
	private:
		std::string										m_request;
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
		// bool											m_connection;

		void			readRequest(int fd);
		void			parseRequest(std::shared_ptr<Client> client, Config& config);
		void			parseRequestLine(std::istringstream& request, Config& config);
		void			parseHeaders(std::istringstream& request, Config& config);
		void			validateMethod();
		void			validateVersion();
		void			validateURI(Config& config);
		void			validateHost(Config& config);
		void			decodeURI();
		// void			parseUrlencoded(std::shared_ptr<Client> client, std::istringstream& body);
		// void			parseMultipart(std::shared_ptr<Client> client, std::istringstream& body);
		bool			headerFound(const std::string& header);
		size_t			getContentLength();
		// void			parseJson(std::shared_ptr<Client> client, std::string body);

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
