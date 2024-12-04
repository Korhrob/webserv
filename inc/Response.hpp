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
	BLANK,
	SINGLE,
	CHUNK,
	NONE
};

class Response
{
	private:
		std::string										m_request;
		e_method										m_method;
		std::unordered_map<std::string, std::string>	m_headers;
		e_type											m_send_type;
		// int												m_code;
		size_t											m_size;
		std::string										m_path;
		std::string										m_header;
		std::string										m_body;
		bool											m_success;
		bool											m_connection;

	public:
		Response(std::shared_ptr<Client> client);

		bool	readRequest(int fd);
		void	parseRequest();

		bool 	getSuccess() { return m_success; }
		e_type	getSendType() { return m_send_type; }

		std::string	str();
		std::string	header();
		std::string	body();
		std::string	path();
		size_t		size() { return m_size; }

};
