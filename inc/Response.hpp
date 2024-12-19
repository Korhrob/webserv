#pragma once

#include "Client.hpp"
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
		std::string										m_path = "/index.html";
		std::string										m_version;
		int												m_code;
		e_status										m_status;
		std::unordered_map<std::string, std::string>	m_headers;
		std::string										m_header;
		std::string										m_body;
		e_type											m_send_type;
		size_t											m_size;
		// bool											m_connection;

	public:
		Response(std::shared_ptr<Client> client);

		bool	readRequest(int fd);
		void	parseRequest(std::shared_ptr<Client> client);
		void	parseMultipart(std::shared_ptr<Client> client, std::istringstream& body);

		bool		setMethod(std::string method);

		e_status	getStatus() { return m_status; }
		e_type		getSendType() { return m_send_type; }

		bool		version() { return m_version == "HTTP/1.1"; }

		std::string	str();
		std::string	header();
		std::string	body();
		std::string	path();
		size_t		size() { return m_size; }

};
