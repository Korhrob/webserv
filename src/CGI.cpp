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
	setEnvValue("REQUEST_URI", "/cgi-bin/people.php", envPtrs);
	setEnvValue("HTTP_REFERER", "http://localhost/", envPtrs);
}

void run(std::string cgi, int fdtemp, std::vector<char*> envPtrs)
{
	cgi = std::filesystem::current_path().string() + "/cgi-bin" + cgi;
	char	*args[3] = { const_cast<char*>(INTERPRETER.c_str()), const_cast<char*>(cgi.c_str()), nullptr };

	if (dup2(fdtemp, STDOUT_FILENO) < 0)
		kill(0, SIGKILL);

	close(fdtemp);

	envPtrs.push_back(nullptr);
	execve(args[0], args, envPtrs.data());
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

// void handleCGI(std::vector<mpData> data, queryMap map, std::string script, std::string method)
// {
// 	// fileno isn't allowed
// 	// FILE				*temp = std::tmpfile();
// 	// int					fd = fileno(temp);
// 	//int					timeoutTime = 8;
// 	// std::string			body;
	
// 	int fd = open("tmp_cgi_file", O_CREAT | O_RDWR | O_TRUNC, 0600);
// 	std::vector<char*> 	envPtrs;
// 	createEnv(envPtrs, script);
// 	setEnvValue("REQUEST_METHOD", method, envPtrs);
	
// 	if (!data.empty())
// 		addData(data, envPtrs);
// 	else
// 		addQuery(map, envPtrs);
	
// 	pid_t				pid;
// 	int					status;

// 	pid = fork();
// 	if (pid < 0)
// 	{
// 		freeEnvPtrs(envPtrs);
// 		throw HttpException::internalServerError("Fork fail");
// 	}

// 	// if (pid == 0)
// 	// {
// 	// 	pid_t cgiPid = fork();
// 	// 	if (cgiPid == 0)
// 	// 		run(script, fdtemp, envPtrs);
// 	// 	pid_t timeoutPid = fork();
// 	// 	if (timeoutPid == 0)
// 	// 		sleep(timeoutTime);
// 	// 	pid_t exitedPid = wait(NULL);
// 	// 	if (exitedPid == cgiPid)
// 	// 	{
// 	// 		kill(timeoutPid, SIGKILL);
// 	// 	}
// 	// 	else
// 	// 	{
// 	// 		kill(cgiPid, SIGKILL);
// 	// 		write(fdtemp, "TIMEOUT", 7);
// 	// 	}
// 	// 	wait(NULL);
// 	// 	std::exit(0);
// 	// }

// 	if (pid == 0)
// 	{
// 		run(script, fd, envPtrs);
// 		kill(0, SIGKILL);
// 	}

// 	// cant wait pid here, this blocks execution
// 	// waitpid(pid, &status, 0);
// 	// setCgiString(temp, fdtemp, body);
// 	// close(fdtemp);

// 	// move timeout logic to server, attach pid to client instance
// 	// if (body == "TIMEOUT")
// 	// {
// 	// 	freeEnvPtrs(envPtrs);
// 	// 	throw HttpException::internalServerError("Timeout");
// 	// }

// 	// if (body.empty())
// 	// {
// 	// 	freeEnvPtrs(envPtrs);
// 	// 	throw HttpException::internalServerError("Something went wrong with CGI");
// 	// }

// 	freeEnvPtrs(envPtrs);

// 	epoll_event	event;
// 	event.events = EPOLLIN | EPOLLET;
// 	event.data.fd = fd;

// 	int m_epoll_fd = 0; // should use the epoll_fd from server
// 	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
// 	{
// 		Logger::logError("error connecting cgi fd, epoll_ctl");
// 		close(fd);
// 		return;
// 	}

// 	// we dont actually need to return anything, because epoll loop will resolve the CGI response
// 	// just store the cgi fd -> client instance fd to server instance map
// 	// m_cgi_map[fd] = client->fd();

// 	//return body;
// }
