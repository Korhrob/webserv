#pragma once

#include <string>
#include <vector>

static const unsigned int BUFFER_SIZE = 1024;
static const unsigned int PACKET_SIZE = 8192; // 8 KB
static const unsigned int CLIENT_TIMEOUT = 5000;

const std::string	WHITESPACE = " \t";
const std::vector<std::string>	EMPTY_VECTOR;