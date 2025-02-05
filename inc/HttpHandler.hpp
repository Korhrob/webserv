#pragma once

#include "Config.hpp"
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
        int                         m_fd;
        std::shared_ptr<ConfigNode> m_location;
        bool                        m_isCGI;
        std::string                 m_path;
		e_method					m_method;

        void    		getLocation(HttpRequest& request, Config& config);
        void    		validateCgi(const std::string& target);
        void    		validateMethod(const std::string& method);
        void    		validatePath(const std::string& target);
		void			upload(const std::vector<multipart>& multipartData);
		std::ofstream	getFileStream(std::string filename);

        HttpResponse handleGet();
        HttpResponse handlePost(const std::vector<multipart>& multipartData);
        HttpResponse handleDelete();

    public:
        HttpHandler(int fd);

        HttpResponse    handleRequest(Config& config);
};
