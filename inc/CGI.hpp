#pragma once

#include <iostream>
#include <sys/wait.h>
#include <filesystem>

#include "HttpResponse.hpp"

std::string handleCGI(std::vector<mpData> data, queryMap map, std::string script, std::string method);