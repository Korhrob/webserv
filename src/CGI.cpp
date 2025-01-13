#include "CGI.hpp"

CGI::CGI()	{
}

CGI::~CGI()	{
}

int CGI::runCGI(std::string cgi, Client client)
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