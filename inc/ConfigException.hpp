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
			return ConfigException("unknown directive " + key);
		}

		static ConfigException	emptyDirective(const std::string& key)
		{
			return ConfigException("empty directive " + key);
		}

		static ConfigException	tooManyDirectives(const std::string& key)
		{
			return ConfigException("too many directives " + key);
		}

		static ConfigException	tooFewDirectives(const std::string& key)
		{
			return ConfigException("too few directives " + key);
		}

		static ConfigException	outOfRange(const std::string& value)
		{
			return ConfigException("value out of range " + value);
		}

		static ConfigException	nonNumerical(const std::string& value)
		{
			return ConfigException("non numerical value " + value);
		}

		static ConfigException	expectPath(const std::string& value)
		{
			return ConfigException("expecting path " + value);
		}

		static ConfigException	invalidErrorPage()
		{
			return ConfigException("error_page missing code(s) or path");
		}

		static ConfigException	duplicateErrorPage()
		{
			return ConfigException("duplicate error_page code");
		}

};