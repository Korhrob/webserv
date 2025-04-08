#pragma once

#include <string>
#include <vector>

static const unsigned int BUFFER_SIZE = 1024;
static const unsigned int PACKET_SIZE = 8192; // 8 KB
static const unsigned int EPOLL_POOL = 128;
static const unsigned int URI_MAX = 8192;
static const unsigned int HEADERS_MAX = 32000;
static const unsigned int TIMEOUT_CGI = 8;

#ifndef LOG_ENABLE
# define LOG_ENABLE true
#endif
#ifndef LOG_STDOUT
# define LOG_STDOUT
#endif

const std::string				WHITESPACE = " \t";
const std::string				EMPTY_STRING;
const std::vector<std::string>	EMPTY_VECTOR;
const std::vector<std::string>	MANDATORY_DIRECTIVES { "listen", "server_name" };
const std::vector<std::string>	VALID_METHODS { "GET", "POST", "DELETE" };
const std::string				RESPONSE_TIMEOUT = "HTTP/1.1 408 Request Timeout\r\nConnection: close\r\n\r\n";
const std::vector<int>			ERROR_CODES { 400, 401, 402, 403, 404, 500 };

// because this list is growing quite large,
// might be better to convert it to unordered_set
const std::vector<std::string>	VALID_DIRECTIVES { "listen", "host", "server_name", "index", "root", "methods", "cgi", "uploadDir",
													"client_max_body_size", "access_log", "error_log", "worker_connections",
													"error_page", "autoindex", "cgi_path", "cgi_ext", "return", "alias", "try_files",
													"keepalive_timeout" };
