#include "HttpResponse.hpp"
#include "HttpException.hpp"
#include "Directory.hpp"

#include <filesystem>

HttpResponse::HttpResponse(int code, const std::string& msg, const std::string& path, const std::string& targetUrl, int close, t_ms timeout)
: m_code(code), m_msg(msg), m_body(""), m_type(TYPE_SINGLE), m_targetUrl(targetUrl), m_close(close), m_timeout(timeout)
{
	Logger::log(path + "     " + std::to_string(code) + "        " + targetUrl);
	setBody(path);
	setHeaders();
}

HttpResponse::HttpResponse(const std::string& msg, const std::string& body) : m_msg(msg), m_body(""), m_close(0)
{
	if (body.empty())
	{
		m_code = 404;
		m_msg = "Not Found: CGI fail";
		m_type = TYPE_SINGLE;
	}
	else
	{
		m_code = 200;
		size_t size = body.size();
		if (size <= PACKET_SIZE) {
			m_body = body;
			m_type = TYPE_SINGLE;
		} else
			m_type = TYPE_CHUNKED;
	}
	setHeaders();
}

void	HttpResponse::setBody(const std::string& path)
{
	if (!path.empty())
	{
		try {
			if (std::filesystem::is_directory(path))
			{
				m_body = listDirectory(path, m_targetUrl);
				if (m_body.length() > PACKET_SIZE)
					m_type = TYPE_CHUNKED;
				return;
			}

			size_t size = std::filesystem::file_size(path);
			if (size <= PACKET_SIZE)
			{
				try {
					m_body = getBody(path);
				} catch (HttpException& e) {
					m_code = e.code();
					m_msg = e.what();
				}
			}
			else
				m_type = TYPE_CHUNKED;

		} catch (const std::filesystem::filesystem_error& e) {
			if (e.code() == std::errc::no_such_file_or_directory)
			{
				m_code = 404;
				m_msg = "Not Found";
			}
			else if (e.code() == std::errc::permission_denied)
			{
				m_code = 403;
				m_msg = "Forbidden";
			}
			else
			{
				m_code = 500;
				m_msg = "Internal Server Error: filesystem error";
			}
		}
	}
}

std::string	HttpResponse::getBody(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);

	if (!file)
	{
		if (!std::filesystem::exists(path))
			throw HttpException::notFound();

		std::filesystem::perms perms = std::filesystem::status(path).permissions();
		if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none)
			throw HttpException::forbidden("permission denied");

		throw HttpException::internalServerError("filesystem error");
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

void	HttpResponse::setHeaders()
{
	if (!m_close)
		m_close = (m_code == 408) || (m_code == 413) || (m_code == 500);

	if (!m_targetUrl.empty())
		m_headers.emplace("Location", m_targetUrl);

	if (m_close)
		m_headers.emplace("Connection", "close");
	else
	{
		m_headers.emplace("Connection", "keep-alive");
		m_headers.emplace("Keep-Alive", "timeout=" + std::to_string(m_timeout.count() / 1000));
	}


	if (m_type == TYPE_SINGLE)
		m_headers.emplace("Content-Length", std::to_string(m_body.size()));
	else
		m_headers.emplace("Transfer-Encoding", "chunked");
}

std::string HttpResponse::getResponse()
{
    std::string response;

    response = getStatusLine();
    response += getHeaders();
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
	{
        headers += key + ": " + value + "\r\n";
	}

	headers += "\r\n";

    return headers;
}

e_type  HttpResponse::getSendType()
{
    return m_type;
}

std::string HttpResponse::getHeader()
{
	return getStatusLine() + getHeaders();
}

int HttpResponse::getCloseConnection()
{
	return m_close;
}
