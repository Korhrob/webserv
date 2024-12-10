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
		parseRequest(client);

	client->displayFormData(); // for debugging

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

void	parseMultipart(std::string request, std::shared_ptr<Client> client)
{
	(void)client;
	std::cout << "\n\nMULTIPART PARSING\n";
	std::cout << request << "\n\n";


}

void	Response::parseRequest(std::shared_ptr<Client> client)
{
	size_t	pos = m_request.find('\n');
	if (pos == std::string::npos || pos == std::string::npos - 1) {
		logError("Invalid request");
		return;
	}
	// std::string firstLine = m_request.substr(0, pos);
	// firstLine.erase(firstLine.find_last_not_of("\r") + 1);
	// if (firstLine == client->getBoundary()) {
	// 	parseMultipart(m_request, client);
	// 	return;
	// }
	{
	std::istringstream	request_line(m_request.substr(0, pos));
	std::string			version;
	std::string			method;
	std::string			rest;

	if (!(request_line >> method >> m_path >> version) || request_line >> rest || version != "HTTP/1.1") {
		logError("Invalid request line");
		return;
	}

	std::unordered_map<std::string, e_method> methods = {{"GET", e_method::GET}, {"POST", e_method::POST}, {"DELETE", e_method::DELETE}};
	if (methods.find(method) != methods.end()) {
		m_method = methods[method];
	} else {
		logError("Invalid or missing method");
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
		std::string	value(pos + 1 != std::string::npos ? line.substr(pos + 1) : ""); // should this be an error?
		key.erase(key.find_last_not_of(" \t") + 1);
		std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::toupper(c); });
		std::replace(key.begin(), key.end(), '-', '_');
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of("\r") + 1);
		m_headers.try_emplace(key, value);
	}

	if (m_headers.find("CONNECTION") != m_headers.end())
		m_connection = m_headers["CONNECTION"] == "keep-alive" ? true : false;

	if (m_headers.find("TRANSFER_ENCODING") != m_headers.end())
		m_send_type = m_headers["TRANSFER_ENCODING"] == "chunked" ? CHUNK : SINGLE;

	m_size = m_headers.find("CONTENT_LENGTH") != m_headers.end() ? std::stoul(m_headers["CONTENT_LENGTH"]) : 0;

	// with certain file extension specified in the config file invoke CGI handler (GET,POST)
	m_path = m_path.substr(1); // do this better might throw exception
	if (m_method == GET) {
		if (m_path.empty())
			m_path = "index.html";

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
	/*
		check for content-disposition -> name="name" -> open file with the name
		if chuncked set output_filestream for client
		set status code
	*/

	if (m_method == POST) {
		if (m_headers.find("CONTENT_TYPE") != m_headers.end()) { // has a body to parse
			if (m_headers["CONTENT_TYPE"] == "application/x-www-form-urlencoded") {
				// Parse key-value pairs from the body to a map
				while (getline(request, line, '&')) {
					pos = line.find('=');
					if (pos != std::string::npos)
						client->setFormData(line.substr(0, pos), line.substr(pos + 1));
				}
			} else if (m_headers["CONTENT_TYPE"].find("multipart/form-data") != std::string::npos) {
				// file uploads, parse boundary-separated sections, CGI?
				client->setBoundary("--" + m_headers["CONTENT_TYPE"].substr(m_headers["CONTENT_TYPE"].find("=") + 1));
			}
			} else if (m_headers["CONTENT_TYPE"] == "application/json") {
				// Parse the body as JSON using a library or custom parser
			}
		}
	if (m_method == DELETE) {
		/*
			validate path
			delete resource identified by the path
		*/
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
