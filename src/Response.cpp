#include "Response.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"
#include "Parse.hpp"
#include "Logger.hpp"
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
#include <variant>

Response::Response(std::shared_ptr<Client> client, Config& config) : m_client(client), m_config(config),
m_parsing(REQUEST), m_code(200), m_msg("OK"), m_status(STATUS_BLANK), m_header(""), m_body(""), m_size(0)
{
	try {
		while (m_parsing != COMPLETE) {
			readRequest(client->fd());
			switch (m_parsing) {
				case REQUEST:
					parseRequest();
					break;
				case CHUNKED:
					parseChunked();
					break;
				case MULTIPART:
					parseMultipart(getBoundary(m_headers["content-type"]), m_multipartData);
					break;
				case COMPLETE:
					break;
			}
		} 
	} catch (HttpException& e) {
		m_code = e.getStatusCode();
		m_msg = e.what(); 
	}

	// log("=================== DEBUG =====================");
	// log("------------------- QUERY ---------------------");
	// displayQueryData();
	// log("------------------ CHUNKED --------------------");
	// log(std::string(m_unchunked.begin(), m_unchunked.end()));
	// log("----------------- MULTIPART -------------------");
	// displayMultipart(m_multipartData);
	// log("===============================================");

	// set environment variables & invoke CGI here

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

		Logger::getInstance().log("== SINGLE RESPONSE ==\n" + str() + "\n\n");
	}
	else
	{
		m_send_type = TYPE_CHUNK;
		m_header = getHeaderChunk();

		Logger::getInstance().log("== CHUNK RESPONSE ==" + std::to_string(m_size));
	}

	m_status = STATUS_OK;

}

void	Response::readRequest(int fd)
{
	char	buffer[PACKET_SIZE];
	ssize_t	bytes_read = recv(fd, buffer, PACKET_SIZE, 0);

	Logger::getInstance().log("-- BYTES READ " + std::to_string(bytes_read) + "--\n\n");

	if (bytes_read == -1)
		throw HttpException::internalServerError("failed to recieve request");
	if (bytes_read == 0)
	{
		m_status = STATUS_FAIL;
		m_send_type = TYPE_NONE;
		throw HttpException::badRequest("empty request");
	}
	m_request.insert(m_request.end(), buffer, buffer + bytes_read);
	Logger::getInstance().log(std::string(buffer, bytes_read));
}

void	Response::parseRequest()
{
	std::string			emptyLine = "\r\n\r\n";
	auto 				endOfHeaders = std::search(m_request.begin(), m_request.end(), emptyLine.begin(), emptyLine.end());

	if (endOfHeaders == m_request.end())
		throw HttpException::badRequest("invalid request");

	std::istringstream	request(std::string(m_request.begin(), endOfHeaders));

	parseRequestLine(request);
	parseHeaders(request);

	if (m_method == "GET")
		return handleGet();
	if (m_method == "POST")
		return handlePost(endOfHeaders);
	if (m_method == "DELETE")
		return handleDelete();
}

void	Response::handleGet()
{
	try {
		m_size = std::filesystem::file_size(m_path);
		m_parsing = COMPLETE;
	} catch (const std::exception& e) {
		m_size = 0;
		throw HttpException::notFound();
	}
}

void	Response::handlePost(std::vector<char>::iterator& endOfHeaders)
{
	if (!headerFound("content-type"))
		throw HttpException::badRequest("missing content type in a POST request");

	m_request.erase(m_request.begin(), endOfHeaders + 4);

	if (headerFound("transfer-encoding")) {
		if (m_headers["transfer-encoding"] != "chunked")
			throw HttpException::notImplemented(); // is this only for methods?
		parseChunked();
	}
	else if (m_headers["content-type"].find("multipart") != std::string::npos)
		parseMultipart(getBoundary(m_headers["content-type"]), m_multipartData);
	// else
	// 	throw HttpException::notImplemented(); // invalid content type?
}

void	Response::handleDelete()
{
	try {
		if (std::filesystem::is_directory(m_path))
			throw HttpException::forbidden();
		else {
			Logger::getInstance().log("REMOVING " + m_path);
			std::filesystem::remove(m_path);
			m_parsing = COMPLETE;
		}
	} catch (std::filesystem::filesystem_error& e) {
		throw HttpException::internalServerError("unable to delete target " + m_path);
	}
}

void	Response::parseRequestLine(std::istringstream& request)
{
	std::string	line;
	std::string	excess;

	getline(request, line);
	std::istringstream	requestLine(line);

	if (!(requestLine >> m_method >> m_target >> m_version) || requestLine >> excess)
		throw HttpException::badRequest("invalid request line");

	validateVersion();
	validateURI();
	validateMethod();
}

void	Response::validateVersion()
{
	if (m_version != "HTTP/1.1")
		throw HttpException::httpVersionNotSupported();
}

void	Response::validateURI()
{
	if (m_target.find("../") != std::string::npos)
		throw HttpException::badRequest("forbidden traversal pattern in URI");
	
	decodeURI();
	parseQueryString();
	validateCgi();

	std::vector<std::string> root;
	getDirective("root", root);
	m_path = root.empty() ? m_target : root.front() + m_target;

	try { // fix this - Set a default file to answer if the request is a directory
		if (!std::filesystem::exists(m_path) || std::filesystem::is_directory(m_path)) {
			std::vector<std::string> indices;
			getDirective("index", indices);
			for (std::string index: indices) {
				std::string newPath = m_path + index;
				if (std::filesystem::exists(newPath)) {
					m_path = newPath;
					break;
				}
			}
		}
		if (!std::filesystem::exists(m_path))
			throw HttpException::notFound();
		std::filesystem::perms perms = std::filesystem::status(m_path).permissions();
		if ((perms & std::filesystem::perms::others_read) == std::filesystem::perms::none)
			throw HttpException::forbidden();
	} catch (std::exception& e) {
		throw HttpException::notFound();
	}
}

void	Response::validateMethod()
{
	std::vector<std::string>	methods;

	getDirective("methods", methods);
	if (std::find(methods.begin(), methods.end(), m_method) == methods.end())
		throw HttpException::notImplemented();
}

void	Response::decodeURI() {
	try {
		while (m_target.find('%') != std::string::npos) {
			size_t pos = m_target.find('%');
			m_target.insert(pos, 1, stoi(m_target.substr(pos + 1, 2), nullptr, 16));
			m_target.erase(pos + 1, 3);
		}
	} catch (std::exception& e) {
		throw HttpException::badRequest("malformed URI");
	}
}

void	Response::parseQueryString()
{
	size_t pos = m_target.find("?");
	if (pos == std::string::npos)
		return;

	std::istringstream query(m_target.substr(pos + 1));
	m_target = m_target.substr(0, pos);

	for (std::string line; getline(query, line, '&');) {
		size_t pos = line.find('=');
		if (pos == std::string::npos)
			throw HttpException::badRequest("malformed query string");
		m_queryData[line.substr(0, pos)].push_back(line.substr(pos + 1));
	}
}

void	Response::validateCgi()
{
	if (m_target.find(".") == std::string::npos)
		return;
	
	std::string					extension(m_target.substr(m_target.find_last_of(".")));
	std::vector<std::string>	cgi;

	getDirective("cgi", cgi);
	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_status = STATUS_CGI;
}

void	Response::parseHeaders(std::istringstream& request)
{
	std::regex	headerRegex(R"(^[!#$%&'*+.^_`|~A-Za-z0-9-]+:\s*.+$)");

	for (std::string line; getline(request, line);) {
		if (line.back() == '\r')
			line.pop_back();
    	if (!std::regex_match(line, headerRegex)) 
			throw HttpException::badRequest("malformed header");
		size_t pos = line.find(':');
		std::string key = line.substr(0, pos);
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		if (m_headers.find(key) != m_headers.end())
			throw HttpException::badRequest("duplicate header");
		std::string value = line.substr(pos + 1);
		value.erase(0, value.find_first_not_of(" "));
		value.erase(value.find_last_not_of(" ") + 1);
		m_headers[key] = value;
	}

	if (headerFound("connection") && m_headers["connection"] == "close")
		m_client->setCloseConnection();

	validateHost();
}

void	Response::validateHost()
{
	if (!headerFound("host"))
		throw HttpException::badRequest("missing host header");

	std::vector<std::string> hosts;
	m_config.tryGetDirective("server_name", hosts);
	for (std::string host: hosts) {
		if (host == m_headers["host"])
			return;
	}
	throw HttpException::badRequest("invalid host");
}

void	Response::parseChunked() { // not properly tested
	Logger::getInstance().log("IN CHUNKED PARSING");
	if (m_request.empty()) {
		Logger::getInstance().log("EMPTY CHUNK");
		m_parsing = CHUNKED;
		return;
	}
	std::string					delim = "\r\n";
	std::vector<char>::iterator	currentPos = m_request.begin();
	std::vector<char>::iterator	endOfSize;
	std::vector<char>::iterator	endOfContent;

	while (true) {
		endOfSize = std::search(currentPos, m_request.end(), delim.begin(), delim.end());
		if (endOfSize == currentPos || endOfSize == m_request.end()) // content should begin with size
			throw HttpException::badRequest("malformed chunked content");
		std::string	sizeString(currentPos, endOfSize);
		size_t		chunkSize = getChunkSize(sizeString);
		if (chunkSize == 0) {
			Logger::getInstance().log("CHUNKED PARSING COMPLETE");
			m_parsing = COMPLETE;
			return;
		}
		endOfSize += 2;
		endOfContent = std::search(endOfSize, m_request.end(), delim.begin(), delim.end());
		if (endOfContent == m_request.end()) { // incomplete chunk
			if (currentPos > m_request.begin())
				m_request.erase(m_request.begin(), currentPos); // erase what's already unchunked
			m_parsing = CHUNKED;
			return;
		}
		if (static_cast<size_t>(endOfContent - endOfSize) != chunkSize)
			throw HttpException::badRequest("invalid chunk size");
		m_unchunked.insert(m_unchunked.end(), endOfSize, endOfContent);
		currentPos = endOfContent + 2;
	}
}

size_t	Response::getChunkSize(std::string& hex) {
	try {
		return stoul(hex, nullptr, 16);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid chunk size");
	}
}

void	Response::parseMultipart(std::string boundary, std::vector<multipart>& multipartData)
{
	if (getContentLength() != m_request.size()) {
		m_parsing = MULTIPART;
		return;
	}

	std::string	emptyLine = "\r\n\r\n";
	std::string end = "--\r\n";
	size_t		boundaryLen = boundary.length();

	auto firstBoundary = std::search(m_request.begin(), m_request.end(), boundary.begin(), boundary.end());
	if (firstBoundary != m_request.begin()) // multipart/form-data should begin with boundary
		throw HttpException::badRequest("invalid multipart/form-data content");
	
	auto	currentPos = m_request.begin() + boundaryLen;

	while (!std::equal(currentPos, m_request.end(), end.begin(), end.end())) {
		currentPos += 2; // skip \r\n after boundary
		auto endOfHeaders = std::search(currentPos, m_request.end(), emptyLine.begin(), emptyLine.end());
		if (endOfHeaders == m_request.end()) // headers should end with an empty line
			throw HttpException::badRequest("invalid multipart/form-data content");
		multipart	part;
		std::string	headers(currentPos, endOfHeaders);
		ParseMultipartHeaders(headers, part);
		currentPos = endOfHeaders + 4;
		auto endOfContent = std::search(currentPos, m_request.end(), boundary.begin(), boundary.end());
		if (endOfContent == m_request.end()) // content should end with boundary
			throw HttpException::badRequest("invalid multipart/form-data content");
		if (part.filename.empty()) {
			part.content.insert(part.content.end(), currentPos, endOfContent - 2);
		} else {
			std::ofstream file = getFileStream(part.filename);
			file.write(&(*currentPos), std::distance(currentPos, endOfContent - 2));
		}
		multipartData.push_back(part);
		currentPos = endOfContent + boundaryLen;
	}
	m_parsing = COMPLETE;
}

size_t	Response::getContentLength()
{
	if (!headerFound("content-length"))
		throw HttpException::lengthRequired();

	size_t length;

	try {
		length = std::stoul(m_headers["content-length"]);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid content length");
	}
	return length; 
}

std::string	Response::getBoundary(std::string& contentType)
{
	size_t startOfBoundary = contentType.find("boundary=");
	if (startOfBoundary == std::string::npos)
		throw HttpException::badRequest("missing boundary for multipart/form-data");
	
	return "--" + contentType.substr(startOfBoundary + 9);
}

void	Response::ParseMultipartHeaders(std::string& headerString, multipart& part)
{
	std::istringstream	headers(headerString);
	size_t				startPos;
	size_t				endPos;

	for (std::string line; getline(headers, line);) {
		if (line.back() == '\r')
			line.pop_back();
		std::transform(line.begin(), line.end(), line.begin(), ::tolower);
		if (line.find("content-disposition: form-data") != std::string::npos) {
			startPos = line.find("name=\"");
			if (startPos == std::string::npos)
				throw HttpException::badRequest("invalid header for multipart/form-data");
			startPos += 6;
			endPos = line.find("\"", startPos);
			if (endPos == std::string::npos)
				throw HttpException::badRequest("malformed multipart header");
			part.name = line.substr(startPos, endPos - startPos);
			startPos = line.find("filename=\"");
			if (startPos != std::string::npos) {
				startPos += 10;
				endPos = line.find("\"", startPos);
				if (endPos == std::string::npos)
					throw HttpException::badRequest("malformed multipart header");
				part.filename = line.substr(startPos, endPos - startPos);
			}
		}
		else if (line.find("content-type: ") != std::string::npos) {
			startPos = line.find("content-type: ") + 14;
			part.contentType = line.substr(startPos);
			if (part.contentType.find("multipart") != std::string::npos) // parse nested multipart
				parseMultipart(getBoundary(part.contentType), part.nestedData);
		}
		else
			throw HttpException::badRequest("invalid header for multipart/form-data");
	}
}

std::ofstream	Response::getFileStream(std::string filename)
{
	if (filename.find("../") != std::string::npos)	
		throw HttpException::badRequest("forbidden traversal pattern in filename");
	
	std::string					filePath;
	std::vector<std::string>	folder;

	getDirective("uploadFolder", folder); // should it be an error if folder is not defined?
	filePath = folder.empty() ? filename : folder.front() + '/' + filename; // what if there are multiple files with the same name

	std::ofstream filestream(filePath, std::ios::out | std::ios::binary);
	if (!filestream)
		throw HttpException::internalServerError("open failed for file upload");
			
	return filestream;
}

bool	Response::headerFound(const std::string& header)
{
	if (m_headers.find(header) != m_headers.end())
		return true;
	return false;
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

void	Response::getDirective(std::string dir, std::vector<std::string>& out)
{
	std::shared_ptr<ConfigNode>	location(m_config.findNode(m_target));
	if (!location)
		location = m_config.findNode("/");
	
	location->tryGetDirective(dir, out);
}

void	Response::displayQueryData() // debug
{
	for (auto& [key, values] : m_queryData) {
		std::cout << key << "=";
		for (std::string value: values)
			std::cout << value << "\n";
	}
}

void	Response::displayMultipart(std::vector<multipart>& multipartData) // debug
{
	for (multipart part: multipartData) {
		Logger::getInstance().log("name: " + part.name);
		Logger::getInstance().log("filename: " + part.filename);
		Logger::getInstance().log("content-type: " + part.contentType);
		Logger::getInstance().log("content: " + std::string(part.content.begin(), part.content.end()));
		Logger::getInstance().log("");
		if (!part.nestedData.empty())
			displayMultipart(part.nestedData);
	}
}

// void	Response::parseUrlencoded(std::shared_ptr<Client> client, std::istringstream& body)
// {
// 	for (std::string line; getline(body, line, '&');) {
// 		size_t pos = line.find('=');
// 		if (pos != std::string::npos) {
// 			std::string value = line.substr(pos + 1);
// 			decode(value);
// 			replace(value.begin(), value.end(), '+', ' ');
// 			client->addFormData(line.substr(0, pos), value);
// 		}
// 	}
// }
