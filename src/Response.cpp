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
#include <regex>

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
	log(m_request);
	return true;
}

void	parseMultipart(std::string request, std::string boundary, std::shared_ptr<Client> client)
{
	(void)client;
	std::cout << "\n\nMULTIPART PARSING\n";
	std::cout << request << "\n\n";

	std::istringstream	data(request);
	std::string			line;

	while (getline(data, line)) {
		while (!line.empty()) {
			// content-disposition & content-type
		}
		// content
		if (line == boundary + "\r\n")
			continue;
	}
}

void	Response::parseRequest(std::shared_ptr<Client> client)
{
	size_t	pos = m_request.find('\n');
	if (pos == std::string::npos || pos == std::string::npos - 1) {
		logError("Invalid request");
		return;
	}
	{
	std::istringstream	request_line(m_request.substr(0, pos));
	std::string			version;
	std::string			method;
	std::string			rest;

	if (!(request_line >> method >> m_path >> version) || request_line >> rest || version != "HTTP/1.1") {
		logError("Invalid request line");
		m_code = 400;
		return;
	}

	std::unordered_map<std::string, e_method> methods = {{"GET", e_method::GET}, {"POST", e_method::POST}, {"DELETE", e_method::DELETE}};
	if (methods.find(method) != methods.end()) {
		m_method = methods[method];
	} else {
		logError("Invalid or missing method");
		m_code = 400;
		return;
	}
	}
	std::istringstream	request(m_request.substr(pos + 1));
	std::regex 			headerRegex(R"(^[!#$%&'*+.^_`|~A-Za-z0-9-]+:\s*.*[\x20-\x7E]\r*$)");
	std::string			line;
	std::string			key;
	std::string 		value;

	while (getline(request, line) && (line.find(':') != std::string::npos)) {
    	if (std::regex_match(line, headerRegex)) {
			pos = line.find(':');
			key = line.substr(0, pos);
			std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::toupper(c); });
			std::replace(key.begin(), key.end(), '-', '_');
			value = line.substr(pos + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of("\r") + 1);
			m_headers.try_emplace(key, value);
		} else {
			m_code = 400; // Bad Request
			return;
		}
	}

	// with certain file extension specified in the config file invoke CGI handler (GET,POST)
	if (m_method == GET) {
		if (m_path.empty())
			m_path = "/index.html";

		m_path = m_path.substr(1);
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
				client->openFile();
			} else if (m_headers["CONTENT_TYPE"].find("multipart/form-data") != std::string::npos) {
				std::cout << "IN MULTIPART PARSING\n";
				while (getline(request, line))
					std::cout << line << "\n";
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

