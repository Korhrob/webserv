#pragma once

#include <string>

#define MAX_RESPONSE_SIZE 8192 // 8 KB

std::string	getBody(const std::string& path);
std::string	getHeader(const size_t& len);
std::string	post();
