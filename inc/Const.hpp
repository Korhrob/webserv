#pragma once

#include <string>
#include <vector>

static const unsigned int BUFFER_SIZE = 1024;
static const unsigned int PACKET_SIZE = 8192; // 8 KB
static const unsigned int CLIENT_TIMEOUT = 5000; // 5 seconds

const std::string				WHITESPACE = " \t";
const std::vector<std::string>	EMPTY_VECTOR;
const std::vector<std::string>	MANDATORY_DIRECTIVES { "listen", "server_name" };
const std::vector<std::string>	VALID_DIRECTIVES { "listen", "server_name", "index", "root" };