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
#include <fstream>
#include <filesystem>
#include <vector>

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
	if (m_request.empty())
		return;

	size_t	pos = m_request.find('\n');

	if (pos == std::string::npos) {
		logError("Invalid request header");
		return;
	}
	{
	std::istringstream	first_line(m_request.substr(0, pos));
	std::string			version;
	std::string			method;

	if (!(first_line >> method >> m_path >> version) || version != "HTTP/1.1") { // what if there's stuff after version?
		logError("");
		return;
	}

	std::unordered_map<std::string, e_method> methods = {{"GET", e_method::GET}, {"POST", e_method::POST}, {"DELETE", e_method::DELETE}};
	if (methods.find(method) != methods.end()) {
		m_method = methods[method];
	} else {
		logError("");
		return;
	}
	}
	std::istringstream	request(m_request.substr(pos + 1));
	std::string			line;

	while (getline(request, line)) {
		pos = line.find(':');
		if (pos == std::string::npos)
			break;
		std::string	key = line.substr(0, pos);
		std::string	value = line.substr(pos + 1); // possibly an issue?
		key.erase(key.find_last_not_of(" \t") + 1);
		value.erase(0, value.find_first_not_of(" \t"));
		m_headers.try_emplace(key, value);
	}

	m_size = m_headers.find("Content-Length") != m_headers.end() ? std::stoul(m_headers["Content-Length"]) : 0;

	m_connection = m_headers.find("Connection") != m_headers.end() && m_headers["Connection"] == "keep-alive" ? true : false;

	m_send_type = m_headers.find("Transfer-Encoding") != m_headers.end() && m_headers["Transfer-Encoding"] == "chunked" ? CHUNK : SINGLE;

	/*
		trim whitespace
		check for content-disposition -> name="name" -> open file with the name
		check for content length/chuncked
		if chuncked set output_filestream for client
		set status code
		make an unordered map for form data (POST)
	*/

	m_path = m_path.substr(1); // do this better might throw exception
	if (m_method == GET) {
		if (m_path.empty())
				m_path = "index.html";

			// test for 'path', 'path.html', 'path/index.html' 
			// what if path is empty
		const std::vector<std::string> alt { ".html", "/index.html" };

		std::ifstream file(m_path);
		if (!file.good()) // && text/html
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

		try {
			m_size = std::filesystem::file_size(m_path);
		} catch (const std::exception& e) {
			m_size = 0;
			std::cerr << e.what() << '\n';
		}
	}

	if (m_method == POST) {
		/*
			depending on Content-Type
				application/x-www-form-urlencoded:
					Parse key-value pairs from the body, similar to URL query parameters.
				multipart/form-data:
					Used for file uploads. You'll need to parse boundary-separated sections.
				application/json:
					Parse the body as JSON using a library or custom parser.
		*/
	}

	if (m_method == DELETE) {
		/*
			validate path
			delete resource identified by the path
		*/
	}
	
	// std::cout << "-- PARSED ---------------------------------------------\n"
	// << method << " " << m_path << " " << version << "\n";
	// for (const auto& [key, value] : m_headers) {
    //     std::cout << key << ":" << value << "\n";
    // }
	// std::cout << "-------------------------------------------------------\n";
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
