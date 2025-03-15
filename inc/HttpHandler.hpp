#pragma once

#include "Config.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <memory>
#include <string>

enum	e_method
{
	GET,
	POST,
	DELETE
};

class HttpHandler {
    private:
		std::shared_ptr<ConfigNode> m_server;
        std::shared_ptr<ConfigNode> m_location;
        std::string                 m_path;
		std::string					m_target; // robert
		e_method					m_method;
        bool                        m_cgi;
		size_t						m_maxSize;

        void	setLocation(HttpRequest& request, Config& config);
		void	validateRequest(HttpRequest& request);
        void	validateMethod(const std::string& method);
        void	validatePath(const std::string& target);
        void	validateCgi(const std::string& target);
		void	setMaxSize();
		void	upload(const std::vector<multipart>& multipartData);

        HttpResponse handleGet();
		HttpResponse handlePost(const std::vector<multipart>& multipartData);
        HttpResponse handleDelete();
		
		std::string	getErrorPage(int code);
		static const HttpResponse&	remoteClosedConnection();
		
    public:
        HttpHandler();
		~HttpHandler() = default;

		HttpHandler(const HttpHandler&) = delete;
		HttpHandler& operator=(const HttpHandler&) = delete;

        HttpResponse	handleRequest(std::shared_ptr<Client> client, Config& config);
		
		const std::string&	getTarget();
};
