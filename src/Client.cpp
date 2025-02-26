#include "Response.hpp"
#include "Client.hpp"

void	Client::setEnv(std::string envp)
{
	(void)envp;
}

void	Client::setEnvValue(std::string envp, std::string value)
{
	std::string env = envp + "=" + value;
	char* newEnv = strdup(env.c_str());
	m_envPtrs.push_back(newEnv);
	Logger::getInstance().log(env);
}

void	Client::unsetEnv()
{
	m_env.clear();
}

std::vector<char*>	Client::getEnv()
{
	return (m_envPtrs);
}

std::string	Client::getEnvValue(std::string envp)
{
	return (m_env[envp]);
}

void Client::createEnv()
{
	// std::string postData = "first_name=John&last_name=Doe";
	// int contentLength = postData.size();

	setEnvValue("SERVER_NAME", "localhost");
	setEnvValue("GATEWAY_INTERFACE", "CGI/1.1");
	setEnvValue("SERVER_PORT", "8080");
	setEnvValue("SERVER_PROTOCOL", "HTTP/1.1");
	setEnvValue("REMOTE_ADDR", "127.0.0.1");
	setEnvValue("SCRIPT_NAME", "people.cgi.php");
	setEnvValue("SCRIPT_FILENAME", "/home/avegis/projects/wwebserver/cgi-bin/people.cgi.php");
	setEnvValue("QUERY_STRING", "");
	setEnvValue("CONTENT_TYPE", "application/x-www-form-urlencoded");
	// setEnvValue("CONTENT_LENGTH", std::to_string(contentLength));
	setEnvValue("PHP_SELF", "../cgi-bin/people.cgi.php");
	setEnvValue("DOCUMENT_ROOT", "/home/avegis/projects/wwebserver");
	setEnvValue("HTTP_USER_AGENT", "Mozilla/5.0");
	setEnvValue("REQUEST_URI", "/cgi-bin/people.cgi.php");
	setEnvValue("HTTP_REFERER", "http://localhost/");
	setEnvValue("first_name", "asd");
	setEnvValue("last_name", "qweasd");
	setEnvValue("search_type", "first_name");
	setEnvValue("search_name", "asd");
}

void Client::setBody(std::string string)
{
	m_body = string;
}

std::string Client::getBody()
{
	return (m_body);
}