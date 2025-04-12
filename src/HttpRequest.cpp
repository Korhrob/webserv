#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpException.hpp"
#include "Client.hpp"
#include "CGI.hpp"
#include "Server.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <cstring>

HttpRequest::HttpRequest() : m_state(HEADERS), m_cgi(false), m_contentLength(0)
{
	reset();
}

void	HttpRequest::appendRequest(std::vector<char>& request)
{
	m_request.insert(m_request.end(), request.begin(), request.end());
	request.clear();
	std::vector<char>().swap(request);
}

void	HttpRequest::parseRequest(Config& config)
{
	if (m_state == HEADERS)
	{
		std::string	emptyLine = "\r\n\r\n";
		auto		endOfHeaders = std::search(m_request.begin(), m_request.end(), emptyLine.begin(), emptyLine.end());

		if (endOfHeaders == m_request.end())
			return;

		if (std::distance(m_request.begin(), endOfHeaders) > HEADERS_MAX)
			throw HttpException::badRequest("Headers too long");

		std::istringstream	request(std::string(m_request.begin(), endOfHeaders));

		parseRequestLine(request);
		parseHeaders(request);
		setLocation(config);
		validateMethod();
		setCgi();
		setPath();

		if (m_method == "POST")
		{
			setMaxSize();
			setBodyType();
			m_request.erase(m_request.begin(), endOfHeaders + 4);
		}
		else
			m_state = COMPLETE;
	}

	if (m_state == CHUNKED)
		parseChunked();

	if (m_state == MULTIPART)
		parseMultipart(boundary(m_headers["content-type"]), m_multipart);
}

HttpResponse	HttpRequest::processRequest(t_ms timeout)
{
	if (m_method == "POST")
		handlePost(m_multipart);

	else if (m_method == "DELETE")
		handleDelete();

	return HttpResponse(200, "OK", m_path, m_target, closeConnection(), timeout);
}

void	HttpRequest::parseRequestLine(std::istringstream& request)
{
	std::string	line;
	std::string	excess;
	std::string	version;

	getline(request, line);

	if (line.length() > URI_MAX)
		throw HttpException::URITooLong("requested URI too long");

	std::istringstream	requestLine(line);

	if (!(requestLine >> m_method >> m_target >> version) || requestLine >> excess)
		throw HttpException::badRequest("invalid request line");

	if (version != "HTTP/1.1")
		throw HttpException::HTTPVersionNotSupported(version);

	parseURI();
	requestLine.clear();
}

void	HttpRequest::parseURI()
{
	if (m_target.find("../") != std::string::npos)
		throw HttpException::badRequest("forbidden traversal pattern in URI");
	
	decodeURI();
	parseQueryString();
}

void	HttpRequest::decodeURI()
{
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

		m_query[line.substr(0, pos)].push_back(line.substr(pos + 1));
	}
}

void	HttpRequest::parseHeaders(std::istringstream& request)
{
	std::regex	headerRegex(R"(^[!#$%&'*+.^_`|~A-Za-z0-9-]+:\s*.*$)");

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

void	HttpRequest::setLocation(Config& config)
{
    m_server = config.findServerNode(host());
    m_location = m_server->findClosestMatch(m_target);

	std::vector<std::string>	redirect;

	m_location->tryGetDirective("return", redirect);

	if (!redirect.empty())
	{
		int 		code = std::stoi(redirect.front());
		std::string msg = HttpException::statusMessage(code);

		if (msg.empty())
			throw HttpException::notImplemented("unknown status code in return directive");

		if (redirect.size() == 2)
			throw HttpException::redirect(code, msg, redirect[1]);
		else
			throw HttpException::withCode(code);
	}
}

void    HttpRequest::setCgi()
{
	size_t	pos = m_target.find_last_of(".");

	if (pos == std::string::npos)
		return;

	std::string					extension(m_target.substr(pos));
	std::vector<std::string>	cgi;

    if (!m_location->tryGetDirective("cgi", cgi))
		m_server->tryGetDirective("cgi", cgi);

	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_cgi = true;
}

void    HttpRequest::setPath()
{
	std::vector<std::string> root;

    if (!m_location->tryGetDirective("root", root))
		m_server->tryGetDirective("root", root);

	m_root = root.front();
    m_path = m_root + m_target;

	if (!std::filesystem::exists(m_path))
		tryTry_files();

	if (std::filesystem::exists(m_path) && std::filesystem::is_directory(m_path))
	{
		tryIndex();

		if (std::filesystem::is_directory(m_path))
			tryAutoindex();
	}

	if (!std::filesystem::exists(m_path))
		throw HttpException::notFound("requested resource could not be found");

	std::filesystem::perms perms = std::filesystem::status(m_path).permissions();

	if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none)
		throw HttpException::forbidden("permission denied for requested resource");
}

void	HttpRequest::tryTry_files()
{
	std::vector<std::string>	try_files;

	if (!m_location->tryGetDirective("try_files", try_files))
			m_server->tryGetDirective("try_files", try_files);

		if (!try_files.empty())
		{
			bool success = false;

			for (std::string temp : try_files) {

				std::size_t pos = 0;
				while ((pos = temp.find("$uri", pos)) != std::string::npos)
				{
					temp.replace(pos, 4, m_target);
					pos += m_target.length();
				}

				if (std::filesystem::exists(m_root + temp)) {
					m_path = m_root + temp;
					m_target = temp;
					success = true;
					break;
				}
			}

			if (!success)
			{
				std::regex errorValue("^=\\d{3}$");

				if (std::regex_match(try_files.back(), errorValue))
				{
					int code = std::stoi(try_files.back().substr(1));

					throw HttpException::withCode(code);
				}
			}
		}
}

void	HttpRequest::tryIndex()
{
	std::vector<std::string>	index;

	if (!m_location->tryGetDirective("index", index))
		m_server->tryGetDirective("index", index);

	for (std::string& index : index) {

		std::string temp = m_path;

		if (temp.back() != '/')
			temp += '/';

		temp += index;

		if (std::filesystem::exists(temp))
		{
			m_path = temp;
			return;
		}
	}
}

void	HttpRequest::tryAutoindex()
{
	if (m_path.back() != '/')
		m_path += "/";

	if (m_target.back() != '/')
		m_target += "/";

	if (!m_location->autoindexOn())
		throw HttpException::forbidden("autoindex: off, but trying to access directory");
}

void	HttpRequest::setMaxSize()
{
	std::vector<std::string>	maxSize;

	if (!m_location->tryGetDirective("client_max_body_size", maxSize))
		m_server->tryGetDirective("client_max_body_size", maxSize);

	m_maxSize = std::stoul(maxSize.front());
}

void	HttpRequest::setBodyType()
{
	auto it = m_headers.find("transfer-encoding");
	if (it != m_headers.end() && it->second == "chunked")
	{
		m_state = CHUNKED;
		return;
	}

	it = m_headers.find("content-type");
	if (it != m_headers.end() && it->second.find("multipart") != std::string::npos)
	{
		m_state = MULTIPART;
		
		if (contentLength() > m_maxSize)
			throw HttpException::payloadTooLarge("max body size exceeded");

		return;
	}

	throw HttpException::notImplemented("unknown content type");
}

void	HttpRequest::parseChunked()
{
	if (!m_unchunked.is_open())
	{
		m_unchunkedData = std::filesystem::temp_directory_path() / (uniqueId() + "_unchunked");
		m_unchunked.open(m_unchunkedData, std::ios::out | std::ios::binary | std::ios::app);

		if (!m_unchunked.is_open())
			throw HttpException::internalServerError("error opening a stream");
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
		if (endOfSize == m_request.end())
			return eraseUnchunked(currentPos);

		std::string	sizeString(currentPos, endOfSize);
		size_t		size = chunkSize(sizeString);

		if (size == 0)
		{
			Logger::log("Unchunking complete");
			m_state = COMPLETE;
			m_unchunked.close();

			return;
		}

		m_contentLength += size;
		if (m_contentLength > m_maxSize)
			throw HttpException::payloadTooLarge("max body size exceeded");

		endOfSize += 2;

		endOfContent = std::search(endOfSize, m_request.end(), delim.begin(), delim.end());
		if (endOfContent == m_request.end())
			return eraseUnchunked(currentPos);

		if (static_cast<size_t>(endOfContent - endOfSize) != size)
			throw HttpException::badRequest("invalid chunk size");

		m_unchunked << std::string(endOfSize, endOfContent);

		if (m_unchunked.fail())
		{
			m_unchunked.close();
			throw HttpException::internalServerError("filestream error");
		}

		currentPos = endOfContent + 2;
	}
}

void	HttpRequest::eraseUnchunked(const std::vector<char>::iterator&	currentPos)
{
	if (currentPos > m_request.begin())
		m_request.erase(m_request.begin(), currentPos);
}

size_t	HttpRequest::chunkSize(const std::string& hex) {
	size_t	size;
	size_t	idx;

	try {
		size = stoul(hex, &idx, 16);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid chunk size");
	}

	if (idx != hex.length())
		throw HttpException::badRequest("invalid chunk size");

	return size;
}

void	HttpRequest::parseMultipart(const std::string& boundary, std::vector<mpData>& multipart)
{
	if (contentLength() < m_request.size())
		throw HttpException::badRequest("Request body size exceeds content-length");

	if (contentLength() > m_request.size())
		return;

	std::string end = "--\r\n";
	std::string	emptyLine = "\r\n\r\n";
	size_t		boundaryLen = boundary.length();

	auto	firstBoundary = std::search(m_request.begin(), m_request.end(), boundary.begin(), boundary.end());
	if (firstBoundary != m_request.begin())
		throw HttpException::badRequest("invalid multipart/form-data content 1");
	
	auto	currentPos = m_request.begin() + boundaryLen;

	while (!std::equal(currentPos, m_request.end(), end.begin(), end.end()))
	{
		currentPos += 2;

		auto	endOfHeaders = std::search(currentPos, m_request.end(), emptyLine.begin(), emptyLine.end());
		if (endOfHeaders == m_request.end())
			throw HttpException::badRequest("invalid multipart/form-data content 2");

		mpData		part;
		std::string	headers(currentPos, endOfHeaders);

		parseMultipartHeaders(headers, part);
		currentPos = endOfHeaders + 4;

		auto	endOfContent = std::search(currentPos, m_request.end(), boundary.begin(), boundary.end());
		if (endOfContent == m_request.end())
			throw HttpException::badRequest("invalid multipart/form-data content 3");

		if (currentPos != endOfContent)
		{
			if (part.filename.empty())
				part.content.insert(part.content.end(), currentPos, endOfContent - 2);

			else
			{
				std::ofstream out(std::filesystem::temp_directory_path() / part.filename, std::ios::out | std::ios::binary | std::ios::app);
				if (!out.is_open())
					throw HttpException::internalServerError("error opening a stream");

				out << std::string(currentPos, endOfContent);
				if (out.fail())
				{
					out.close();
					throw HttpException::internalServerError("filestream error");
				}

				out.close();
			}
		}

		multipart.push_back(part);
		currentPos = endOfContent + boundaryLen;
	}

	m_state = COMPLETE;
}

void	HttpRequest::parseMultipartHeaders(const std::string& headerString, mpData& part)
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

				part.filename = uniqueId() + "_" + line.substr(startPos, endPos - startPos);

				if (part.filename.find("../") != std::string::npos)	
					throw HttpException::badRequest("forbidden traversal pattern in filename");
			}
		}
		else if (line.find("content-type: ") != std::string::npos)
		{
			startPos = line.find("content-type: ") + 14;
			part.contentType = line.substr(startPos);

			if (part.contentType.find("multipart") != std::string::npos)
				parseMultipart(boundary(part.contentType), part.nestedData);
		}
		else
			throw HttpException::badRequest("invalid header for multipart/form-data");
	}
}

size_t	HttpRequest::contentLength()
{
	auto	it = m_headers.find("content-length");

	if (it == m_headers.end())
		throw HttpException::lengthRequired("content-length header missing");

	size_t	size;
	size_t	idx;

	try {
		size = std::stoul(it->second, &idx);
	} catch (std::exception& e) {
		throw HttpException::badRequest("invalid content length");
	}

	if (it->second.length() != idx)
		throw HttpException::badRequest("invalid content length");

	return size;
}

std::string	HttpRequest::boundary(std::string contentType)
{
	size_t startOfBoundary = contentType.find("boundary=");

	if (startOfBoundary == std::string::npos)
		throw HttpException::badRequest("missing boundary for multipart/form-data");
	
	return "--" + contentType.substr(startOfBoundary + 9);
}

const std::string		HttpRequest::uniqueId()
{
	auto now = std::chrono::system_clock::now();
	auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

	return std::to_string(timestamp);
}

void HttpRequest::handleDelete()
{
	std::filesystem::remove(m_path);
	m_path = "";
}

void	HttpRequest::handlePost(const std::vector<mpData>& multipart)
{
	std::vector<std::string>	uploadDir;
	std::string					directory;

	if (m_location->tryGetDirective("uploadDir", uploadDir))
	{
		directory = m_root + '/' + uploadDir.front();
	}
	else
	{
		std::vector<std::string> root;

		m_server->tryGetDirective("uploadDir", uploadDir);
		m_server->tryGetDirective("root", root);

		directory = root.front() + '/' + uploadDir.front();
	}

	if (!std::filesystem::exists(directory))
	{
		if (!std::filesystem::create_directory(directory))
			throw HttpException::internalServerError("unable to create upload directory");
	}

	std::filesystem::perms perms = std::filesystem::status(directory).permissions();

	if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
		throw HttpException::forbidden("permission denied for upload directory");

	for (mpData part: multipart)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;
			std::filesystem::path destination = directory + "/" + part.filename;
			std::filesystem::copy_file(tmpFile, destination);
		}
		if (!part.nestedData.empty())
			handlePost(part.nestedData);
	}
}

void    HttpRequest::validateMethod()
{
	if (std::find(VALID_METHODS.begin(), VALID_METHODS.end(), m_method) == VALID_METHODS.end())
		throw HttpException::notImplemented("unknown method");

    std::vector<std::string>    allowedMethods;

    if (!m_location->tryGetDirective("methods", allowedMethods))
		m_server->tryGetDirective("methods", allowedMethods);

    if (std::find(allowedMethods.begin(), allowedMethods.end(), m_method) == allowedMethods.end())
        throw HttpException::methodNotAllowed("method not allowed for the resource");
}

const std::string	HttpRequest::ePage(int code)
{
	std::vector<std::string>	root;
	std::string					errorPage;

	Logger::log("ePage: " + std::to_string(code));

	if (m_location)
	{
		errorPage = m_location->findErrorPage(code);
		if (!errorPage.empty())
		{
			if (!m_location->tryGetDirective("root", root))
				m_server->tryGetDirective("root", root);

			return root.front() + errorPage;
		}
	}

	if (!m_server)
	{
		Logger::logError("null server");
		return "";
	}

	errorPage = m_server->findErrorPage(code);
	m_server->tryGetDirective("root", root);

	return root.front() + errorPage;
}

unsigned long	HttpRequest::timeoutDuration()
{
	std::vector<std::string>	timeoutDuration;
	unsigned long				timeout;

	m_server->tryGetDirective("keepalive_timeout", timeoutDuration);
	timeout = std::stoul(timeoutDuration.front());

	return timeout;
}

bool	HttpRequest::closeConnection()
{
	if (m_headers.find("connection") != m_headers.end())
		return m_headers["connection"] == "close";

	return false;
}

const std::string&	HttpRequest::host()
{
	if (m_headers.find("host") != m_headers.end())
		return m_headers["host"];

	return EMPTY_STRING;
}

const std::vector<mpData>&	HttpRequest::multipart()
{
	return m_multipart;
}

const queryMap&	HttpRequest::query()
{
	return m_query;
}

const std::string&	HttpRequest::path()
{
	return m_path;
}

bool	HttpRequest::server()
{
	return m_server != nullptr;
}

e_state	HttpRequest::state()
{
	return m_state;
}

void	HttpRequest::setServer(ConfigNode* server)
{
	m_server = server;
}

void	HttpRequest::setState(e_state state)
{
	m_state = state;
}

bool	HttpRequest::isCgi()
{
	return m_cgi;
}

void	HttpRequest::reset()
{
	if (m_unchunked.is_open())
		m_unchunked.close();

	if (!m_unchunkedData.empty())
	{
		try {
			std::filesystem::remove(m_unchunkedData);
		} catch (std::filesystem::filesystem_error& e) {
			Logger::log("error removing unchunked data");
		}
	}

	for (mpData part: m_multipart)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;

			try {
				std::filesystem::remove(tmpFile);
			} catch (std::filesystem::filesystem_error& e) {
				Logger::log("error removing multipart data");
			}
		}
	}

	m_state = HEADERS;
	m_server = nullptr;
	m_location = nullptr;
	m_method = EMPTY_STRING;
	m_target = EMPTY_STRING;
	m_query = {};
	m_headers = {};
	m_root = EMPTY_STRING;;
	m_path = EMPTY_STRING;;
	m_cgi = false;
	m_request = {};
	m_unchunkedData = EMPTY_STRING;
	m_multipart = {};
	m_contentLength = 0;
}

HttpRequest::~HttpRequest()
{
	if (m_unchunked.is_open())
		m_unchunked.close();

	if (!m_unchunkedData.empty())
	{
		try {
			std::filesystem::remove(m_unchunkedData);
		} catch (std::filesystem::filesystem_error& e) {
			Logger::log("error removing unchunked data");
		}
	}

	for (mpData part: m_multipart)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;

			try {
				std::filesystem::remove(tmpFile);
			} catch (std::filesystem::filesystem_error& e) {
				Logger::log("error removing multipart data");
			}
		}
	}
}

int HttpRequest::prepareCgi(int client_fd, Server& server)
{
	int sockfds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockfds) == -1) {
        throw HttpException::internalServerError("socketpair");
        return 0;
    }
	
	int server_fd = server.getEpollFd();
	
	epoll_event	event;
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = sockfds[0];

	if (epoll_ctl(server_fd, EPOLL_CTL_ADD, sockfds[0], &event) == -1)
	{
		Logger::logError(std::strerror(errno));
		close(sockfds[0]);
		close(sockfds[1]);
		throw HttpException::internalServerError("epoll_ctl");
	}
	
	server.mapClientCgi(sockfds[0], client_fd);
	
	pid_t pid = fork();
	
	if (pid < 0)
	{
		close(sockfds[0]);
		close(sockfds[1]);
		throw HttpException::internalServerError("fork");
	}
	
	if (pid == 0)
	{
		std::vector<char*> 	envPtrs;
		createEnv(envPtrs, static_cast<const std::string>(m_target));
		setEnvValue("REQUEST_METHOD", static_cast<const std::string>(m_method), envPtrs);
		
		if (!m_multipart.empty())
			addData(m_multipart, envPtrs);
		else
			addQuery(m_query, envPtrs);

		close(sockfds[0]);
		run(static_cast<const std::string>(m_target), sockfds[1], envPtrs, server);
	}
	else
	{
		close(sockfds[1]);
	}

	return pid;
}
