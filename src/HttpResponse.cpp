#include "HttpResponse.hpp"
#include "HttpException.hpp"

HttpResponse::HttpResponse(int code, const std::string& msg) : m_code(code), m_msg(msg), m_body(""), m_sendType(TYPE_SINGLE) {}

HttpResponse::HttpResponse(int code, const std::string& msg, const std::string& path) : m_code(code), m_msg(msg)
{
    try {
		size_t size = std::filesystem::file_size(path);
        if (size <= PACKET_SIZE) {
            m_body = getBody(path);
            m_headers.emplace("Content-Length", std::to_string(m_body.size()));
            m_sendType = TYPE_SINGLE;
        } else {
            m_headers.emplace("Transfer-Encoding", "chunked");
            m_sendType = TYPE_CHUNKED;
        }
	} catch (const std::exception& e) {
		throw HttpException::notFound();
	}
}

std::string	HttpResponse::getBody(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);

	if (!file.good())
		return "";

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

std::string HttpResponse::getResponse()
{
    std::string response;

    response = getStatusLine();
    response += getHeaders();
    response += "\r\n";
    if (!m_body.empty())
        response += m_body;

    return response;
}

std::string HttpResponse::getStatusLine()
{
    return "HTTP/1.1 " + std::to_string(m_code) + " " + m_msg + "\r\n";
}

std::string HttpResponse::getHeaders()
{
    std::string headers;

    for (auto [key, value]: m_headers)
        headers += key + ": " + value + "\r\n";

    return headers;
}

e_type  HttpResponse::getSendType()
{
    return m_sendType;
}

int HttpResponse::getStatusCode()
{
    return m_code;
}
