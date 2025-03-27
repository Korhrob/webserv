#include "CGI.hpp"
#include "Client.hpp"
#include "HttpException.hpp"

static void setEnvValue(std::string envp, std::string value, std::vector<char*>& envPtrs)
{
	std::string env = envp + "=" + value;
	char* newEnv = strdup(env.c_str());
	envPtrs.push_back(newEnv);
}

static void createEnv(std::vector<char*>& envPtrs, std::string script)
{
	setEnvValue("SERVER_NAME", "localhost", envPtrs);
	setEnvValue("GATEWAY_INTERFACE", "CGI/1.1", envPtrs);
	setEnvValue("SERVER_PORT", "8080", envPtrs);
	setEnvValue("SERVER_PROTOCOL", "HTTP/1.1", envPtrs);
	setEnvValue("REMOTE_ADDR", "127.0.0.1", envPtrs);
	setEnvValue("SCRIPT_NAME", script, envPtrs);
	setEnvValue("SCRIPT_FILENAME", "/home/avegis/projects/wwebserver/cgi-bin" + script, envPtrs);
	setEnvValue("QUERY_STRING", "", envPtrs);
	setEnvValue("CONTENT_TYPE", "application/x-www-form-urlencoded", envPtrs);
	setEnvValue("PHP_SELF", "../cgi-bin" + script, envPtrs);
	setEnvValue("DOCUMENT_ROOT", "/home/avegis/projects/wwebserver", envPtrs);
	setEnvValue("HTTP_USER_AGENT", "Mozilla/5.0", envPtrs);
	setEnvValue("REQUEST_URI", "/cgi-bin/people.cgi.php", envPtrs);
	setEnvValue("HTTP_REFERER", "http://localhost/", envPtrs);
}

static void run(std::string cgi, int fdtemp, std::vector<char*> envPtrs)
{
	std::string interpreter = "/usr/bin/php";
	std::filesystem::path currentPath = std::filesystem::current_path();
	cgi = currentPath.string() + "/cgi-bin" + cgi;
	char	*arg[3] = {(char *)interpreter.c_str(), (char *)cgi.c_str(), nullptr};
	if (dup2(fdtemp, STDOUT_FILENO) < 0)
	{
		perror("dup2");
		exit(EXIT_FAILURE);
	}
	envPtrs.push_back(nullptr);
	execve((char *)interpreter.c_str(), arg, envPtrs.data());
	perror("execve");
	exit(EXIT_FAILURE);
}

static void setCgiString(FILE *temp, int fdtemp, std::string& body)
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
	body = string;
}

static void addData(std::vector<multipart> data, std::vector<char*>& envPtrs)
{
	for (multipart part: data) {
		std::string str(part.content.begin(), part.content.end());
		setEnvValue(part.name, str, envPtrs);
		if (!part.nestedData.empty())
			addData(part.nestedData, envPtrs);
	}
}

static void addQuery(queryMap map, std::vector<char*>& envPtrs)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		setEnvValue(it->first, it->second[0], envPtrs);
	}
}

std::string handleCGI(std::vector<multipart> data, queryMap map, std::string script, std::string method)
{
	pid_t				pid;
	int					status;
	FILE				*temp = std::tmpfile();
	int					fdtemp = fileno(temp);
	int					timeoutTime = 8;
	std::vector<char*> 	envPtrs;
	std::string			body;
	createEnv(envPtrs, script);
	setEnvValue("REQUEST_METHOD", method, envPtrs);
	if (!data.empty())
		addData(data, envPtrs);
	else
		addQuery(map, envPtrs);
	pid = fork();
	if (pid < 0)
		throw HttpException::internalServerError("Fork fail");
	if (pid == 0)
	{
		pid_t cgiPid = fork();
		if (cgiPid == 0)
			run(script, fdtemp, envPtrs);
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
			write(fdtemp, "TIMEOUT", 7);
		}
		wait(NULL);
		exit(0);
	}
	else
	{
		waitpid(pid, &status, 0);
		setCgiString(temp, fdtemp, body);
	}
	if (body == "TIMEOUT")
		throw HttpException::internalServerError("Timeout");
	return body;
}

