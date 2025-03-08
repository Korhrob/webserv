#pragma once

#include <string>
#include <exception>

class HttpException : public std::exception {
    private:
        int         m_statusCode;
        std::string m_msg;
		std::string	m_targetUrl;

    public:
        HttpException(int code, std::string msg, std::string targetUrl = "")
            : m_statusCode(code), m_msg(msg), m_targetUrl(targetUrl) {}

		~HttpException() = default;

		HttpException() = delete;
		HttpException(const HttpException&) = delete;
		HttpException&	operator=(const HttpException&) = delete;

        const char* what() const noexcept override {
            return m_msg.c_str();
        }

		const std::string	getTargetUrl() {
			return m_targetUrl;
		}

        int getStatusCode() const noexcept {
            return m_statusCode;
        }

		static HttpException	movedPermanently(const std::string& targetUrl) {
			return HttpException(302, "Moved Permanently", targetUrl);
		}

		static HttpException	temporaryRedirect(const std::string& targetUrl) {
			return HttpException(307, "Temporary Redirect", targetUrl);
		}

        static HttpException    badRequest(std::string log) {
            return HttpException(400, "Bad Request: " + log);
        }

        static HttpException    forbidden() {
            return HttpException(403, "Forbidden");
        }

        static HttpException    notFound() {
            return HttpException(404, "Not Found");
        }

		static HttpException	requestTimeout()
		{
			return HttpException(408, "Request Timeout");
		}

		static HttpException	lengthRequired() {
			return HttpException(411, "Length Required");
		}

        static HttpException    internalServerError(std::string log) {
            return HttpException(500, "Internal Server Error: " + log);
        }

        static HttpException    notImplemented() {
            return HttpException(501, "Not Implemented");
        }

        static HttpException    httpVersionNotSupported() {
            return HttpException(505, "HTTP Version Not Supported");
        }
};