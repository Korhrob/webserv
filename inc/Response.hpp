#pragma once

#include "Client.hpp"
#include <memory>
#include <string>

enum	e_method
{
	GET,
	POST
	//DELETE
	//...
};

enum	e_type
{
	NONE,
	SINGLE,
	CHUNK
};

class Response
{
	private:
		std::string		m_request;
		e_method		m_method;
		e_type			m_send_type;
		int				m_code;
		size_t			m_size;
		std::string		m_path;
		std::string		m_header;
		std::string		m_body;
		bool			m_success;

	public:
		Response(std::shared_ptr<Client> client);

		void	readRequest(int fd);
		void	parseRequest();

		bool 	getSuccess() { return m_success; }
		e_type	getSendType() { return m_send_type; }

		std::string	str();
		std::string	header();
		std::string	body();
		std::string	path();
		size_t		size() { return m_size; }

};