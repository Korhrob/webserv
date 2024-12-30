#pragma once

#include <string>
#include <iostream> // std::cout
#include <sys/socket.h> // socket
#include <unistd.h> // close
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // For inet_ntoa and struct in_addr

#include <string.h> // strlen
#include <poll.h> // pollfd
#include <fcntl.h> // fnctl
#include <chrono> // time
#include <sstream>
#include <unordered_map>
#include <fstream> // ofstream
#include <variant>
#include <vector>
#include <map>

#include "ILog.hpp" // log,  logError
#include "Const.hpp"

typedef struct sockaddr_in t_sockaddr_in;
typedef std::chrono::steady_clock::time_point t_time;

// TODO: move inline function to their own .cpp file

struct s_part {
	std::string	filename;
	std::string	contentType;
	std::string	content;
};

using multipartMap = std::unordered_map<std::string, s_part>;

enum	e_multipart
{
	FILENAME,
	CONTENT_TYPE,
	CONTENT
};

struct jsonValue {
    std::variant<std::string, double, bool, std::nullptr_t, 
                 std::map<std::string, jsonValue>, 
                 std::vector<jsonValue>> value;
};

using jsonMap = std::unordered_map<std::string, jsonValue>;

class Client
{

	private:
		bool											m_alive;
		struct pollfd&									m_pollfd; // shortcut
		int												m_id;
		t_sockaddr_in									m_addr;
		// unsigned int									m_addr_len = sizeof(t_sockaddr_in);
		unsigned int									m_files_sent;
		t_time											m_last_activity;

		std::string										m_boundary;
		std::unordered_map<std::string, std::string>	m_formData;
		multipartMap									m_multipartData;
		jsonMap											m_jsonData;
		std::ofstream									m_file;

	public:

		Client(struct pollfd& pollfd, int id) : m_pollfd(pollfd), m_id(id)
		{
			m_pollfd.fd = -1;
			m_pollfd.events = POLLIN | POLLOUT;
			m_pollfd.revents = 0;
			m_alive = false;
		}
		~Client() {}

		bool	isAlive() { return m_alive; }
		bool	incoming() { return m_pollfd.revents & POLLIN; }
		bool	outgoing() { return m_pollfd.revents & POLLOUT; }
		bool	fileIsOpen() { return m_file.is_open(); }

		int		fd() { return m_pollfd.fd; }
		struct pollfd& getPollfd() { return m_pollfd; }

		bool	connect(int fd, t_sockaddr_in sock_addr, t_time time)
		{
			// set up non blocking fd
			int flags = fcntl(fd, F_GETFL, 0);
    		fcntl(fd, F_SETFL, flags | O_NONBLOCK);

			// check fcntl errors

			// reinitialize client information
			m_pollfd.fd = fd;
			m_pollfd.events = POLLIN | POLLOUT;
			m_pollfd.revents = 0;
			m_alive = true;

			m_addr = sock_addr;
			m_files_sent = 0;
			m_last_activity = time;

			log("Client connected!");

			return true;
		}


		void	disconnect()
		{
			close(m_pollfd.fd);
			m_pollfd.fd = -1;
			m_pollfd.revents = 0;
			m_alive = false;

			log("Client disconnected!");
		}

		void	update(t_time time)
		{
			m_last_activity = time;
		}

		int	respond(const std::string& response)
		{
			int bytes_sent = send(m_pollfd.fd, response.c_str(), response.size(), 0);
			log("-- BYTES SENT " + std::to_string(bytes_sent) + "--\n\n");
			m_files_sent++;
			m_pollfd.revents = POLLOUT; 

			return (bytes_sent > 0);
		}

		bool	timeout(t_time now)
		{
			if (m_pollfd.revents & POLLIN)
				return false;

			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_activity).count();

			return (diff > CLIENT_TIMEOUT);
		}

		void	resetEvents()
		{
			m_pollfd.revents = 0;
		}

		void	setBoundary(std::string boundary)
		{
			m_boundary = boundary;
		}

		std::string	getBoundary()
		{
			return m_boundary;
		}

		char	hexToChar(std::string hex)
		{
			return stoi(hex, nullptr, 16);
		}

		void	addFormData(std::string key, std::string value)
		{
			m_formData.insert_or_assign(key, value);
		}

		std::string	getFormData(std::string key)
		{
			return (m_formData[key]);
		}

		std::ofstream&	openFile(std::string name)
		{
			// auto now = std::chrono::steady_clock::now();
			// auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
			m_file.open(std::to_string(m_id) + "_" + name, std::ios::out | std::ios::app | std::ios::binary);
			
			return m_file;
		}
		
		std::ofstream&	getFileStream()
		{
			return m_file;
		}

		void	closeFile()
		{
			m_file.close();
		}

		void	addMultipartData(std::string key, int type, std::string value)
		{
			switch (type) {
				case FILENAME:
					m_multipartData[key].filename = value;
					break;
				case CONTENT_TYPE:
					m_multipartData[key].contentType = value;
					break;
				case CONTENT:
					m_multipartData[key].content.append(value);
			}
		}

		std::string	getFilename(std::string name)
		{
			return m_multipartData[name].filename;
		}

		std::string	getContentType(std::string name)
		{
			return m_multipartData[name].contentType;
		}

		std::string	getContent(std::string name)
		{
			return m_multipartData[name].content;
		}

		void	displayMultipartData() // debugging
		{
			for (auto [key, value]: m_multipartData) {
			log("key: " + key);
			log("filename: " + value.filename);
			log("content-type: " + value.contentType);
			log("content: " + value.content);
			}
		}

		void	displayFormData() // debugging
		{
			for (auto& [key, value] : m_formData)
				std::cout << key << "=" << value << "\n";
		}
		
};
