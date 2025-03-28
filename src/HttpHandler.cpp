#include "HttpHandler.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"
#include "CGI.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <string>

HttpHandler::HttpHandler() : m_cgi(false) {}

HttpResponse HttpHandler::handleRequest(std::shared_ptr<Client> client, Config& config)
{
	m_code = 200;
	m_msg = "OK";
	m_redir = "";

	HttpRequest request(client->fd());

    try {
		request.parseRequest();
        setLocation(request, config);
		setTimeoutDuration(client);
		validateRequest(request);
        switch (m_method)
		{
        	case GET:
				if (m_cgi)
				{
					m_cgi = false;
					std::string body = handleCGI(request.getMultipartData(), request.getQuery(), request.getTarget(), "GET");
					return HttpResponse("CGI success", body);
				}
				break;
        	case POST:
				request.parseBody(m_maxSize);
				if (m_cgi)
				{
					m_cgi = false;
					std::string body = handleCGI(request.getMultipartData(), request.getQuery(), request.getTarget(), "POST");
					return HttpResponse("CGI success", body);
				}
				handlePost(request.getMultipartData());
				break;
        	case DELETE:
            	handleDelete();
				break;
		}
    } catch (HttpException& e) {
		 // received an empty request
		if (!e.code())
			return remoteClosedConnection();

		Logger::log("code: " + std::to_string(e.code()));
		// if (!m_server) it means an exception was thrown before host was found
		// set m_server to default with port from client
		if (!m_server) // temporary so it doesn't segfault atm
			return HttpResponse(e.code(), e.what(), "www/error/400.html", e.redir(), request.getCloseConnection(), client->getTimeoutDuration());

		return HttpResponse(e.code(), e.what(), ePage(e.code()), e.redir(), request.getCloseConnection(), client->getTimeoutDuration());
    }

	// constructing response here when everything goes well
	return HttpResponse(m_code, m_msg, m_path, m_redir, request.getCloseConnection(), client->getTimeoutDuration());
}

std::string	HttpHandler::ePage(int code)
{
	std::vector<std::string>	root;
	std::string					errorPage;

	if (!m_location || !m_location->tryGetDirective("root", root))
		m_server->tryGetDirective("root", root);

	std::vector<std::string>	pages;

	if (!m_location || !m_location->tryGetDirective("error_page", pages))
	{
		root = m_server->findDirective("root");
		errorPage = m_server->findErrorPage(code);
	}
	else
	{
		errorPage = m_location->findErrorPage(code);
	}

	// double check these
	if (!root.empty())
		return root.front() + errorPage;

	return errorPage;
}

void	HttpHandler::setLocation(HttpRequest& request, Config& config)
{
    m_server = config.findServerNode(request.getHost());
    m_location = m_server->findClosestMatch(request.getTarget());

	std::vector<std::string>	redirect;

	m_location->tryGetDirective("return", redirect);

	if (!redirect.empty())
	{
		// is this a config error?
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
				throw HttpException::internalServerError("invalid timeout value"); // should this be a config error?

		} catch (std::exception& e) {
			throw HttpException::internalServerError("invalid timeout value"); // should this be a config error?
		}

		client->setTimeoutDuration(timeout);
	}
}

void	HttpHandler::validateRequest(HttpRequest& request)
{
	validateMethod(request.getMethod());
	validatePath(request.getTarget());
	setMaxSize();
}

void    HttpHandler::validateMethod(const std::string& method)
{
    std::vector<std::string>    allowedMethods;

    if (!m_location->tryGetDirective("methods", allowedMethods))
		m_server->tryGetDirective("methods", allowedMethods);

    if (std::find(allowedMethods.begin(), allowedMethods.end(), method) == allowedMethods.end())
	{
        throw HttpException::notImplemented();
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
		throw HttpException::notImplemented();
}

void    HttpHandler::validatePath(const std::string& target)
{
    validateCgi(target);

    std::vector<std::string>    root;

    if (!m_location->tryGetDirective("root", root))
	{
		// current location block doesn't have root directive
		// get one from server block directly instead
		m_server->tryGetDirective("root", root);
	}

	// saving target for later use(?)
	std::string r = root.empty() ? "" : root.front();
    m_path = r + target;
	m_target = target;

	Logger::log("root: " + r + ", target: " + target);

    try {
	
		std::vector<std::string> directives;

		// this portion became pretty big, could try to cut it up into helper functions
		if (!std::filesystem::exists(m_path))
		{

			if (!m_location->tryGetDirective("try_files", directives))
			{
				// ditto root, read above
				m_server->tryGetDirective("try_files", directives);
			}

			// first check if try_files directive exists,
			// then loop through all the directives and test them
			if (!directives.empty())
			{
				bool success = false;

				// not using reference cause we want copies of temp
				for (std::string temp : directives) {

					std::size_t pos = 0;
					// search for $url and replace it with target
					while ((pos = temp.find("$uri", pos)) != std::string::npos)
					{
						temp.replace(pos, 4, target);
						pos += target.length();
					}

					Logger::log("try_files: " + temp);
					if (std::filesystem::exists(r + temp)) {
						m_path = r + temp;
						m_target = temp;
						success = true;
						break;
					}
				}

				// if none of these passed, we have to check the try_files.back()
				// and redirect to that error code
				if (!success)
				{
					std::string str_code = directives.back();
					int e_code = std::stoi(str_code.substr(1, str_code.length()));

					Logger::log("try_files failed use ecode: " + std::to_string(e_code));

					// redirect to error code page
					// double check that this is safe

					std::vector<std::string> tmp;
					if (m_location->tryGetDirective("error_page", tmp))
					{
						m_path = r + m_location->findErrorPage(e_code);
					}
					else
					{
						m_path = r + m_server->findErrorPage(e_code);
					}

					Logger::log(m_path);
				}
			}
		}

		if (std::filesystem::exists(m_path) && std::filesystem::is_directory(m_path))
		{
			if (!m_location->tryGetDirective("index", directives))
			{
				// ditto root, read above
				m_server->tryGetDirective("index", directives);
			}

			// we didnt have try_files directive or couldn't find a match,
			// now try all index directives
			// using reference cause we dont want a copy
			for (std::string& index : directives) {

				std::string temp = m_path;

				if (target.back() != '/')
					temp += '/';

				temp += index;

				Logger::log("try index: " + temp);
				if (std::filesystem::exists(temp))
				{
					Logger::log("target has index");
					m_path = temp;
					break;
				}
			}

			// if after indexing attempts its still a directory,
			// handle it as such
			if (std::filesystem::is_directory(m_path))
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

				// could setup some autoindexing settings here,
				// but I think its out of scope
				// #autoindex_exact_size off; # optional, hide file size
				// #autoindex_localtime on; #optional, shows file timestamps in local time
			}
		}

		if (!std::filesystem::exists(m_path))
			throw HttpException::notFound();

		std::filesystem::perms perms = std::filesystem::status(m_path).permissions();

		if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none)
			throw HttpException::forbidden("permission denied");

	} catch (HttpException& e) {
		if (e.code() == 403)
			throw HttpException::forbidden(e.what());
		else
			throw HttpException::notFound();

	} catch (std::exception& e) {
		// other possible exceptions thrown by stoi() / replace() / filesystem functions
		throw HttpException::internalServerError(e.what());
	}
}

void    HttpHandler::validateCgi(const std::string& target)
{
	// Logger::log("testing some stuff\n\n");
	// std::cout << target << std::endl;
    if (target.find(".") == std::string::npos) return;

	std::string					extension(target.substr(target.find_last_of(".")));
	std::vector<std::string>	cgi;

    if (!m_location->tryGetDirective("cgi", cgi))
		m_server->tryGetDirective("cgi", cgi);

	// for (const auto& str : cgi) {std::cout << str << std::endl;}

	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_cgi = true;
}

void	HttpHandler::setMaxSize()
{
	std::vector<std::string>	maxSize;
	size_t						idx;

	if (!m_location->tryGetDirective("client_max_body_size", maxSize))
		m_server->tryGetDirective("client_max_body_size", maxSize);

	if (!maxSize.empty())
	{
		try {
			m_maxSize = std::stoul(maxSize.front(), &idx);
			if (idx != maxSize.front().length())
				throw HttpException::internalServerError("invalid max body size value"); // config error

		} catch (std::exception& e) {
			throw HttpException::internalServerError("invalid max body size value"); // config error
		}
	}
}

void HttpHandler::handlePost(const std::vector<multipart>& multipartData)
{
	// if (m_cgi)
	// 	handleCGI();
	// else
	upload(multipartData);

	m_code = 303;
	m_msg = "See Other";
	m_redir = "index";
}

void	HttpHandler::upload(const std::vector<multipart>& multipartData)
{
	std::vector<std::string>	uploadDir;
	if (!m_location->tryGetDirective("uploadDir", uploadDir))
		m_server->tryGetDirective("uploadDir", uploadDir);

	if (uploadDir.empty())
		throw HttpException::internalServerError("upload directory not defined");

	std::filesystem::perms perms = std::filesystem::status(uploadDir.front()).permissions();

	if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
		throw HttpException::forbidden("permission denied");

	if (!std::filesystem::exists(uploadDir.front()))
	{
		if (!std::filesystem::create_directory(uploadDir.front()))
			throw HttpException::internalServerError("unable to create upload directory");
	}

	for (multipart part: multipartData)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;
			std::filesystem::path destination = uploadDir.front() + "/" + part.filename;
			try {
				std::filesystem::copy_file(tmpFile, destination);
			} catch (std::filesystem::filesystem_error& e) {
				throw HttpException::internalServerError(e.what());
			}
		}
		if (!part.nestedData.empty())
			upload(part.nestedData);
	}
}

void HttpHandler::handleDelete()
{
	try {
		std::filesystem::remove(m_path);
		m_path = "";
	} catch (std::filesystem::filesystem_error& e) {
		throw HttpException::internalServerError("unable to delete target " + m_path);
	}
}

const std::string&	HttpHandler::getTarget()
{
	return m_path;
}

// singleton pattern
// declared a constant response when client closed connection
// so we can reuse the same instance multiple times
const HttpResponse&	HttpHandler::remoteClosedConnection()
{
	static const HttpResponse instance(0, "remote closed connection", "", "", 2, (t_ms)5000);
	return instance;
}
