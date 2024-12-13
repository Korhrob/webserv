#pragma once

#include <iostream>
#include <map>
#include <string>

class CGI
{
	private:
		struct FileInfo	{
			std::string	m_name;
			std::string	m_type;
			std::string	m_tmp_name;
			size_t		m_size;
			int			m_error;
		};
		std::map<std::string, FileInfo>	files;

	public:
		CGI();
		~CGI();
		void create_files_global(const std::map<std::string, std::string>& map_info);
		void runCGI(std::string script, const std::map<std::string, std::string>& map_info, std::string file_location);

};

