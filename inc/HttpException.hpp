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

		const std::string	target() {
			return m_targetUrl;
		}

        int code() const noexcept {
            return m_statusCode;
        }

		static HttpException	temporaryRedirect(const std::string& targetUrl) {
			return HttpException(307, "Temporary Redirect", targetUrl);
		}

        static HttpException    badRequest(std::string log = "") {
            return HttpException(400, "Bad Request " + log);
        }

        static HttpException    forbidden(std::string log = "") {
            return HttpException(403, "Forbidden " + log);
        }

        static HttpException    notFound(std::string log = "") {
            return HttpException(404, "Not Found " + log);
        }

		static HttpException	requestTimeout(std::string log = "")
		{
			return HttpException(408, "Request Timeout " + log);
		}

		static HttpException	lengthRequired(std::string log = "") {
			return HttpException(411, "Length Required " + log);
		}

		static HttpException	payloadTooLarge(std::string log = "") {
			return HttpException(413, "Payload Too Large " + log);
		}

        static HttpException    internalServerError(std::string log = "") {
            return HttpException(500, "Internal Server Error " + log);
        }

        static HttpException    notImplemented(std::string log = "") {
            return HttpException(501, "Not Implemented " + log);
        }

        static HttpException    httpVersionNotSupported(std::string log = "") {
            return HttpException(505, "HTTP Version Not Supported " + log);
        }
};
