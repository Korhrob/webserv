#include "HttpHandler.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"
#include "CGI.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <string>

HttpHandler::HttpHandler() : m_cgi(false) {}

HttpResponse HttpHandler::handleRequest(int fd, Config& config)
{
	HttpRequest request(fd);

    try {
		request.parseRequest();
        getLocation(request, config);
		validateRequest(request);
        switch (m_method)
		{
        	case GET:
				if (m_cgi)
				{
					m_cgi = false;
					return handleCGI(request.getMultipartData(), request.getQuery(), request.getTarget(), "GET");
				}
            	return handleGet();
        	case POST:
				request.parseBody(m_maxSize);
				if (m_cgi)
				{
					m_cgi = false;
					return handleCGI(request.getMultipartData(), request.getQuery(), request.getTarget(), "POST");
				}
            	return handlePost(request.getMultipartData());
        	case DELETE:
            	return handleDelete();
		}
    } catch (HttpException& e) {
		if (!m_server)
			return HttpResponse(e.code(), e.what(), "www/error/400.html", e.target(), true);
        return HttpResponse(e.code(), e.what(), getErrorPage(e.code()), e.target(), request.closeConnection());
    }

	return HttpResponse(500, "FAIL", "", "", true); // robert
}

std::string	HttpHandler::getErrorPage(int code)
{
	std::vector<std::string>	root;
	std::string					errorPage;

	m_location->tryGetDirective("root", root);
	errorPage = m_server->getErrorPage(code);

	if (!root.empty() && !errorPage.empty())
		return root[0] + errorPage;
	if (!errorPage.empty())
		return errorPage;
	return EMPTY_STRING;
}

void	HttpHandler::getLocation(HttpRequest& request, Config& config)
{
    m_server = config.findServerNode(request.getHost());

	// technically should never happen, as findServerNode would never return null
    // if (m_server == nullptr) // temp
	// 	throw HttpException::badRequest("Server node is null");
	
    m_location = m_server->findClosestMatch(request.getTarget());
	
	std::vector<std::string>	redirect;
	m_location->tryGetDirective("return", redirect);
	if (!redirect.empty())
		throw HttpException::temporaryRedirect(redirect[1]); // is this safe? checked in config parsing?
}

void	HttpHandler::validateRequest(HttpRequest& request)
{
	validateMethod(request.getMethod());
	validatePath(request.getTarget());
	getMaxSize();
}

void    HttpHandler::validateMethod(const std::string& method)
{
    std::vector<std::string>    allowedMethods;

    m_location->tryGetDirective("methods", allowedMethods);
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
						m_path = r + m_location->getErrorPage(e_code);
					}
					else
					{
						m_path = r + m_server->getErrorPage(e_code);
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
					throw HttpException::forbidden();
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

		if ((perms & std::filesystem::perms::others_read) == std::filesystem::perms::none)
			throw HttpException::forbidden();

	} catch (std::exception& e) {

		// shouldn't always throw not found!!
		// what about forbidden?
		throw HttpException::notFound();

	}
}

void    HttpHandler::validateCgi(const std::string& target)
{
	// Logger::log("testing some stuff\n\n");
	// std::cout << target << std::endl;
    if (target.find(".") == std::string::npos) return;

	std::string					extension(target.substr(target.find_last_of(".")));
	std::vector<std::string>	cgi;

    m_location->tryGetDirective("cgi", cgi);

	// for (const auto& str : cgi) {std::cout << str << std::endl;}

	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_cgi = true;
}

void	HttpHandler::getMaxSize()
{
	std::vector<std::string> maxSize;
	m_server->tryGetDirective("client_max_body_size", maxSize);

	if (!maxSize.empty()) // should a default value be set in a .hpp?
	{
		try {
			m_maxSize = std::stoul(maxSize.front());
		} catch (std::exception& e) {
			throw HttpException::internalServerError("failed to get max body size");
		}
	}
}

HttpResponse HttpHandler::handleGet()
{
	// if file validation for directory listing is handled ealier,
	// we dont have to check if this is a directory
	// could double check that file exists,
	// but that seems to be handled earlier

	// if (std::filesystem::is_directory(m_path))
	// {
	// 	if (m_location->autoindexOn()) // robert
	// 	{
	// 		Logger::log("autoindex: on; target is a directory: " + m_path + ":" + m_target);
	// 		return HttpResponse(200, "OK", m_path, m_target);
	// 	}
	// 	throw HttpException::forbidden();
	// }

	return HttpResponse(200, "OK", m_path, m_target);
}

HttpResponse HttpHandler::handlePost(const std::vector<multipart>& multipartData)
{
	upload(multipartData);
	
	return HttpResponse(200, "OK");
}

void	HttpHandler::upload(const std::vector<multipart>& multipartData)
{
	std::vector<std::string>	uploadDir;
	m_location->tryGetDirective("uploadDir", uploadDir);

	
	if (uploadDir.empty())
		throw HttpException::internalServerError("upload directory not defined"); // is this a config error?
	std::filesystem::perms perms = std::filesystem::status(uploadDir[0]).permissions();
	if ((perms & std::filesystem::perms::others_write) == std::filesystem::perms::none)
		throw HttpException::forbidden();

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

HttpResponse HttpHandler::handleDelete()
{
	try {
		if (std::filesystem::is_directory(m_path))
			throw HttpException::forbidden();
		std::filesystem::remove(m_path);
		return HttpResponse(200, "OK");
	} catch (std::filesystem::filesystem_error& e) {
		throw HttpException::internalServerError("unable to delete target " + m_path);
	}
}

const std::string&	HttpHandler::getTarget()
{
	return m_path;
}
