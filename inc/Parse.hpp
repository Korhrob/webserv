#pragma once

#include <string>

std::string	getBody(const std::string& path);
std::string	getHeaderSingle(const size_t& len, int ecode, std::string msg);
std::string	getHeaderChunk();