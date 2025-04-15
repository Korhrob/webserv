#include "CGI.hpp"
#include "Client.hpp"
#include "HttpException.hpp"
#include "Server.hpp"

#include <fcntl.h>
#include <unistd.h>

void setEnvValue(const std::string &envp, const std::string &value, std::vector<char*>& envPtrs)
{
	const std::string env = envp + "=" + value;
	char* newEnv = strdup(env.c_str());
	if (newEnv == nullptr)
	{
		freeEnvPtrs(envPtrs);
		throw HttpException::internalServerError("Malloc fail");
	}
	envPtrs.push_back(newEnv);
}

void createEnv(std::vector<char*>& envPtrs, const std::string& script)
{
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::filesystem::path currentPath = std::filesystem::current_path();
	setEnvValue("SERVER_NAME", "localhost", envPtrs);
	setEnvValue("GATEWAY_INTERFACE", "CGI/1.1", envPtrs);
	setEnvValue("SERVER_PORT", "8080", envPtrs);
	setEnvValue("SERVER_PROTOCOL", "HTTP/1.1", envPtrs);
	setEnvValue("REMOTE_ADDR", "127.0.0.1", envPtrs);
	setEnvValue("SCRIPT_NAME", script, envPtrs);
	setEnvValue("SCRIPT_FILENAME", currentPath.string() + "/cgi-bin" + script, envPtrs);
	setEnvValue("SCRIPT_FILENAME", currentPath.string() + "/cgi-bin" + script, envPtrs);
	setEnvValue("QUERY_STRING", "", envPtrs);
	setEnvValue("CONTENT_TYPE", "application/x-www-form-urlencoded", envPtrs);
	setEnvValue("PHP_SELF", "../cgi-bin" + script, envPtrs);
	setEnvValue("DOCUMENT_ROOT", currentPath.string(), envPtrs);
	setEnvValue("DOCUMENT_ROOT", currentPath.string(), envPtrs);
	setEnvValue("HTTP_USER_AGENT", "Mozilla/5.0", envPtrs);
	setEnvValue("REQUEST_URI", "/cgi-bin" + script, envPtrs);
	setEnvValue("REQUEST_URI", "/cgi-bin" + script, envPtrs);
	setEnvValue("HTTP_REFERER", "http://localhost/", envPtrs);
}

void run(const std::string& cgi, int temp_fd, std::vector<char*>& envPtrs, Server& server)
{
	std::string n_cgi = std::filesystem::current_path().string();
	n_cgi.append("/cgi-bin");
	n_cgi.append(cgi);

	const char	*args[3] = { INTERPRETER.c_str(), n_cgi.c_str(), nullptr };

	if (dup2(temp_fd, STDOUT_FILENO) < 0)
	{
		setEnvValue("DUP", "FAIL", envPtrs);
	}
	close(temp_fd);
	setEnvValue("DUP", "OK", envPtrs);
	envPtrs.push_back(nullptr);

	execve(args[0], const_cast<char* const*>(args), envPtrs.data());
	freeEnvPtrs(envPtrs);
	n_cgi.clear();
	std::string().swap(n_cgi);
	server.~Server();
	std::exit(1);
}

void addData(std::vector<mpData>& data, std::vector<char*>& envPtrs)
{
	for (mpData part: data) {
		std::string str(part.content.begin(), part.content.end());
		setEnvValue(part.name, str, envPtrs);
		if (!part.nestedData.empty())
			addData(part.nestedData, envPtrs);
	}
	data.clear();
}

void addQuery(queryMap& map, std::vector<char*>& envPtrs)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if(it->first.empty() || it->second.empty())
		{
			freeEnvPtrs(envPtrs);
			throw HttpException::badRequest("Invalid Query");
		}
		setEnvValue(it->first, it->second[0], envPtrs);
	}
	map.clear();
}

void freeEnvPtrs(std::vector<char*>& envPtrs)
{
	for (auto& it : envPtrs)
	{
		if (it)
			free(it);
	}
	envPtrs.clear();
	std::vector<char*>().swap(envPtrs);
}
