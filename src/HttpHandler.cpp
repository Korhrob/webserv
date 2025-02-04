#include "HttpHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"

HttpHandler::HttpHandler(int fd, Config& config) : m_fd(fd), m_config(config) {}

HttpResponse HttpHandler::handleRequest()
{
    try {
        HttpRequest request(m_fd);
        getLocation(request);
        validateLocation(request);
        std::string method(request.getMethod());
        if (method == "GET")
            return handleGet();
        // if (method == "POST")
        //     return handlePost();
        // if (method == "DELETE")
        //     return handleDelete();
        return handleGet();
    } catch (HttpException& e) {
        return HttpResponse(e.getStatusCode(), e.what());
    }
}

void HttpHandler::getLocation(HttpRequest& request) // temp
{
    std::shared_ptr<ConfigNode> serverNode = m_config.findServerNode(request.getHost());
    if (serverNode == nullptr)
        throw HttpException::badRequest("Server node is null");
    m_location = serverNode->findClosestMatch(request.getTarget());
}

void    HttpHandler::validateCgi(const std::string& target)
{
    if (target.find(".") == std::string::npos)
        return;
	
	std::string					extension(target.substr(target.find_last_of(".")));
	std::vector<std::string>	cgi;

    m_location->tryGetDirective("cgi", cgi);
	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_isCGI = true;
}

void    HttpHandler::validateLocation(HttpRequest& request)
{
    validateMethod(request.getMethod());
    validatePath(request.getTarget());
}

void    HttpHandler::validateMethod(const std::string& method)
{
    std::vector<std::string>    methods;

    m_location->tryGetDirective("methods", methods);
    if (std::find(methods.begin(), methods.end(), method) == methods.end())
        throw HttpException::notImplemented();
}

void    HttpHandler::validatePath(const std::string& target)
{
    validateCgi(target);

    std::vector<std::string>    root;

    m_location->tryGetDirective("root", root);
    m_path = root.empty() ? target : root.front() + target;

    try { // fix this - Set a default file to answer if the request is a directory
		if (!std::filesystem::exists(m_path) || std::filesystem::is_directory(m_path)) {
			std::vector<std::string> indices;
			m_location->tryGetDirective("index", indices);
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

HttpResponse HttpHandler::handleGet()
{
    return HttpResponse(200, "OK", m_path);
}

// const HttpResponse& HttpHandler::handlePost()
// {

// }

// const HttpResponse& HttpHandler::handleDelete()
// {

// }
