#include "HttpRequest.hpp"
#include "HttpException.hpp"

#include <algorithm>
#include <filesystem>
#include <regex>

HttpRequest::HttpRequest(Config& config, int fd) :  m_config(config), m_parsing(REQUEST)
{
	try {
		while (m_parsing != COMPLETE) {
			readRequest(fd);
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
		// m_code = e.getStatusCode();
		// m_msg = e.what(); 
	}
}

void	HttpRequest::readRequest(int fd)
{
	char	buffer[PACKET_SIZE];
	ssize_t	bytes_read = recv(fd, buffer, PACKET_SIZE, 0);

	Logger::getInstance().log("-- BYTES READ " + std::to_string(bytes_read) + "--\n\n");

	if (bytes_read == -1)
		throw HttpException::internalServerError("failed to recieve request");
	if (bytes_read == 0)
	{
		// m_status = STATUS_FAIL;
		// m_send_type = TYPE_NONE;
		throw HttpException::badRequest("empty request");
	}
	m_request.insert(m_request.end(), buffer, buffer + bytes_read);
	Logger::getInstance().log(std::string(buffer, bytes_read));
}

void	HttpRequest::parseRequest()
{
	std::string	emptyLine = "\r\n\r\n";
	auto		endOfHeaders = std::search(m_request.begin(), m_request.end(), emptyLine.begin(), emptyLine.end());

	if (endOfHeaders == m_request.end())
		throw HttpException::badRequest("invalid request");

	std::istringstream	request(std::string(m_request.begin(), endOfHeaders));

	parseRequestLine(request);
	parseHeaders(request);
	parseBody(endOfHeaders);
}

void	HttpRequest::parseRequestLine(std::istringstream& request)
{
	std::string	line;
	std::string	excess;
	std::string version;

	getline(request, line);
	std::istringstream	requestLine(line);

	if (!(requestLine >> m_method >> m_target >> version) || requestLine >> excess)
		throw HttpException::badRequest("invalid request line");

	if (version != "HTTP/1.1")
		throw HttpException::httpVersionNotSupported();

	parseURI();
}

void	HttpRequest::parseURI()
{
	if (m_target.find("../") != std::string::npos)
		throw HttpException::badRequest("forbidden traversal pattern in URI");
	
	decodeURI();
	parseQueryString();
}

void	HttpRequest::decodeURI() {
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

void	HttpRequest::parseQueryString()
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

void	HttpRequest::parseHeaders(std::istringstream& request)
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
}

void	HttpRequest::parseBody(std::vector<char>::iterator endOfHeaders)
{
	m_request.erase(m_request.begin(), endOfHeaders + 4);
	if (m_request.empty()) {
		m_parsing = COMPLETE;
		return;
	}

	if (m_headers.find("transfer-encoding") != m_headers.end()
		&& m_headers["transfer-encoding"] == "chunked")
		parseChunked();
	else if (m_headers["content-type"].find("multipart") != std::string::npos)
		parseMultipart(getBoundary(m_headers["content-type"]), m_multipartData);
	else
		throw HttpException::notImplemented();
}

void	HttpRequest::parseChunked() { // not properly tested
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

size_t	HttpRequest::getChunkSize(std::string& hex) {
	try {
		return stoul(hex, nullptr, 16);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid chunk size");
	}
}

void	HttpRequest::parseMultipart(std::string boundary, std::vector<multipart>& multipartData)
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
		// if (part.filename.empty()) {
		part.content.insert(part.content.end(), currentPos, endOfContent - 2);
		// } else {
		// 	std::ofstream file = getFileStream(part.filename);
		// 	file.write(&(*currentPos), std::distance(currentPos, endOfContent - 2));
		// }
		multipartData.push_back(part);
		currentPos = endOfContent + boundaryLen;
	}
	m_parsing = COMPLETE;
}

size_t	HttpRequest::getContentLength()
{
	if (m_headers.find("content-length") != m_headers.end())
		throw HttpException::lengthRequired();

	size_t length;

	try {
		length = std::stoul(m_headers["content-length"]);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid content length");
	}
	return length; 
}

std::string	HttpRequest::getBoundary(std::string& contentType)
{
	size_t startOfBoundary = contentType.find("boundary=");
	if (startOfBoundary == std::string::npos)
		throw HttpException::badRequest("missing boundary for multipart/form-data");
	
	return "--" + contentType.substr(startOfBoundary + 9);
}

void	HttpRequest::ParseMultipartHeaders(std::string& headerString, multipart& part)
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

// std::ofstream	HttpRequest::getFileStream(std::string filename)
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
