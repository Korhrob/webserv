#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sys/wait.h>
#include <signal.h>
#include <memory>
#include <filesystem>

#include "HttpResponse.hpp"

std::string handleCGI(std::vector<multipart> data, queryMap map, std::string script, std::string method);