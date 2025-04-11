#pragma once

#include <string>
#include <vector>

static const unsigned int BUFFER_SIZE = 1024;
static const unsigned int PACKET_SIZE = 8192;
static const unsigned int EPOLL_POOL = 128;
static const unsigned int URI_MAX = 8192;
static const unsigned int HEADERS_MAX = 32000;

#ifndef LOG_ENABLE
# define LOG_ENABLE true
#endif
#ifndef LOG_STDOUT
# define LOG_STDOUT
#endif

const std::string				INTERPRETER = "/usr/bin/php";
const std::string				WHITESPACE = " \t";
const std::string				EMPTY_STRING;
const std::vector<std::string>	EMPTY_VECTOR;
const std::vector<std::string>	MANDATORY_DIRECTIVES { "listen", "server_name" };
const std::vector<std::string>	VALID_METHODS { "GET", "POST", "DELETE" };
const std::vector<int>			ERROR_CODES { 307, 400, 403, 404, 405, 411, 413, 414, 500, 501, 503, 505 };
const std::vector<std::string>	VALID_DIRECTIVES { "listen", "host", "server_name", "index", "root", "methods", "cgi", "uploadDir",
													"client_max_body_size", "error_page", "autoindex", "return", "try_files",
													"keepalive_timeout" };
