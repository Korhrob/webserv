#include "Response.hpp"
#include "Parse.hpp"
#include "ILog.hpp"
#include "Const.hpp"
#include "Client.hpp"

#include <memory> // shared_ptr
#include <string>
#include <algorithm> // min
#include <iostream>
#include <sstream>
#include <fstream> // ifstream
#include <filesystem> // filesize
#include <vector> // vector

Response::Response(std::shared_ptr<Client> client)
{
	if (readRequest(client->fd()))
		parseRequest();

	if (m_send_type == NONE)
		return;

	// WRITE HEADER AND BODY

	if (m_size < PACKET_SIZE)
	{
		m_send_type = SINGLE;
		m_body = getBody(m_path);
		m_size = m_body.size();
		m_header = getHeaderSingle(m_size);

		log("== SINGLE RESPONSE ==\n" + str() + "\n\n");
	}
	else
	{
		m_send_type = CHUNK;
		m_header = getHeaderChunk();

		log("== CHUNK RESPONSE ==");
	}

	m_success = true;

}

bool	Response::readRequest(int fd)
{
	// read a bit about max request size for HTTP/1.1
	char	buffer[BUFFER_SIZE];
	//memset(buffer, 0, BUFFER_SIZE);
	size_t	bytes_read = recv(fd, buffer, BUFFER_SIZE, 0);

	log("-- BYTES READ " + std::to_string(bytes_read) + "--\n\n");

	if (bytes_read <= 0)
	{
		logError("Empty or invalid request");
		m_success = false;
		return false;
	}

	buffer[bytes_read] = '\0';
	m_request = std::string(buffer, bytes_read);
	log(buffer);
	return true;
}

void	Response::parseRequest()
{
	// dont read empty requests

	// TODO: Find a better way to parse first line
	size_t pos = m_request.find('\n');

	if (pos == std::string::npos)
	{
		logError("Invalid request header");
		return;
	}

	std::string first_line = m_request.substr(0, pos);
	std::vector<std::string> info;
	size_t start = 0;
	size_t end = first_line.find(' ');

	while (end != std::string::npos) {
		info.push_back(first_line.substr(start, end - start));
		start = end + 1;
		end = first_line.find(' ', start);
	}

	if (info.size() != 2)
	{
		logError("Invalid request header(2)");
		return;
	}

	if (info[0] == "GET")
	{
		m_method = GET;

		// cut the '/' out
		m_path = info[1].substr(1, info[1].size());

		if (m_path.empty())
			m_path = "index.html";

		// test for 'path', 'path.html', 'path/index.html' 
		// what if path is empty
		const std::vector<std::string> alt { ".html", "/index.html" };

		std::ifstream file(m_path);
		if (!file.good())
		{
			for (auto& it : alt)
			{
				std::string	new_path = m_path + it;
				log("Try " + new_path);
				file.clear();
				file = std::ifstream(new_path);
				if (file.good())
				{
					m_path = new_path;
					break;
				}
			}
		}

		try
		{
			m_size = std::filesystem::file_size(m_path);
		}
		catch(const std::exception& e)
		{
			// 404
			m_size = 0;
			std::cerr << e.what() << '\n';
		}

	}
	else 
	{
		// post
		// delete
		m_send_type = NONE;
		logError(info[0] + " not implemented!!");
	}

}

std::string	Response::str()
{
	return m_header + m_body;
}

std::string	Response::body()
{
	return m_body;
}

std::string	Response::header()
{
	return m_header;
}

std::string	Response::path()
{
	return m_path;
}