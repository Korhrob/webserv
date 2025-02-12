#pragma once

#include <string>
#include <exception>

class ConfigException : public std::exception
{
	private:
		std::string	m_message;

		ConfigException(const std::string& message) : m_message(message) {};

	public:

		const char* what() const noexcept override {
			return m_message.c_str();
		};

		static ConfigException	duplicateKey(const std::string& key)
		{
			return ConfigException("duplicate key " + key);
		}

		static ConfigException	invalidPort(const std::string& port)
		{
			return ConfigException("invalid port " + port);
		}

		static ConfigException	portRange(const std::string& port)
		{
			return ConfigException(port + " out of range, use a value between 0-65535");
		}

		static ConfigException	invalidMethod(const std::string& method)
		{
			return ConfigException("invalid method " + method);
		}

		static ConfigException	unknownDirective(const std::string& key)
		{
			return ConfigException("uknown directive " + key);
		}

		static ConfigException	emptyDirective(const std::string& key)
		{
			return ConfigException("empty directive for key " + key);
		}

};