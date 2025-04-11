#include "HttpResponse.hpp"
#include "HttpException.hpp"
#include "Directory.hpp"

#include <filesystem>

HttpResponse::HttpResponse(int code, const std::string& msg, const std::string& path, const std::string& targetUrl, int close, t_ms timeout)
: m_code(code), m_msg(msg), m_body(""), m_type(TYPE_SINGLE), m_targetUrl(targetUrl), m_close(close), m_timeout(timeout)
{
	setBody(path);
	setHeaders();
}

HttpResponse::HttpResponse(const std::string& body, t_ms timeout) : m_code(200), m_msg("OK"), m_body(""), m_targetUrl(""), m_close(0), m_timeout(timeout)
{
	size_t size = body.size();

	if (size <= PACKET_SIZE)
	{
		m_body = body;
		m_type = TYPE_SINGLE;
	}
	else
		m_type = TYPE_CHUNKED;

	setHeaders();

	m_headers.emplace("Content-type", "text/html");
}

void	HttpResponse::setBody(const std::string& path)
{
	if (path.empty())
		return;

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
				m_body = readBody(path);
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

const std::string	HttpResponse::readBody(const std::string& path)
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
	m_headers = {};

	if (!m_close)
		m_close = (m_code == 408) || (m_code == 413) || (m_code == 500);

	if (m_code == 307)
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

const std::string HttpResponse::response()
{
    std::string response;

    response = statusLine();
    response += headers();
    response += m_body;

    return response;
}

const std::string HttpResponse::statusLine()
{
    return "HTTP/1.1 " + std::to_string(m_code) + " " + m_msg + "\r\n";
}

const std::string HttpResponse::headers()
{
    std::string headers;

    for (auto [key, value]: m_headers)
	{
        headers += key + ": " + value + "\r\n";
	}

	headers += "\r\n";

    return headers;
}

e_type  HttpResponse::sendType()
{
    return m_type;
}

const std::string HttpResponse::header()
{
	return statusLine() + headers();
}

const std::string& HttpResponse::body()
{
	return m_body;
}

int HttpResponse::closeConnection()
{
	return m_close;
}

void	HttpResponse::appendBody(const std::string& str)
{
	m_body.append(str);
}
