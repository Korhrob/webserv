#include "HttpRequest.hpp"
#include "HttpException.hpp"

#include <filesystem>
#include <algorithm>
#include <regex>

HttpRequest::HttpRequest(int fd) : m_fd(fd) {}

void	HttpRequest::parseRequest()
{
	readRequest();

	std::string	emptyLine = "\r\n\r\n";
	auto		endOfHeaders = std::search(m_request.begin(), m_request.end(), emptyLine.begin(), emptyLine.end());

	if (endOfHeaders == m_request.end())
		throw HttpException::badRequest("invalid request");

	std::istringstream	request(std::string(m_request.begin(), endOfHeaders));

	parseRequestLine(request);
	parseHeaders(request);

	m_request.erase(m_request.begin(), endOfHeaders + 4);
}

void	HttpRequest::parseBody(size_t maxSize)
{
	m_contentLength = 0;
	getBodyType(maxSize);

	while (m_body != COMPLETE)
	{
		if (m_body == CHUNKED)
			parseChunked(maxSize);
		else if (m_body == MULTIPART)
			parseMultipart(getBoundary(m_headers["content-type"]), m_multipartData);
		if (m_body != COMPLETE)
			readRequest();
	}
}

void	HttpRequest::getBodyType(size_t maxSize)
{
	if (m_headers.find("transfer-encoding") != m_headers.end() && m_headers["transfer-encoding"] == "chunked")
		m_body = CHUNKED;
	else if (m_headers["content-type"].find("multipart") != std::string::npos)
	{
		m_body = MULTIPART;
		if (getContentLength() > maxSize)
			throw HttpException::badRequest("Body exceeds max size limit");
	}
	else
		throw HttpException::notImplemented();
}

void	HttpRequest::readRequest()
{
	char	buffer[PACKET_SIZE];
	ssize_t	bytes_read = recv(m_fd, buffer, PACKET_SIZE, 0);

	Logger::getInstance().log("-- BYTES READ " + std::to_string(bytes_read) + " --\n\n");

	if (bytes_read == -1)
		throw HttpException::internalServerError("failed to receive request");
	if (bytes_read == 0)
		throw HttpException::requestTimeout();
	m_request.insert(m_request.end(), buffer, buffer + bytes_read);
	Logger::getInstance().log(std::string(buffer, bytes_read));
}

void	HttpRequest::parseRequestLine(std::istringstream& request)
{
	std::string	line;
	std::string	excess;
	std::string	version;

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

	std::istringstream queryString(m_target.substr(pos + 1));
	m_target.erase(pos);

	for (std::string line; getline(queryString, line, '&');)
	{
		size_t pos = line.find('=');
		if (pos == std::string::npos)
			throw HttpException::badRequest("malformed query string");
		m_queryData[line.substr(0, pos)].push_back(line.substr(pos + 1));
	}
}

void	HttpRequest::parseHeaders(std::istringstream& request)
{
	std::regex	headerRegex(R"(^[!#$%&'*+.^_`|~A-Za-z0-9-]+:\s*.+$)");

	for (std::string line; getline(request, line);)
	{
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

void	HttpRequest::parseChunked(size_t maxSize) { // not properly tested
	std::cout << "MAX SIZE: " << maxSize << "\n"; 
	if (!m_unchunked.is_open())
	{
		std::filesystem::path	unchunked = std::filesystem::temp_directory_path() / "unchunked";
		m_unchunked.open(unchunked, std::ios::binary | std::ios::app);
	}

	if (m_request.empty())
		return;

	std::string					delim = "\r\n";
	std::vector<char>::iterator	currentPos = m_request.begin();
	std::vector<char>::iterator	endOfSize;
	std::vector<char>::iterator	endOfContent;

	while (true)
	{
		endOfSize = std::search(currentPos, m_request.end(), delim.begin(), delim.end());
		if (endOfSize == m_request.end()) {
			Logger::log("INCOMPLETE CHUNK");
			if (currentPos > m_request.begin())
				m_request.erase(m_request.begin(), currentPos); // erase what's already unchunked
			return;
		}
		std::string	sizeString(currentPos, endOfSize);
		size_t		chunkSize = getChunkSize(sizeString);
		if (chunkSize == 0)
		{
			Logger::log("CHUNKED PARSING COMPLETE");
			m_body = COMPLETE;
			m_unchunked.close();
			return;
		}
		endOfSize += 2;
		endOfContent = std::search(endOfSize, m_request.end(), delim.begin(), delim.end());
		if (endOfContent == m_request.end()) // incomplete chunk
		{
			Logger::log("INCOMPLETE CHUNK");
			if (currentPos > m_request.begin())
				m_request.erase(m_request.begin(), currentPos); // erase what's already unchunked
			return;
		}
		if (static_cast<size_t>(endOfContent - endOfSize) != chunkSize)
			throw HttpException::badRequest("invalid chunk size");
		m_unchunked.write(&(*endOfSize), std::distance(endOfSize, endOfContent));
		m_contentLength += chunkSize;
		std::cout << "CONTENT LENGTH: " << m_contentLength << "\n";
		if (m_contentLength > maxSize)
			throw HttpException::badRequest("Body exceeds max size limit");
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
	if (getContentLength() != m_request.size())
		return;

	std::string	emptyLine = "\r\n\r\n";
	std::string end = "--\r\n";
	size_t		boundaryLen = boundary.length();

	auto firstBoundary = std::search(m_request.begin(), m_request.end(), boundary.begin(), boundary.end());
	if (firstBoundary != m_request.begin())
		throw HttpException::badRequest("invalid multipart/form-data content");
	
	auto	currentPos = m_request.begin() + boundaryLen;

	while (!std::equal(currentPos, m_request.end(), end.begin(), end.end()))
	{
		currentPos += 2;
		auto endOfHeaders = std::search(currentPos, m_request.end(), emptyLine.begin(), emptyLine.end());
		if (endOfHeaders == m_request.end())
			throw HttpException::badRequest("invalid multipart/form-data content");
		multipart	part;
		std::string	headers(currentPos, endOfHeaders);
		ParseMultipartHeaders(headers, part);
		currentPos = endOfHeaders + 4;
		auto endOfContent = std::search(currentPos, m_request.end(), boundary.begin(), boundary.end());
		if (endOfContent == m_request.end())
			throw HttpException::badRequest("invalid multipart/form-data content");
		if (currentPos != endOfContent)
		{
			if (part.filename.empty())
				part.content.insert(part.content.end(), currentPos, endOfContent - 2);
			else
			{
				std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;
				std::ofstream fileStream(tmpFile, std::ios::binary);
				if (!fileStream)
					throw HttpException::internalServerError("error opening file");
				fileStream.write(&(*currentPos), std::distance(currentPos, endOfContent - 2));
				fileStream.close();
			}
		}
		multipartData.push_back(part);
		currentPos = endOfContent + boundaryLen;
	}
	m_body = COMPLETE;
}

size_t	HttpRequest::getContentLength()
{
	if (m_headers.find("content-length") == m_headers.end())
		throw HttpException::lengthRequired();

	try {
		return std::stoul(m_headers["content-length"]);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid content length");
	}
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

	for (std::string line; getline(headers, line);)
	{
		if (line.back() == '\r')
			line.pop_back();
		std::transform(line.begin(), line.end(), line.begin(), ::tolower);
		if (line.find("content-disposition: form-data") != std::string::npos)
		{
			startPos = line.find("name=\"");
			if (startPos == std::string::npos)
				throw HttpException::badRequest("invalid header for multipart/form-data");
			startPos += 6;
			endPos = line.find("\"", startPos);
			if (endPos == std::string::npos)
				throw HttpException::badRequest("invalid header for multipart/form-data");
			part.name = line.substr(startPos, endPos - startPos);
			startPos = line.find("filename=\"");
			if (startPos != std::string::npos)
			{
				startPos += 10;
				endPos = line.find("\"", startPos);
				if (endPos == std::string::npos)
					throw HttpException::badRequest("invalid header for multipart/form-data");
				part.filename = line.substr(startPos, endPos - startPos);
				if (part.filename.find("../") != std::string::npos)	
					throw HttpException::badRequest("forbidden traversal pattern in filename");
			}
		}
		else if (line.find("content-type: ") != std::string::npos)
		{
			startPos = line.find("content-type: ") + 14;
			part.contentType = line.substr(startPos);
			if (part.contentType.find("multipart") != std::string::npos) // parse nested multipart
				parseMultipart(getBoundary(part.contentType), part.nestedData);
		}
		else
			throw HttpException::badRequest("invalid header for multipart/form-data");
	}
}

const std::string&	HttpRequest::getHost()
{
	if (m_headers.find("host") != m_headers.end())
		return m_headers["host"];
	return EMPTY_STRING;
}

const std::string&	HttpRequest::getTarget()
{
	return m_target;
}

const std::string&	HttpRequest::getMethod()
{
	return m_method;
}

const std::vector<multipart>&	HttpRequest::getMultipartData()
{
	return m_multipartData;
}

HttpRequest::~HttpRequest()
{
	for (multipart part: m_multipartData)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;
			try {
				std::filesystem::remove(tmpFile);
			} catch (std::filesystem::filesystem_error& e) {
				Logger::log("error removing temporary file");
			}
		}
	}
	// try {
	// 	std::filesystem::remove(std::filesystem::temp_directory_path() / "unchunked");
	// } catch (std::filesystem::filesystem_error& e) {
	// 	Logger::log("error removing temporary file");
	// }
}
