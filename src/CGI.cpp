#include "CGI.hpp"

CGI::CGI()	{
}

CGI::~CGI()	{
}

void CGI::create_files_global(const std::map<std::string, std::string>& map_info)
{
	FileInfo file = {"file.txt", "text/plain", "/path/asd.tmp", 1024, 0};

	files["file"] = file;
}

void CGI::runCGI(std::string script, const std::map<std::string, std::string>& map_info, std::string file_location)
{

}