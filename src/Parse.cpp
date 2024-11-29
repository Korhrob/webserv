
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Parse.hpp"

std::string	getBody(const std::string& path)
{
	// temporary response
	if (path.empty()) // get index.html
		return "<html><body>this is a temporary response</body></html>";

	std::ifstream file(path, std::ios::binary);

	if (!file)
		return "";

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

std::string	getHeaderSingle(const size_t& len)
{
	std::string	header;

	header = "HTTP/1.1 200 OK\r\n";
	header += "Content-Type: text/html\r\n";
	header += "Content-Length: " + std::to_string(len) + "\r\n";
	header += "\r\n";

	return header;
}

std::string	getHeaderChunk()
{
	std::string	header;

	header = "HTTP/1.1 200 OK\r\n";
	header += "Content-Type: text/html\r\n";
	header += "Transfer-Encoding: chunked\r\n";
	header += "\r\n";

	return header;
		
}
