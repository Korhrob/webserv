#include "HttpHandler.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <string>

HttpHandler::HttpHandler() : m_cgi(false) {}

// HttpResponse HttpHandler::handleRequest(int fd, Config& config)
// {
// 	HttpRequest request(fd);

//     try {
// 		request.parseRequest();
//         getLocation(request, config);
// 		validateRequest(request);
//         switch (m_method)
// 		{
//         	case GET:
// 				// if (m_cgi)
// 				// 	return handleCGI();
//             	return handleGet();
//         	case POST:
// 				request.parseBody(m_maxSize);
// 				// if (m_cgi)
// 					// 	return handleCGI();
//             	return handlePost(request.getMultipartData());
//         	case DELETE:
//             	return handleDelete();
// 			default:
// 				return HttpResponse(500, "FAIL", "", "", true); // robert
// 		}
//     } catch (HttpException& e) {
// 		if (!m_server)
// 			return HttpResponse(e.code(), e.what(), "www/error/400.html", e.target(), true);
//         return HttpResponse(e.code(), e.what(), errorPage(e.code()), e.target(), request.closeConnection());
//     }
// }

// HttpResponse HttpHandler::handleRequest(int fd, Config& config)
// {
// 	HttpRequest request(fd);

//     try {
// 		request.parseRequest();
//         getLocation(request, config);
// 		validateRequest(request);
//         switch (m_method)
// 		{
//         	case GET:
// 				// if (m_cgi)
// 				// 	return handleCGI();
//             	return handleGet();
//         	case POST:
// 				request.parseBody(m_maxSize);
// 				// if (m_cgi)
// 					// 	return handleCGI();
//             	return handlePost(request.getMultipartData());
//         	case DELETE:
//             	return handleDelete();
// 			default:
// 				return HttpResponse(500, "FAIL", "", "", true); // robert
// 		}
//     } catch (HttpException& e) {
// 		if (!m_server)
// 			return HttpResponse(e.code(), e.what(), "www/error/400.html", e.target(), true);
//         return HttpResponse(e.code(), e.what(), errorPage(e.code()), e.target(), request.closeConnection());
//     }
// }

HttpResponse HttpHandler::handleRequest(std::shared_ptr<Client> client, Config& config)
{
	m_code = 200;
	m_msg = "OK";
	m_redir = "";
	HttpRequest request(client->fd());

    try {
		request.parseRequest();
        setLocation(request, config);
		validateRequest(request);
        switch (m_method)
		{
        	case GET:
				// if (m_cgi)
				// 	handleCGI();
				break;
        	case POST:
				request.parseBody(m_maxSize);
            	handlePost(request.getMultipartData());
				break;
        	case DELETE:
            	handleDelete();
				break;
		}
    } catch (HttpException& e) {
		if (e.code() != 0)
        	return HttpResponse(e.code(), e.what(), errorPage(e.code()), e.redir(), request.closeConnection());
    }

	// if e.code() = 0
	// return remoteClosedConnection(); // robert
	return HttpResponse(m_code, m_msg, m_path, m_redir, request.closeConnection());
}

std::string	HttpHandler::errorPage(int code)
{
	std::vector<std::string>	root;
	std::string					errorPage;

	if (!m_location->tryGetDirective("root", root))
		m_server->tryGetDirective("root", root);

	std::vector<std::string>	pages;
	if (m_location->tryGetDirective("error_page", pages))
	{
		errorPage = m_location->findErrorPage(code);
	}
	else
	{
		errorPage = m_server->findErrorPage(code);
	}
	// errorPage = m_server->findErrorPage(code); // robert

	// double check these
	if (!root.empty() && !errorPage.empty())
		return root.front() + errorPage;

	if (!errorPage.empty())
		return errorPage;

	return EMPTY_STRING;
}

void	HttpHandler::setLocation(HttpRequest& request, Config& config)
{
    m_server = config.findServerNode(request.getHost());
    m_location = m_server->findClosestMatch(request.getTarget());

	std::vector<std::string>	redirect;

	m_location->tryGetDirective("return", redirect);

	if (!redirect.empty())
	{
		if (redirect.size() != 2)
			throw HttpException::internalServerError("invalid redirect"); // config error?
		throw HttpException::temporaryRedirect(redirect[1]);
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
    if (target.find(".") == std::string::npos) return;

	std::string					extension(target.substr(target.find_last_of(".")));
	std::vector<std::string>	cgi;

    if (!m_location->tryGetDirective("cgi", cgi))
		m_server->tryGetDirective("cgi", cgi);

	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_cgi = true;
}

void	HttpHandler::setMaxSize()
{
	std::vector<std::string> maxSize;

	if (!m_location->tryGetDirective("client_max_body_size", maxSize))
		m_server->tryGetDirective("client_max_body_size", maxSize);

	if (!maxSize.empty())
	{
		try {
			m_maxSize = std::stoul(maxSize.front());
		} catch (std::exception& e) {
			throw HttpException::internalServerError("failed to get max body size");
		}
	}
}

// HttpResponse HttpHandler::handleGet()
// {
// 	// if file validation for directory listing is handled ealier,
// 	// we dont have to check if this is a directory
// 	// could double check that file exists,
// 	// but that seems to be handled earlier

// 	// if (std::filesystem::is_directory(m_path))
// 	// {
// 	// 	if (m_location->autoindexOn()) // robert
// 	// 	{
// 	// 		Logger::log("autoindex: on; target is a directory: " + m_path + ":" + m_target);
// 	// 		return HttpResponse(200, "OK", m_path, m_target);
// 	// 	}
// 	// 	throw HttpException::forbidden();
// 	// }

// 	return HttpResponse(200, "OK", m_path, m_target);
// }

// HttpResponse HttpHandler::handlePost(const std::vector<multipart>& multipartData)
// {
// 	upload(multipartData);

// 	return HttpResponse(303, "See Other", "", "index");
// }

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
		throw HttpException::internalServerError("upload directory not defined"); // is this a config error?
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

// HttpResponse HttpHandler::handleDelete()
// {
// 	try {
// 		if (std::filesystem::is_directory(m_path))
// 			throw HttpException::forbidden("trying to delete a directory");
// 		std::filesystem::remove(m_path);
// 		return HttpResponse(200, "OK");
// 	} catch (std::filesystem::filesystem_error& e) {
// 		throw HttpException::internalServerError("unable to delete target " + m_path);
// 	}
// }

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
	static const HttpResponse instance(0, "remote closed connection", "", "", true);
	return instance;
}