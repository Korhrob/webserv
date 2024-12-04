#include "Response.hpp"
#include "Parse.hpp"
#include "ILog.hpp"
#include "Const.hpp"
#include "Client.hpp"

#include <memory>
#include <string>
#include <algorithm> // min
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>

Response::Response(std::shared_ptr<Client> client)
{
	readRequest(client->fd());
	parseRequest();

	// WRITE HEADER AND BODY

	if (m_size < PACKET_SIZE)
	{
		m_send_type = SINGLE;
		m_body = getBody(m_path);
		m_size = m_body.size();
		m_header = getHeaderSingle(m_size);

		std::cout << "== SINGLE RESPONSE ==\n" << str() << "\n\n" << std::endl;
	}
	else
	{
		m_send_type = CHUNK;
		m_header = getHeaderChunk();

		std::cout << "== CHUNK RESPONSE ==" << std::endl;
	}

	m_success = true;

}

bool	Response::readRequest(int fd)
{
	// read a bit about max request size for HTTP/1.1
	char	buffer[BUFFER_SIZE];
	//memset(buffer, 0, BUFFER_SIZE);
	size_t	bytes_read = recv(fd, buffer, BUFFER_SIZE, 0);

	log("-- BYTES READ " + std::to_string(bytes_read) + "--\n\n");

	if (bytes_read <= 0)
	{
		logError("Empty or invalid request");
		m_success = false;
		return false;
	}

	buffer[bytes_read] = '\0';
	m_request = std::string(buffer, bytes_read);
	log(buffer);
	return true;
}

void	Response::parseRequest()
{
	if (m_request.empty())
		return;
		
	size_t	pos = m_request.find('\n');

	if (pos == std::string::npos) {
		logError("Invalid request header");
		return;
	}

	std::istringstream	first_line(m_request.substr(0, pos));
	std::string			version;
	std::string			method;

	if (!(first_line >> method >> m_path >> version) || version != "HTTP/1.1") {
		logError("");
		return;
	}

	std::unordered_map<std::string, e_method> methods = {{"GET", e_method::GET}, {"POST", e_method::POST}, {"DELETE", e_method::DELETE}};
	auto it = methods.find(method);
	if (it != methods.end()) {
		m_method = it->second;
	} else {
		logError("");
		return;
	}

	try {
		m_size = std::filesystem::file_size(&m_path[1]); // robert?
	} catch (const std::exception& e) {
		m_size = 0;
		std::cerr << e.what() << '\n';
	}

	std::istringstream	request(m_request.substr(pos + 1));
	std::string			line;

	while (getline(request, line)) {
		pos = line.find(':');
		if (pos == std::string::npos)
			break;
		m_headers.try_emplace(line.substr(0, pos), line.substr(pos + 1)); // trim whitespace
	}

	std::cout << "-- PARSED ---------------------------------------------\n"
	<< method << " " << m_path << " " << version << "\n";
	for (const auto& [key, value] : m_headers) {
        std::cout << key << ":" << value << "\n";
    }
	std::cout << "-------------------------------------------------------\n";
}

std::string	Response::str()
{
	return m_header + m_body;
}

std::string	Response::body()
{
	return m_body;
}

std::string	Response::header()
{
	return m_header;
}

std::string	Response::path()
{
	return m_path;
}
