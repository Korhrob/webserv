#include "HttpHandler.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"
#include "CGI.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <regex>

HttpHandler::HttpHandler() {}

HttpResponse HttpHandler::handleRequest(std::shared_ptr<Client> client, Config& config)
{
	initHandler();

	t_ms		td = client->getTimeoutDuration();
	HttpRequest request(client->fd());

    try {
		request.parseRequest();
		validateRequest(request, client, config);
        switch (m_method)
		{
        	case GET:
				if (m_cgi)
					return HttpResponse(handleCGI(request.multipart(), request.query(), request.target(), "GET"), td);
				break;
        	case POST:
				request.parseBody(m_maxSize);
				if (m_cgi)
					return HttpResponse(handleCGI(request.multipart(), request.query(), request.target(), "POST"), td);
				handlePost(request.multipart());
				break;
        	case DELETE:
            	handleDelete();
				break;
		}
    } catch (HttpException& e) {
		 // client closed connection
		if (!e.code())
			return remoteClosedConnection();

		// an exception was thrown before host was found
		if (!m_server)
			m_server = config.findServerNode("127.0.0.1:" + std::to_string(client->port()));

		// constructing error response
		return HttpResponse(e.code(), e.what(), ePage(e.code()), e.redir(), request.closeConnection(), td);
    } catch (...) {
		// backup catch for any exception that might be thrown and isn't caught elsewhere
		return HttpResponse(500, "Internal Server Error: unknown error occurred", ePage(500), "", true, td);
	}

	// constructing success response
	return HttpResponse(m_code, m_msg, m_path, m_redir, request.closeConnection(), client->getTimeoutDuration());
}

void	HttpHandler::validateRequest(HttpRequest& request, std::shared_ptr<Client> client, Config& config)
{
	m_target = request.target();

	setLocation(request, config);
	setTimeoutDuration(client);
	setMethod(request.method());
	setCgi();
	setPath();
	setMaxSize();
}

void	HttpHandler::setLocation(HttpRequest& request, Config& config)
{
    m_server = config.findServerNode(request.host());
    m_location = m_server->findClosestMatch(m_target);

	std::vector<std::string>	redirect;

	m_location->tryGetDirective("return", redirect);

	if (!redirect.empty())
	{
		// config error?
		// should it be checked during config parsing that return directive always has 2 values?
		if (redirect.size() != 2)
			throw HttpException::internalServerError("invalid redirect");
		throw HttpException::temporaryRedirect(redirect[1]);
	}
}

void	HttpHandler::setTimeoutDuration(std::shared_ptr<Client> client)
{
	std::vector<std::string>	timeoutDuration;
	size_t						idx;
	int							timeout;

	if (m_server->tryGetDirective("keepalive_timeout", timeoutDuration))
	{
		try {
			timeout = std::stoi(timeoutDuration.front(), &idx);
			if (idx != timeoutDuration.front().length())
				throw HttpException::internalServerError("invalid timeout value"); // config error?

		} catch (std::exception& e) {
			throw HttpException::internalServerError("invalid timeout value"); // config error?
		}

		client->setTimeoutDuration(timeout);
	}
}

void    HttpHandler::setMethod(const std::string& method)
{
    std::vector<std::string>    allowedMethods;

    if (!m_location->tryGetDirective("methods", allowedMethods))
		m_server->tryGetDirective("methods", allowedMethods);

    if (std::find(allowedMethods.begin(), allowedMethods.end(), method) == allowedMethods.end())
	{
        throw HttpException::notImplemented("requested method not implemented");
	}

	static const std::unordered_map<std::string, e_method>	methodMap = {
		{"GET", GET},
		{"POST", POST},
		{"DELETE", DELETE}
	};

	auto it = methodMap.find(method);
	if (it != methodMap.end())
		m_method = it->second;
	else
		throw HttpException::notImplemented("requested method not implemented");
}

void    HttpHandler::setCgi()
{
    if (m_target.find(".") == std::string::npos) return;

	std::string					extension(m_target.substr(m_target.find_last_of(".")));
	std::vector<std::string>	cgi;

    if (!m_location->tryGetDirective("cgi", cgi))
		m_server->tryGetDirective("cgi", cgi);

	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_cgi = true;
}

void    HttpHandler::setPath()
{
	std::vector<std::string> root;

    if (!m_location->tryGetDirective("root", root))
		m_server->tryGetDirective("root", root);

	m_root = root.front();
    m_path = m_root + m_target;

	Logger::log("root: " + m_root + ", target: " + m_target);

	if (!std::filesystem::exists(m_path))
		tryTry_files();

	if (std::filesystem::exists(m_path) && std::filesystem::is_directory(m_path))
	{
		tryIndex();

		// if after indexing attempts its still a directory,
		// handle it as such
		if (std::filesystem::is_directory(m_path))
			tryAutoindex();
	}

	if (!std::filesystem::exists(m_path))
		throw HttpException::notFound("resource could not be found");

	std::filesystem::perms perms = std::filesystem::status(m_path).permissions();

	if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none)
		throw HttpException::forbidden("permission denied for requested resource");
}

void	HttpHandler::tryTry_files()
{
	std::vector<std::string>	try_files;

	if (!m_location->tryGetDirective("try_files", try_files))
			m_server->tryGetDirective("try_files", try_files);

		// first check if try_files directive exists,
		// then loop through all the directives and test them
		if (!try_files.empty())
		{
			bool success = false;

			for (std::string temp : try_files) {

				std::size_t pos = 0;
				// search for $url and replace it with target
				while ((pos = temp.find("$uri", pos)) != std::string::npos)
				{
					temp.replace(pos, 4, m_target);
					pos += m_target.length();
				}

				Logger::log("try_files: " + temp);
				if (std::filesystem::exists(m_root + temp)) {
					m_path = m_root + temp;
					m_target = temp;
					success = true;
					break;
				}
			}

			// if none of these passed, we have to check the try_files.back()
			// and redirect to that error code
			if (!success)
			{
				std::regex errorValue("^=\\d{3}$");

				if (std::regex_match(try_files.back(), errorValue))
				{
					int code = std::stoi(try_files.back().substr(1));

					Logger::log("try_files failed use ecode: " + std::to_string(code));
					throw HttpException::withCode(code);
				}
			}
		}
}

void	HttpHandler::tryIndex()
{
	std::vector<std::string>	index;

	if (!m_location->tryGetDirective("index", index))
		m_server->tryGetDirective("index", index);

	// we didnt have try_files directive or couldn't find a match,
	// now try all index directives
	for (std::string& index : index) {

		std::string temp = m_path;

		if (temp.back() != '/')
			temp += '/';

		temp += index;

		Logger::log("try index: " + temp);
		if (std::filesystem::exists(temp))
		{
			Logger::log("target has index");
			m_path = temp;
			return;
		}
	}
}

void	HttpHandler::tryAutoindex()
{
	if (m_path.back() != '/')
		m_path += "/";

	if (m_target.back() != '/')
		m_target += "/";

	// if autoindexing is off, throw forbidden
	if (!m_location->autoindexOn())
	{
		Logger::log("autoindex: off, but trying to access directory");
		throw HttpException::forbidden("autoindex: off, but trying to access directory");
	}
}

void	HttpHandler::setMaxSize()
{
	std::vector<std::string>	maxSize;
	size_t						idx;

	if (!m_location->tryGetDirective("client_max_body_size", maxSize))
		m_server->tryGetDirective("client_max_body_size", maxSize);

	if (!maxSize.empty()) // config error?
	{
		m_maxSize = std::stoul(maxSize.front(), &idx);
		if (idx != maxSize.front().length())
			throw HttpException::internalServerError("invalid max body size value"); // config error
	}
}

void	HttpHandler::handlePost(const std::vector<mpData>& multipartData)
{
	std::vector<std::string>	uploadDir;

	if (!m_location->tryGetDirective("uploadDir", uploadDir))
		m_server->tryGetDirective("uploadDir", uploadDir);

	if (uploadDir.empty()) // config error
		throw HttpException::internalServerError("upload directory not defined");

	if (!std::filesystem::exists(uploadDir.front()))
	{
		if (!std::filesystem::create_directory(uploadDir.front()))
			throw HttpException::internalServerError("unable to create upload directory");
	}

	std::filesystem::perms perms = std::filesystem::status(uploadDir.front()).permissions();

	if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
		throw HttpException::forbidden("permission denied for upload directory");

	for (mpData part: multipartData)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;
			std::filesystem::path destination = uploadDir.front() + "/" + part.filename;
			std::filesystem::copy_file(tmpFile, destination);
		}
		if (!part.nestedData.empty())
			handlePost(part.nestedData);
	}
}

void HttpHandler::handleDelete()
{
	std::filesystem::remove(m_path);
	m_path = "";
}

void HttpHandler::initHandler()
{
	m_server = nullptr;
	m_location = nullptr;
	m_code = 200;
	m_msg = "OK";
	m_path = "";
	m_redir = "";
	m_target = "";
	m_cgi = false;
}

std::string	HttpHandler::ePage(int code)
{
	std::vector<std::string>	root;
	std::string					errorPage;

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

	errorPage = m_server->findErrorPage(code);
	m_server->tryGetDirective("root", root);

	return root.front() + errorPage;
}

const std::string&	HttpHandler::path()
{
	return m_path;
}

// singleton pattern
// declared a constant response when client closed connection
// so we can reuse the same instance multiple times
const HttpResponse&	HttpHandler::remoteClosedConnection()
{
	static const HttpResponse instance(0, "Remote Closed Connection", "", "", 2, (t_ms)5000);
	return instance;
}
