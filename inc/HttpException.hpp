#pragma once

#include <string>
#include <exception>

class HttpException : public std::exception {
    private:
        int         m_code;
        std::string m_msg;
		std::string	m_redir;

    public:
        HttpException(int code, std::string msg, std::string targetUrl = "")
            : m_code(code), m_msg(msg), m_redir(targetUrl) {}

		~HttpException() = default;

		HttpException() = delete;
		HttpException(const HttpException&) = delete;
		HttpException&	operator=(const HttpException&) = delete;

        const char* what() const noexcept override {
            return m_msg.c_str();
        }

		const std::string	redir() {
			return m_redir;
		}

        int code() const noexcept {
            return m_code;
        }

        static HttpException    remoteClosedConnetion() {
            return HttpException(0, "Remote Closed Connection");
        }

		static HttpException	temporaryRedirect(const std::string& targetUrl) {
			return HttpException(307, "Temporary Redirect", targetUrl);
		}

        static HttpException    badRequest(std::string msg = "") {
            return HttpException(400, "Bad Request " + msg);
        }

        static HttpException    forbidden(std::string msg = "") {
            return HttpException(403, "Forbidden " + msg);
        }

        static HttpException    notFound(std::string msg = "") {
            return HttpException(404, "Not Found " + msg);
        }

		static HttpException	requestTimeout(std::string msg = "")
		{
			return HttpException(408, "Request Timeout " + msg);
		}

		static HttpException	lengthRequired(std::string msg = "") {
			return HttpException(411, "Length Required " + msg);
		}

		static HttpException	payloadTooLarge(std::string msg = "") {
			return HttpException(413, "Payload Too Large " + msg);
		}

        static HttpException    internalServerError(std::string msg = "") {
            return HttpException(500, "Internal Server Error " + msg);
        }

        static HttpException    notImplemented(std::string msg = "") {
            return HttpException(501, "Not Implemented " + msg);
        }

        static HttpException    httpVersionNotSupported(std::string msg = "") {
            return HttpException(505, "HTTP Version Not Supported " + msg);
        }
};