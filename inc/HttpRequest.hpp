#pragma once

#include "Const.hpp"
#include "Logger.hpp"
#include "Config.hpp"

#include <vector>
#include <string>
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <unordered_map>

class Client;
class HttpResponse;

enum	e_state
{
	COMPLETE,
	HEADERS,
	CHUNKED,
	MULTIPART
};

enum	e_method
{
	GET,
	POST,
	DELETE
};

struct	mpData {
	std::string				name;
	std::string				filename;
	std::string				contentType;
	std::vector<char>		content;
	std::vector<mpData>	nestedData;
};

using queryMap = std::unordered_map<std::string, std::vector<std::string>>;
using t_ms = std::chrono::milliseconds;

class HttpRequest {
	private:
		e_state											m_state;
		std::shared_ptr<ConfigNode>						m_server;
		std::shared_ptr<ConfigNode>						m_location;
		std::string										m_method;
		std::string										m_target;
		queryMap										m_query;
		std::unordered_map<std::string, std::string>	m_headers;
		std::string										m_root;
		std::string										m_path;
		bool											m_cgi;
		size_t											m_maxSize;
		std::vector<char>								m_request;
		int												m_unchunked;
		std::vector<mpData>								m_multipart;
		size_t											m_contentLength;

		void							parseRequestLine(std::istringstream& request);
		void							parseURI();
		void							decodeURI();
		void							parseQueryString();
		void							parseHeaders(std::istringstream& request);
		void							setLocation(Config& config);
		void							setCgi();
		void							setPath();
		void							tryTry_files();
		void							tryIndex();
		void							tryAutoindex();
		void							setMaxSize();
		void							setBodyType();
		void							parseChunked();
		size_t							chunkSize(std::string& hex);
		void							parseMultipart(std::string boundary, std::vector<mpData>& multipartData);
		void							parseMultipartHeaders(std::string& headerString, mpData& part);
		size_t							contentLength();
		std::string						boundary(std::string& contentType);
		std::string						uniqueId();
		void							handleDelete();
		void							handlePost(const std::vector<mpData>& multipart);
		const std::string&				host();
		e_method    					method();
		const queryMap&					query();
		const std::vector<mpData>&		multipart();

	public:
		HttpRequest();
		~HttpRequest();

		HttpRequest(const HttpRequest&) = delete;
		HttpRequest&	operator=(const HttpRequest&) = default;

		void							reset();
		void							appendRequest(std::vector<char>& request);
		void							parseRequest(Config& config);
		HttpResponse					processRequest(t_ms timeout);
		bool							closeConnection();
		int								timeoutDuration();
		void							setServer(std::shared_ptr<ConfigNode>);
		void							setState(e_state state);
		bool							server();
		e_state							state();
		std::string						path();
		std::string						ePage(int code);


};
