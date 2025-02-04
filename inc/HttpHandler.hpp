#pragma once

#include "Config.hpp"
#include "HttpResponse.hpp"

#include <memory>
#include <string>

class HttpHandler {
    private:
        int                         m_fd;
        Config&                     m_config;
        std::shared_ptr<ConfigNode> m_location;
        bool                        m_isCGI;
        std::string                 m_path;

        void    getLocation(HttpRequest& request);
        void    validateCgi(const std::string& target);
        void    validateMethod(const std::string& method);
        void    validatePath(const std::string& target);
        void    validateLocation(HttpRequest& request);

        HttpResponse handleGet();
        // const HttpResponse& handlePost();
        // const HttpResponse& handleDelete();

    public:
        HttpHandler(int fd, Config& config);

        HttpResponse    handleRequest();
};
