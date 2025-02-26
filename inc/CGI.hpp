#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <signal.h>
#include <memory>

class Response;
class Client;

int runCGI(std::string script, std::shared_ptr<Client> client, std::string method);