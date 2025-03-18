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
		int							m_code;
		std::string					m_msg;
        std::string                 m_path;
		std::string					m_redir;
		std::string					m_target;
		e_method					m_method;
        bool                        m_cgi;
		size_t						m_maxSize;

        void	getLocation(HttpRequest& request, Config& config);
		void	validateRequest(HttpRequest& request);
        void	validateMethod(const std::string& method);
        void	validatePath(const std::string& target);
        void	validateCgi(const std::string& target);
		void	getMaxSize();
		void	upload(const std::vector<multipart>& multipartData);

        // HttpResponse handleGet();
		// HttpResponse handlePost(const std::vector<multipart>& multipartData);
        // HttpResponse handleDelete();

		void handlePost(const std::vector<multipart>& multipartData);
        void handleDelete();

		std::string	errorPage(int code);
		
    public:
        HttpHandler();
		~HttpHandler() = default;

		HttpHandler(const HttpHandler&) = delete;
		HttpHandler& operator=(const HttpHandler&) = delete;

        HttpResponse	handleRequest(int fd, Config& config);
		
		const std::string&	getTarget();
};
