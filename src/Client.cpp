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

void	Client::populateEnv()
{
	
}

void	Client::freeEnv(char** env)
{
	int i = 0;

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

void Client::setHtmlBody(std::string string)
{
	m_html_body = string;
}