#include "Response.hpp"
#include "ConfigNode.hpp"
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

struct s_part {
	std::string	filename;
	std::string	content_type;
	std::string	content;
};

using part = s_part;
using multipart = std::unordered_map<std::string, part>;

struct s_part {
	std::string	filename;
	std::string	content_type;
	std::string	content;
};

using part = s_part;
using multipart = std::unordered_map<std::string, part>;

Response::Response(std::shared_ptr<Client> client) : m_status(STATUS_BLANK), m_size(0), m_body(""), m_header("")
{
	if (readRequest(client->fd()))
		parseRequest(client);

	// client->displayFormData(); // for debugging

	if (m_send_type == TYPE_NONE)
		return;

	// WRITE HEADER AND BODY

	if (m_size <= PACKET_SIZE)
	{
		m_send_type = TYPE_SINGLE;
		m_body = getBody(m_path);
		if (!m_body.empty())
		m_size = m_body.size();
		m_header = getHeaderSingle(m_size, m_code);

		log("== SINGLE RESPONSE ==\n" + str() + "\n\n");
	}
	else
	{
		m_send_type = TYPE_CHUNK;
		m_header = getHeaderChunk();

		log("== CHUNK RESPONSE ==" + std::to_string(m_size));
	}

	m_status = STATUS_OK;

}

bool	Response::readRequest(int fd)
{
	char	buffer[PACKET_SIZE];
	size_t	bytes_read = recv(fd, buffer, PACKET_SIZE, 0);

	log("-- BYTES READ " + std::to_string(bytes_read) + "--\n\n");

	if (bytes_read <= 0) // bad request tms?
	{
		logError("Empty or invalid request");
		m_status = STATUS_FAIL;
		m_code = 404;
		return false;
	}

	buffer[bytes_read] = '\0';
	m_request = std::string(buffer, bytes_read);
	log(m_request);
	return true;
}

void	Response::parseMultipart(std::shared_ptr<Client> client, std::istringstream& body)
{
	log("IN MULTIPART PARSING");
	/*
		Validate content_length
		Headers always begin right after the boundary line.
		Content is always separated from the headers by a blank line (\r\n\r\n).
		Each part is separated by the boundary, and the last boundary is marked by boundary-- to indicate the end of the request.
	*/
	std::string	boundary = client->getBoundary();
	std::string name;
	std::string key;
	std::string	value;
	multipart	parsedData;
	size_t		startPos;
	size_t		endPos;

	for (std::string line; getline(body, line);) {
		if (line.back() == '\r')
			line.pop_back();
		if (line.empty() || line == boundary)
			continue;
		if (line == boundary + "--")
			break;
		if (line.find("Content-Disposition") != std::string::npos) {
				startPos = line.find("name=\"");
			if (startPos != std::string::npos) {
				startPos += 6;
				endPos = line.find("\"", startPos);
				name = line.substr(startPos, endPos - startPos);
				parsedData[name];
			}
			startPos = line.find("filename=\"");
			if (startPos != std::string::npos) {
				startPos += 10;
				endPos = line.find("\"", startPos);
				parsedData[name].filename = line.substr(startPos, endPos - startPos);
				client->openFile(parsedData[name].filename);
			}
		} else if (line.find("Content-Type: ") != std::string::npos) {
			startPos = line.find("Content-Type: ") + 14;
			parsedData[name].content_type = line.substr(startPos);
		} else {
			if (parsedData[name].filename.empty())
				parsedData[name].content = line;
			else
				client->getFileStream() << line << "\n";
		}
	}
	for (auto [key, value]: parsedData) {
		log("key: " + key);
		log("filename: " + value.filename);
		log("content-type: " + value.content_type);
		log("content: " + value.content);
	}
	client->closeFile();
}

void	Response::parseRequest(std::shared_ptr<Client> client)
{
	size_t	pos = m_request.find('\n');
	if (pos == std::string::npos || pos == std::string::npos - 1) {
		logError("Invalid request");
		return;
	}

	std::istringstream	request_line(m_request.substr(0, pos));
	std::string			method;
	std::string			extra;

	if (!(request_line >> method >> m_path >> m_version) || request_line >> extra || !version()) {
		logError("Invalid request line");
		m_code = 400;
		return;
	}

	if (!setMethod(method))
		return;

	std::istringstream	request(m_request.substr(pos + 1));
	std::regex 			headerRegex(R"(^[!#$%&'*+.^_`|~A-Za-z0-9-]+:\s*.*[\x20-\x7E]*$)");
	std::string			key;
	std::string 		value;

	for (std::string line; getline(request, line);) {
		if (line.back() == '\r')
			line.pop_back();
		if (line.empty())
			break;
    	if (std::regex_match(line, headerRegex)) {
			pos = line.find(':');
			key = line.substr(0, pos);
			std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::toupper(c); });
			std::replace(key.begin(), key.end(), '-', '_');
			value = line.substr(pos + 1);
			value.erase(0, value.find_first_not_of(" "));
			value.erase(value.find_last_not_of(" ") + 1);
			m_headers.try_emplace(key, value);
		} else {
			m_code = 400; // Bad Request
			return;
		}
	}
	// with certain file extension specified in the config file invoke CGI handler (GET,POST)
	// if (m_method == GET) {
	m_path = m_path.substr(1);

	if (m_path.empty() || m_path == "/")
		m_path = "/index.html";

	//m_path = m_path.substr(1);

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
		m_code = 200;
	} catch (const std::exception& e) {
		m_size = 0;
		m_code = 404;
		std::cerr << e.what() << '\n';
	}
	// }
	/*
		if chuncked set output_filestream for client
		set status code
	*/

	if (m_method == POST) {
		// if (m_headers.find("CONTENT_TYPE") != m_headers.end()) { // has a body to parse
		std::istringstream	body;
		pos = m_request.find("\r\n\r\n");
		if (pos != std::string::npos) {
			body.str(m_request.substr(pos + 4));
			if (m_headers.find("CONTENT_LENGTH") != m_headers.end()) {
				std::string length = m_headers["CONTENT_LENGTH"];
				if (body.str().size() != std::stoul(length)) {
					logError("Invalid content length");
					// m_code = 
					return;
				}
			}
		}
			if (m_headers["CONTENT_TYPE"] == "application/x-www-form-urlencoded") {
				// Parse key-value pairs from the body to a map
				for (std::string line; getline(request, line, '&');) {
					pos = line.find('=');
					if (pos != std::string::npos)
						client->setFormData(line.substr(0, pos), line.substr(pos + 1));
				}
			} else if (m_headers["CONTENT_TYPE"].find("multipart/form-data") != std::string::npos) {
				client->setBoundary("--" + m_headers["CONTENT_TYPE"].substr(m_headers["CONTENT_TYPE"].find("=") + 1));
				parseMultipart(client, body);
			} else if (m_headers["CONTENT_TYPE"] == "application/json") {
				// Parse the body as JSON using a library or custom parser
			// }
		} // else bad request
	}

	if (m_method == DELETE) {
		/*
			validate path
			delete resource identified by the path
		*/
	}
}

bool	Response::setMethod(std::string method)
{
	std::unordered_map<std::string, e_method> methods = {{"GET", e_method::GET}, {"POST", e_method::POST}, {"DELETE", e_method::DELETE}};
	if (methods.find(method) != methods.end()) {
		m_method = methods[method];
		return true;
	} else {
		logError("Invalid or missing method");
		m_code = 405; // Method Not Allowed
		return false;
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

