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
	STATUS_FAIL
};

class Response
{
	private:
		std::string										m_request;
		e_method										m_method;
		std::string										m_path;
		std::string										m_version;
		int												m_code;
		std::string										m_msg = "OK";
		e_status										m_status;
		std::unordered_map<std::string, std::string>	m_headers;
		std::string										m_header;
		std::string										m_body;
		e_type											m_send_type;
		size_t											m_size;
		// bool											m_connection;

		bool		readRequest(int fd);
		void		parseRequest(std::shared_ptr<Client> client, Config& config);
		void		parseRequestLine(std::string requestLine, Config& config);
		void		parseHeaders(std::string str);
		void		validateMethod(std::string method);
		void		validateVersion();
		void		validatePath(Config& config);
		void		normalizePath();
		void		parseUrlencoded(std::shared_ptr<Client> client, std::istringstream& body);
		void		parseMultipart(std::shared_ptr<Client> client, std::istringstream& body);
		void		parseJson(std::shared_ptr<Client> client, std::istringstream& body);

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
