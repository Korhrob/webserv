#include "CGI.hpp"
#include "Response.hpp"
#include "Client.hpp"

static void run(std::string cgi, int fdtemp, std::vector<char*> envPtrs)
{
	std::string interpreter = "/usr/bin/php";
	std::string cgitest = "/home/avegis/projects/wwebserver/cgi-bin/people.cgi.php";
	// char	*arg[3] = {(char *)interpreter.c_str(), (char *)cgi.c_str(), nullptr};
	cgi = "yes";
	char	*arg[3] = {(char *)interpreter.c_str(), (char *)cgitest.c_str(), nullptr};
	if (dup2(fdtemp, STDOUT_FILENO) < 0)
	{
		perror("dup2");
		exit(EXIT_FAILURE);
	}
	std::string postData = "first_name=John&last_name=Doe";
	int pipefd[2];
	pipe(pipefd);  // Create pipe
	write(pipefd[1], postData.c_str(), postData.size());  // Write POST data to pipe

	// Close write end since we're only using the read end for stdin
	close(pipefd[1]);

	envPtrs.push_back(nullptr);
	execve((char *)interpreter.c_str(), arg, envPtrs.data());
	perror("execve");
	exit(EXIT_FAILURE);
}

static void setCgiString(FILE *temp, int fdtemp, std::shared_ptr<Client> client)
{
	char buffer[4096];
	std::string string;

	fflush(temp);
	rewind(temp);
	while (!feof(temp))
	{
		if (fgets(buffer, 4096, temp) == NULL)
			break ;
		string += buffer;
	}
	close(fdtemp);
	fclose(temp);
	client->setBody(string);
}

int runCGI(std::string script, std::shared_ptr<Client> client, std::string method)
{
	pid_t		pid;
	int			status;
	FILE		*temp = std::tmpfile();
	int			fdtemp = fileno(temp);
	int			timeoutTime = 10;
	client->createEnv();
	client->setEnvValue("REQUEST_METHOD", method);

	// env = client->mallocEnv();
	// if (env == NULL)
		// return 1;
	pid = fork();
	if (pid < 0)
		return 1;
	if (pid == 0)
	{
		pid_t cgiPid = fork();
		if (cgiPid == 0)
			run(script, fdtemp, client->getEnv());
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
		// client->freeEnv(env);
	}
	return 0;
}

