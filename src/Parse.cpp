
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Parse.hpp"

std::string	getBody(const std::string& path)
{
	std::string	filepath = path.substr(1, path.size());

	// temporary response
	if (filepath.empty())
	{
		std::cout << "getBody " << path << std::endl;
		return "<html><body>this is a temporary response</body></html>";
	}

	std::ifstream file(filepath);

	if (!file)
		return "";

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

std::string	getHeader(const size_t& len)
{
	std::string	header;

	if (len > 0)
		header = "HTTP/1.1 200 OK\r\n";
	else
		header = "HTTP/1.1 404 FAIL\r\n";

	// content type could be based on file extensions
	header += "Content-Type: text/html\r\n";

	// always get the size of body, if size is too large consider 'chunking' the data
	// important to get content-length right, as client will hang and wait indefinitely
	// if len is too large, we can leave this line out and use chunk encoding
	if (len < 1024)
		header += "Content-Length: " + std::to_string(len) + "\r\n";
	else
		header += "Transfer-Encoding: chunked\r\n";

	// allow persistent connection, usually handled by client
	// response += "Connection: keep-alive\r\n";  

	// empty line
	header += "\r\n";

	return header;
}

std::string	post()
{
	std::string	temp = "";

	return temp;
}