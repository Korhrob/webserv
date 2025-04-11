#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <signal.h>
#include <memory>
#include <filesystem>

#include "HttpResponse.hpp"

void setEnvValue(std::string envp, std::string value, std::vector<char*>& envPtrs);
void createEnv(std::vector<char*>& envPtrs, std::string script);
void run(std::string cgi, int temp_fd, std::vector<char*> envPtrs); // , pid_t myPid
void setCgiString(FILE *temp, int fdtemp, std::string& body);
void addData(std::vector<mpData> data, std::vector<char*>& envPtrs);
void addQuery(queryMap map, std::vector<char*>& envPtrs);
void freeEnvPtrs(std::vector<char*>& envPtrs);
//std::string handleCGI(std::vector<mpData> data, queryMap map, std::string script, std::string method);