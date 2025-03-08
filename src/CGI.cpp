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
	// std::string postData = "first_name=John&last_name=Doe";
	// int pipefd[2];
	// pipe(pipefd);  // Create pipe
	// write(pipefd[1], postData.c_str(), postData.size());  // Write POST data to pipe

	// // Close write end since we're only using the read end for stdin
	// close(pipefd[1]);

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

static void addLines(std::ifstream &file, int lines_to_read, std::string &temp, int fd)
{
	// std::string line;
	// for (int i = 0; i < lines_to_read; ++i) 
	// {
	// 	std::getline(file, line);
	// 	temp += line + "\n";
	// }

	(void)file;
	char buffer[1024]; // A buffer to read the file content
    std::string line;
    int lines_read = 0;
    ssize_t bytesRead;

    while (lines_read < lines_to_read) {
        bytesRead = read(fd, buffer, sizeof(buffer) - 1);  // Read data into buffer

        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available yet (non-blocking mode), let’s wait a bit and try again.
                continue;  // This will allow the loop to retry reading
            } else {
                std::cerr << "Error reading file: " << strerror(errno) << std::endl;
                break;
            }
        }

		Logger::log("\n\n\nBYTES READ " + std::to_string(bytesRead) + "\n\n\n");
		
        if (bytesRead == 0) {
            // End of file reached
            break;
        }

        // Null-terminate the buffer to treat it as a string
        buffer[bytesRead] = '\0';

        // Process the data and accumulate lines
        for (ssize_t i = 0; i < bytesRead; ++i) {
            char c = buffer[i];
            if (c == '\n') {
                temp += line + "\n";  // Add completed line to temp
                line.clear();  // Reset line
                ++lines_read;
                if (lines_read >= lines_to_read) {
                    break;
                }
            } else {
                line += c;  // Accumulate characters into a line
            }
        }
    }
}

static void createBody(std::string &body, std::string method)
{
	const char* filename = "www/people.html";
	int fd = open(filename, O_RDONLY | O_NONBLOCK);
	if (fd == -1)
		throw HttpException::internalServerError("Error opening the file!");
	

	std::ifstream file("www/people.html");
	if (!file) 
	{
		throw HttpException::internalServerError("Error opening the file!");
		// std::cerr << "Error opening the file!" << std::endl; // use exception
		// return ;
	}
	std::string temp;
	addLines(file, 42, temp, fd);
	if (method == "POST")
		temp += body;
	addLines(file, 23, temp, fd);
	if (method == "GET")
		temp += body;
	addLines(file, 4, temp, fd);
	file.close();
	close(fd);
	body = temp;
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

HttpResponse handleCGI(std::vector<multipart> data, queryMap map, std::string script, std::string method)
{
	pid_t				pid;
	int					status;
	FILE				*temp = std::tmpfile();
	int					fdtemp = fileno(temp);
	int					timeoutTime = 10;
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
			write(fdtemp, "Content-type: text/plain\r\n\r\nTIMEOUT", 35);
		}
		wait(NULL);
		exit(0);
	}
	else
	{
		waitpid(pid, &status, 0);
		setCgiString(temp, fdtemp, body);
		createBody(body, method);
	}
	// std::cout << body;
	return HttpResponse("CGI success", body);
}

