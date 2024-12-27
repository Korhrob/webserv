#include "Response.hpp"
#include "HttpException.hpp"
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

Response::Response(std::shared_ptr<Client> client, Config& config) : m_status(STATUS_BLANK), m_header(""), m_body(""), m_size(0)
{
	if (readRequest(client->fd())) {
		try {
			parseRequest(client, config);
		} catch (HttpException& e) {
			log(e.what());
			m_code = e.getStatusCode();
			m_msg = e.what(); 
		}
	}

	if (m_send_type == TYPE_NONE)
		return;

	// WRITE HEADER AND BODY

	if (m_size <= PACKET_SIZE)
	{
		m_send_type = TYPE_SINGLE;
		m_body = getBody(m_path);
		if (!m_body.empty())
		m_size = m_body.size();
		m_header = getHeaderSingle(m_size, m_code, m_msg);

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
	// log("IN MULTIPART PARSING");
	/*
		Validate content_length
		Headers always begin right after the boundary line.
		Content is always separated from the headers by a blank line (\r\n\r\n).
		Each part is separated by the boundary, and the last boundary is marked by boundary-- to indicate the end of the request.
	*/
	std::string	boundary(client->getBoundary());
	std::string	name;
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
			}
			startPos = line.find("filename=\"");
			if (startPos != std::string::npos) {
				startPos += 10;
				endPos = line.find("\"", startPos);
				client->addMultipartData(name, FILENAME, line.substr(startPos, endPos - startPos));
				client->openFile(client->getFilename(name));
			}
		} else if (line.find("Content-Type: ") != std::string::npos) {
			startPos = line.find("Content-Type: ") + 14;
			client->addMultipartData(name, CONTENT_TYPE, line.substr(startPos));
		} else {
			if (client->getFilename(name).empty())
				client->addMultipartData(name, CONTENT, line);
			else
				client->getFileStream() << line << "\n";
		}
	}
	client->closeFile();
}

void	Response::parseHeaders(std::string str)
{
	std::istringstream	headers(str);
	std::regex			headerRegex(R"(^[!#$%&'*+.^_`|~A-Za-z0-9-]+:\s*.*[\x20-\x7E]*$)");

	for (std::string line; getline(headers, line);) {
		if (line.back() == '\r')
			line.pop_back();
		if (line.empty())
			break;
    	if (std::regex_match(line, headerRegex)) {
			size_t pos = line.find(':');
			std::string key = line.substr(0, pos);
			std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::toupper(c); });
			std::replace(key.begin(), key.end(), '-', '_');
			std::string value = line.substr(pos + 1);
			value.erase(0, value.find_first_not_of(" "));
			value.erase(value.find_last_not_of(" ") + 1);
			m_headers.try_emplace(key, value);
		} else
			throw HttpException::badRequest();
	}
}

void	Response::validatePath(Config& config)
{
	sanitizePath();

	std::vector<std::string>	out;

	config.tryGetDirective("root", out);
	m_path = out.empty() ? m_path : *out.begin() + m_path; // is root mandatory should this throw an exception?
	
	m_path = m_path.substr(1);

	std::ifstream	file(m_path);

	if (!file.good()) {
		config.tryGetDirective("index", out);
		for (std::string idx: out) {
			std::string newPath = m_path + idx;
			file.clear();
			file = std::ifstream(newPath);
			if (file.good())
			{
				m_path = newPath;
				break;
			}
		}
	}
	if (!file.good())
		throw HttpException::notFound();
	try {
		m_size = std::filesystem::file_size(m_path);
		m_code = 200;
	} catch (const std::exception& e) {
		m_size = 0;
		m_code = 404;
		std::cerr << e.what() << '\n';
	}
}

void	Response::parseRequestLine(std::string line, Config& config)
{
	std::istringstream	requestLine(line);
	std::string			method;
	std::string			extra;

	if (!(requestLine >> method >> m_path >> m_version) || requestLine >> extra)
		throw HttpException::invalidRequestLine();

	validateVersion();
	validateMethod(method);
	validatePath(config);
}

void	Response::parseRequest(std::shared_ptr<Client> client, Config& config)
{
	size_t	requestLine = m_request.find('\n');
	if (requestLine == std::string::npos)
		throw HttpException::invalidRequestLine();

	parseRequestLine(m_request.substr(0, requestLine), config);
	
	size_t	headers = m_request.find("\r\n\r\n");
	if (headers == std::string::npos)
		throw HttpException::badRequest();

	parseHeaders(m_request.substr(requestLine + 1, headers - requestLine));

	switch (m_method) {
		case GET:
			break;
		case POST: {
			std::istringstream	body(m_request.substr(headers + 4));
			if (m_headers.find("CONTENT_LENGTH") != m_headers.end()) {
				std::string length = m_headers["CONTENT_LENGTH"];
				if (body.str().size() != std::stoul(length))
					throw HttpException::badRequest();
			}
			if (m_headers["CONTENT_TYPE"] == "application/x-www-form-urlencoded") {
				for (std::string line; getline(body, line, '&');) {
					size_t pos = line.find('=');
					if (pos != std::string::npos)
						client->setFormData(line.substr(0, pos), line.substr(pos + 1));
				}
			} else if (m_headers["CONTENT_TYPE"].find("multipart/form-data") != std::string::npos) {
					client->setBoundary("--" + m_headers["CONTENT_TYPE"].substr(m_headers["CONTENT_TYPE"].find("=") + 1));
					parseMultipart(client, body);
					client->displayMultipartData();
			} else if (m_headers["CONTENT_TYPE"] == "application/json") {
				// Parse the body as JSON
			}
			else 
				throw HttpException::badRequest();
			}
		case DELETE: {
			// delete resource identified by the path
			break;
		}
		// with certain file extension specified in the config file invoke CGI handler (GET,POST)
		// if chuncked set output_filestream for client
	}
}

void	Response::validateVersion()
{
	if (m_version != "HTTP/1.1")
		throw HttpException::httpVersionNotSupported();
}

void	Response::validateMethod(std::string method)
{
	std::unordered_map<std::string, e_method> methods = {{"GET", e_method::GET}, {"POST", e_method::POST}, {"DELETE", e_method::DELETE}};
	if (methods.find(method) == methods.end())
		throw HttpException::notImplemented();
	m_method = methods[method];
}

void	Response::sanitizePath()
{
	while (m_path.find("../") == 0) {
		m_path.erase(0, 3);
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

size_t Response::size()
{
	return m_size;
}
