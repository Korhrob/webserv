#include "HttpHandler.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>

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
            	return handleGet();
        	case POST:
				request.parseBody(m_maxSize);
            	return handlePost(request.getMultipartData());
        	case DELETE:
            	return handleDelete();
		}
    } catch (HttpException& e) {
        return HttpResponse(e.getStatusCode(), e.what());
    }

	// function has to always return something
	return HttpResponse(0, ""); // Robert
}

void	HttpHandler::getLocation(HttpRequest& request, Config& config)
{
    m_server = config.findServerNode(request.getHost());
    if (m_server == nullptr) // temp
        throw HttpException::badRequest("Server node is null");
	
    m_location = m_server->findClosestMatch(request.getTarget());
}

void	HttpHandler::validateRequest(HttpRequest& request)
{
	validateMethod(request.getMethod());
	validatePath(request.getTarget());
	getMaxSize();
}

void    HttpHandler::validateMethod(const std::string& method)
{
	{
    std::vector<std::string>    methods;

    m_location->tryGetDirective("methods", methods);
    if (std::find(methods.begin(), methods.end(), method) == methods.end())
        throw HttpException::notImplemented();
	}

	std::unordered_map<std::string, e_method>	methods = {{"GET", GET}, {"POST", POST}, {"DELETE", DELETE}};
	for (auto [key, value]: methods)
	{
		if (key == method)
			m_method = methods[key];
	}
}

void    HttpHandler::validatePath(const std::string& target)
{
    validateCgi(target);

    std::vector<std::string>    root;

    m_location->tryGetDirective("root", root);
    m_path = root.empty() ? target : root.front() + target; // error ?

    try { // fix this - Set a default file to answer if the request is a directory
		if (!std::filesystem::exists(m_path) || std::filesystem::is_directory(m_path))
		{
			std::vector<std::string> indices;
			m_location->tryGetDirective("index", indices);
			for (std::string index: indices) {
				std::string newPath = m_path + index;
				if (std::filesystem::exists(newPath))
				{
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

void    HttpHandler::validateCgi(const std::string& target)
{
    if (target.find(".") == std::string::npos) return;

	std::string					extension(target.substr(target.find_last_of(".")));
	std::vector<std::string>	cgi;

    m_location->tryGetDirective("cgi", cgi);

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
	if (std::filesystem::is_directory(m_path))
		throw HttpException::forbidden();

	return HttpResponse(200, "OK", m_path);
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
		throw HttpException::forbidden(); // what's the correct error when uploadDir is not defined? is this a config error?

	for (multipart part: multipartData)
	{
		if (!part.filename.empty())
		{
			std::filesystem::path tmpFile = std::filesystem::temp_directory_path() / part.filename;
			std::filesystem::path destination = uploadDir.front() + "/" + part.filename;
			try {
				std::filesystem::copy_file(tmpFile, destination);
			} catch (std::filesystem::filesystem_error& e) {
				throw HttpException::internalServerError("error uploading file");
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
