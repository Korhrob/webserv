#pragma once

#include <string>
#include <exception>

class HttpException : public std::exception {
    private:
        int         m_statusCode;
        std::string m_errorMessage;

    public:
        HttpException(int code, std::string message)
            : m_statusCode(code), m_errorMessage(message) {}

        const char* what() const noexcept override {
            return m_errorMessage.c_str();
        }

        int getStatusCode() const noexcept {
            return m_statusCode;
        }

        static HttpException    badRequest() {
            return HttpException(400, "Bad Request");
        }

        static HttpException    notFound() {
            return HttpException(404, "Not Found");
        }

        static HttpException    unsupportedMediaType() {
            return HttpException(415, "Unsupported Media Type");
        }

        static HttpException    notImplemented() {
            return HttpException(501, "Not Implemented");
        }

        static HttpException    httpVersionNotSupported() {
            return HttpException(505, "HTTP Version Not Supported");
        }
};
