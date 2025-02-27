#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <signal.h>
#include <memory>

#include "HttpResponse.hpp"

HttpResponse runCGI(std::string script, std::string method);