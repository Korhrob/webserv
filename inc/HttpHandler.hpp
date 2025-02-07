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
        std::shared_ptr<ConfigNode> m_location;
        std::string                 m_path;
		e_method					m_method;
        bool                        m_cgi;

        void    		getLocation(HttpRequest& request, Config& config);
        void    		validateCgi(const std::string& target);
        void    		validateMethod(const std::string& method);
        void    		validatePath(const std::string& target);
		void			upload(const std::vector<multipart>& multipartData);

        HttpResponse handleGet();
		HttpResponse handlePost(const std::vector<multipart>& multipartData);
        HttpResponse handleDelete();

    public:
        HttpHandler();
		~HttpHandler() = default;

		HttpHandler(const HttpHandler&) = delete;
		HttpHandler& operator=(const HttpHandler&) = delete;

        HttpResponse	handleRequest(int fd, Config& config);
		
		const std::string&	getTarget();
};
