#include "Response.hpp"
#include "Client.hpp"

void	Client::setEnv(std::string envp)
{
	m_env[envp];
}

void	Client::setEnvValue(std::string envp, std::string value)
{
	m_env[envp] = value;
}

void	Client::unsetEnv()
{
	m_env.clear();
}

mapStr	Client::getEnv()
{
	return (m_env);
}

std::string	Client::getEnvValue(std::string envp)
{
	return (m_env[envp]);
}

void	Client::populateEnv(std::vector<multipart> info)
{
	m_env.clear();
	// for (auto it = info.begin(); it != info.end(); ++it) 
	// {
	// 	const multipart& m = *it;
	// 	m_env[m.name] = m.content[0];
	// }
	for (auto part: info) 
	{
		m_env[part.name] = part.content.empty() ? "" : std::string(part.content.begin(), part.content.end());
	}
	m_env["SERVER_NAME"] = "localhost";
	m_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	m_env["SERVER_PORT"] = "8080";
	m_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	m_env["REMOTE_ADDR"] = "127.0.0.1";
	m_env["SCRIPT_NAME"] = "/cgi-bin/people.cgi.php";
	m_env["SCRIPT_FILENAME"] = "/cgi-bin/people.cgi.php";
}

void	Client::freeEnv(char** env)
{
	unsigned long i = 0;

	while (i < m_env.size())
	{
		free(env[i]);
		env = NULL;
		i++;
	}
	free(env);
	env = NULL;
}

char**	Client::mallocEnv()
{
	int i = 0;
	mapStr temp = m_env;
	std::string value;
	char** env;

	env = (char **)malloc(sizeof(char *) * (m_env.size() + 1));
	if (!env)
		return NULL;
	env[m_env.size()] = NULL;
	for (mapStr::iterator it = temp.begin(); it != temp.end(); it++)
	{
		value = it->first;
		value += "=";
		value += it->second;
		env[i] = (char *)malloc(sizeof(char) * (strlen(value.c_str()) + 1));
		if (!env[i])
			return NULL;
		strcpy(env[i], value.c_str());
		env[i][strlen(env[i])] = 0;
		i++;
	}
	return env;
}

void Client::setBody(std::string string)
{
	m_body = string;
}

std::string Client::getBody()
{
	return (m_body);
}