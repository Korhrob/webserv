
#include <string>
#include "Parse.hpp"

std::string	getBody(const std::string& path)
{
	std::string	body = "";

	// temporary response, try to get the file contents here
	body = "<html><body>this is a response</body></html>";

	return body;
}

std::string	getHeader(const size_t& len)
{
	std::string	header = "";

	// if len > 0, success, else fail and get default 404 content

	// standard success first line
	header = "HTTP/1.1 200 OK\r\n";

	// content type could be based on file extensions
	header += "Content-Type: text/html\r\n";

	// always get the size of body, if size is too large consider 'chunking' the data
	// important to get content-length right, as client will hang and wait indefinitely
	header += "Content-Length: " + std::to_string(len) + "\r\n";

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