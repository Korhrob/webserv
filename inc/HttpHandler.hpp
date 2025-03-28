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
		std::string					m_root;
        std::string                 m_path;
		std::string					m_redir;
		std::string					m_target;
		e_method					m_method;
        bool                        m_cgi;
		size_t						m_maxSize;

		void	validateRequest(HttpRequest& request, std::shared_ptr<Client> client, Config& config);
        void	setLocation(HttpRequest& request, Config& config);
		void	setTimeoutDuration(std::shared_ptr<Client> client);
        void	setMethod(const std::string& method);
        void	setCgi();
        void	setPath();
		void	tryTry_files();
		void	tryIndex();
		void	tryAutoindex();
		void	setMaxSize();

		void	handlePost(const std::vector<mpData>& multipartData);
		void	upload(const std::vector<mpData>& multipartData);
        void	handleDelete();

		void	initHandler();

		std::string	ePage(int code);

		static const HttpResponse&	remoteClosedConnection();
		
    public:
        HttpHandler();
		~HttpHandler() = default;

		HttpHandler(const HttpHandler&) = delete;
		HttpHandler& operator=(const HttpHandler&) = delete;

        HttpResponse	handleRequest(std::shared_ptr<Client> client, Config& config);
		
		const std::string&	path();
};
