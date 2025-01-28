#pragma once

#include <string>
#include <exception>

class HttpException : public std::exception {
    private:
        int         m_statusCode;
        std::string m_msg;

    public:
        HttpException(int code, std::string msg)
            : m_statusCode(code), m_msg(msg) {}

        const char* what() const noexcept override {
            return m_msg.c_str();
        }

        int getStatusCode() const noexcept {
            return m_statusCode;
        }

        static HttpException    badRequest(std::string log) {
            return HttpException(400, "Bad Request: " + log);
        }

        static HttpException    notFound() {
            return HttpException(404, "Not Found");
        }

		static HttpException	lengthRequired() {
			return HttpException(411, "Length Required");
		}

        static HttpException    unsupportedMediaType() {
            return HttpException(415, "Unsupported Media Type");
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
