#include "CGI.hpp"
#include "Client.hpp"
#include "HttpException.hpp"

#include <fcntl.h>
#include <unistd.h>

void setEnvValue(std::string envp, std::string value, std::vector<char*>& envPtrs)
{
	std::string env = envp + "=" + value;
	char* newEnv = strdup(env.c_str());
	envPtrs.push_back(newEnv);
}

void createEnv(std::vector<char*>& envPtrs, std::string script)
{
	std::filesystem::path currentPath = std::filesystem::current_path();
	setEnvValue("SERVER_NAME", "localhost", envPtrs);
	setEnvValue("GATEWAY_INTERFACE", "CGI/1.1", envPtrs);
	setEnvValue("SERVER_PORT", "8080", envPtrs);
	setEnvValue("SERVER_PROTOCOL", "HTTP/1.1", envPtrs);
	setEnvValue("REMOTE_ADDR", "127.0.0.1", envPtrs);
	setEnvValue("SCRIPT_NAME", script, envPtrs);
	setEnvValue("SCRIPT_FILENAME", currentPath.string() + "/cgi-bin" + script, envPtrs);
	setEnvValue("QUERY_STRING", "", envPtrs);
	setEnvValue("CONTENT_TYPE", "application/x-www-form-urlencoded", envPtrs);
	setEnvValue("PHP_SELF", "../cgi-bin" + script, envPtrs);
	setEnvValue("DOCUMENT_ROOT", currentPath.string(), envPtrs);
	setEnvValue("HTTP_USER_AGENT", "Mozilla/5.0", envPtrs);
	setEnvValue("REQUEST_URI", "/cgi-bin" + script, envPtrs);
	setEnvValue("HTTP_REFERER", "http://localhost/", envPtrs);
}

void run(std::string cgi, int temp_fd, std::vector<char*> envPtrs) // , pid_t myPid
{
	cgi = std::filesystem::current_path().string() + "/cgi-bin" + cgi;
	char	*args[3] = { const_cast<char*>(INTERPRETER.c_str()), const_cast<char*>(cgi.c_str()), nullptr };
	(void)cgi;
	(void)args;

	if (dup2(temp_fd, STDOUT_FILENO) < 0)
		setEnvValue("DUP", "FAIL", envPtrs);
	close(temp_fd);
	setEnvValue("DUP", "OK", envPtrs);
	envPtrs.push_back(nullptr);

	// int nullfd = open("/dev/null", O_WRONLY);
	// if (nullfd != -1)
	// {
	// 	dup2(nullfd, STDERR_FILENO);
	// 	close(nullfd);
	// }

	// int devnull = open("/dev/null", O_RDONLY);
	// if (devnull != -1)
	// {
	// 	dup2(devnull, STDIN_FILENO);
	// 	close(devnull);
	// }
	
	//write(STDOUT_FILENO, "Hello from CGI", 15);

	execlp("echo", "echo", "Hello World", nullptr);
	//execve(args[0], args, envPtrs.data());
	Logger::logError("execve failed");
	//kill(myPid, SIGTERM);
	std::exit(1);
}

void setCgiString(FILE *temp, int fdtemp, std::string& body)
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

void addData(std::vector<mpData> data, std::vector<char*>& envPtrs)
{
	for (mpData part: data) {
		std::string str(part.content.begin(), part.content.end());
		setEnvValue(part.name, str, envPtrs);
		if (!part.nestedData.empty())
			addData(part.nestedData, envPtrs);
	}
}

void addQuery(queryMap map, std::vector<char*>& envPtrs)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if(it->first.empty() || it->second.empty())
			throw HttpException::badRequest("Invalid Query");
		setEnvValue(it->first, it->second[0], envPtrs);
	}
}

void freeEnvPtrs(std::vector<char*>& envPtrs)
{
	for (size_t i = 0; i < envPtrs.size(); ++i) {
        free(envPtrs[i]);
    }
}
