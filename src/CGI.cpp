#include "CGI.hpp"

static void run(std::string cgi, Client &client, int fdtemp, char **env)
{
	char	*arg[2] = {(char *)cgi.c_str(), NULL};
	int		fd;
	int method = 1; // set enum for post, get and delete, 1 for post

	if (method == 1)
	{
		
	}
	if (dup2(fdtemp, STDOUT_FILENO) < 0)
	{
		perror("dup2");
		exit(EXIT_FAILURE);
	}
	execve((char *)cgi.c_str(), arg, env);
	perror("execve");
	exit(0);
}

static void setCgiString(FILE *temp, int fdtemp, Client &client)
{
	char buffer[4096];
	std::string string;

	rewind(temp);
	while (!feof(temp))
	{
		if (fgets(buffer, 4096, temp) == NULL)
			break ;
		string += buffer;
	}
	close(fdtemp);
	fclose(temp);
	client.setHtmlBody(string);
}

int runCGI(std::string cgi, Client client)
{
	char**		env;
	pid_t		pid;
	int			status;
	FILE		*temp = std::tmpfile();
	int			fdtemp = fileno(temp);
	int			timeoutTime = 1;

	env = client.mallocEnv();
	if (env == NULL)
		return 1;
	pid = fork();
	if (pid < 0)
		return 1;
	if (pid == 1)
	{
		pid_t cgiPid = fork();
		if (cgiPid == 0)
			run(cgi, client, fdtemp, env);
		pid_t timeoutPid = fork();
		if (timeoutPid == 0)
		{
			sleep(timeoutTime);
			exit(0);
		}
		pid_t exitedPid = wait(NULL);
		if (exitedPid == cgiPid)
			kill(timeoutPid, SIGKILL);
		else
		{
			kill(cgiPid, SIGKILL);
			write(fdtemp, "Content-type: text/plain\r\n\r\nTIMEOUT", 35);
		}
		wait(NULL);
		exit(0);
	}
	else
	{
		waitpid(pid, &status, 0);
		setCgiString(temp, fdtemp, client);
		client.freeEnv(env);
	}
}