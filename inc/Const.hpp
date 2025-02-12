#pragma once

#include <string>
#include <vector>

static const unsigned int BUFFER_SIZE = 1024;
static const unsigned int PACKET_SIZE = 8192; // 8 KB
static const unsigned int CLIENT_TIMEOUT = 5000; // 5 seconds

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
const std::vector<std::string>	VALID_DIRECTIVES { "listen", "host", "server_name", "index", "root", "methods", "cgi", "uploadDir", "client_max_body_size", "access_log", "error_log", "worker_connections", "client_max_body_size", "error_page", "allow_methods", "autoindex", "cgi_path", "cgi_ext", "return", "alias" };
const std::vector<std::string>	VALID_METHODS { "GET", "POST", "DELETE" };
const std::string				RESPONSE_TIMEOUT = "HTTP/1.1 408 Request Timeout\r\nConnection: close\r\n\r\n";
