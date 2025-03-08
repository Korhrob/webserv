#include <iostream>
#include <filesystem>
#include "Directory.hpp"

std::string	listDirectory(const std::string& root_path, const std::string& target_url)
{
	std::string	path = root_path;
	std::string	out = "<h1>directory listing " + target_url + "</h1>\n";

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
		std::string filename = entry.path().filename().string();
		std::string	filepath = target_url + filename;

		if (std::filesystem::is_directory(entry.path())) {
            filename += "/";
        }

		out += "<a href=\"" + filepath + "\">";
		out += filename;
		out += "</a><br/>\n";
    }

	return out;
}