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

        static HttpException    badRequest(std::string msg = "") {
            return HttpException(400, "Bad Request: " + msg);
        }

        static HttpException    forbidden(std::string msg = "") {
            return HttpException(403, "Forbidden: " + msg);
        }

        static HttpException    notFound(std::string msg = "") {
            return HttpException(404, "Not Found: " + msg);
        }

		static HttpException    methodNotAllowed(std::string msg = "") {
            return HttpException(405, "Method Not Allowed: " + msg);
        }

		static HttpException	lengthRequired(std::string msg = "") {
			return HttpException(411, "Length Required: " + msg);
		}

		static HttpException	payloadTooLarge(std::string msg = "") {
			return HttpException(413, "Payload Too Large: " + msg);
		}

		static HttpException	URITooLong(std::string msg = "") {
			return HttpException(414, "URI Too Long: " + msg);
		}

        static HttpException    internalServerError(std::string msg = "") {
            return HttpException(500, "Internal Server Error: " + msg);
        }

        static HttpException    notImplemented(std::string msg = "") {
            return HttpException(501, "Not Implemented: " + msg);
        }

		static HttpException    serviceUnavailable(std::string msg = "") {
            return HttpException(503, "Service Unavailable: " + msg);
        }

        static HttpException    httpVersionNotSupported(std::string msg = "") {
            return HttpException(505, "HTTP Version Not Supported: " + msg);
        }

		static HttpException	redirect(int code, const std::string& msg, const std::string& targetUrl) {
			return HttpException(code, msg, targetUrl);
		}

		static const std::string	statusMessage(int code)
		{
			switch (code) {
				case 307: return "Temporary Redirect";
				case 400: return "Bad Request";
				case 403: return "Forbidden";
				case 404: return "Not Found";
				case 405: return "Method Not Allowed";
				case 411: return "Length Required";
				case 413: return "Payload Too Large";
				case 414: return "URI Too Long";
				case 500: return "Internal Server Error";
				case 501: return "Not Implemented";
				case 503: return "Service Unavailable";
				case 505: return "Http Version Not Supported";
				default:
					return "";
			}
		}

		static HttpException withCode(int code) {
			switch (code) {
				case 400: return badRequest();
				case 403: return forbidden();
				case 405: return methodNotAllowed();
				case 411: return lengthRequired();
				case 413: return payloadTooLarge();
				case 414: return URITooLong();
				case 500: return internalServerError();
				case 501: return notImplemented();
				case 503: return serviceUnavailable();
				case 505: return httpVersionNotSupported();
				default:
					return notFound("requested resource could not be found");
        }
    }
};
