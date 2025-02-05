#include "HttpHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpException.hpp"
#include "ConfigNode.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>

HttpHandler::HttpHandler(int fd) : m_fd(fd), m_isCGI(false) {}

HttpResponse HttpHandler::handleRequest(Config& config)
{
    try {
        HttpRequest request(m_fd);
        getLocation(request, config);
        switch (m_method) {
        	case GET:
            	return handleGet();
        	case POST:
            	return handlePost(request.getMultipartData());
        	case DELETE:
            	return handleDelete();
		}
    } catch (HttpException& e) {
        return HttpResponse(e.getStatusCode(), e.what());
    }
}

void	HttpHandler::getLocation(HttpRequest& request, Config& config)
{
    std::shared_ptr<ConfigNode> serverNode = config.findServerNode(request.getHost());
    if (serverNode == nullptr) // temp
        throw HttpException::badRequest("Server node is null");

    m_location = serverNode->findClosestMatch(request.getTarget());

	validateMethod(request.getMethod());
	validatePath(request.getTarget());
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
	for (auto [key, value]: methods) {
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

void    HttpHandler::validateCgi(const std::string& target)
{
    if (target.find(".") == std::string::npos) return;

	std::string					extension(target.substr(target.find_last_of(".")));
	std::vector<std::string>	cgi;

    m_location->tryGetDirective("cgi", cgi);

	if (std::find(cgi.begin(), cgi.end(), extension) != cgi.end())
		m_isCGI = true;
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

void	HttpHandler::upload(const std::vector<multipart>& multipartData)
{	
	for (multipart part: multipartData) {
		if (!part.filename.empty()) {
			std::ofstream file = getFileStream(part.filename);
			file.write(part.content.data(), part.content.size());
		}
		if (!part.nestedData.empty())
			upload(part.nestedData);
	}
}

std::ofstream	HttpHandler::getFileStream(std::string filename)
{
	if (filename.find("../") != std::string::npos)	
		throw HttpException::badRequest("forbidden traversal pattern in filename");
	
	std::string					filePath;
	std::vector<std::string>	uploadDir;

	m_location->tryGetDirective("uploadDir", uploadDir); // what if there is no directory set in config file
	filePath = uploadDir.empty() ? filename : uploadDir.front() + '/' + filename; // what if there are multiple files with the same name

	std::ofstream filestream(filePath, std::ios::out | std::ios::binary);
	if (!filestream)
		throw HttpException::internalServerError("open failed for file upload");
			
	return filestream;
}
