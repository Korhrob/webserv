#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <vector>
#include <cstring>

// int main()
// {
// 	char buffer[PATH_MAX];
// 	getcwd(buffer, sizeof(buffer));
// 	std::string path = std::string(buffer) + "/upload.php";
// 	const char *cgi_path = path.c_str();

// 	std::cout << path << std::endl;

// 	std::vector<char*> args;
// 	args.push_back(const_cast<char*>(cgi_path));
// 	args.push_back(NULL);

// 	std::vector<char*> envp;
// 	envp.push_back(const_cast<char*>("PATH=/usr/bin:/bin"));
// 	envp.push_back(const_cast<char*>("CONTENT_TYPE=text/html"));
// 	envp.push_back(const_cast<char*>("QUERY_STRING=name=test"));
// 	envp.push_back(const_cast<char*>("REQUEST_METHOD=GET"));
// 	envp.push_back(NULL);

// 	char **argv = &args[0];
// 	char **env = &envp[0];

// 	if (execve(cgi_path, argv, env) == -1)
// 	{
// 		std::cerr << "Error executing execve: " << strerror(errno) << std::endl;
// 		return 1;
// 	}
// 	return 0;
// }

int main() {
	// Path to the PHP interpreter
	const char *interpreter = "/usr/bin/php";  // Adjust this path if needed
	
	char buffer[PATH_MAX];
	getcwd(buffer, sizeof(buffer));
	std::string path = std::string(buffer) + "/upload.php";
	const char *script = path.c_str();

	std::string file_path = "../uploads/file.txt";
	const char *file_arg = file_path.c_str();

	// Prepare the arguments for execve (PHP interpreter + PHP script)
	const char *args[] = { interpreter, script, file_arg, NULL };

	// Environment variables (optional, can be NULL or specify something)
	const char *env[] = { "REQUEST_METHOD=POST",  NULL };

	// Execute the PHP script using execve
	if (execve(interpreter, const_cast<char**>(args), const_cast<char**>(env)) == -1) {
		std::cerr << "Error executing execve: " << strerror(errno) << std::endl;
		return 1;
	}

	return 0;
}