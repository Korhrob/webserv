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
#include <map>
#include <unordered_map>
#include <fstream> // ofstream
#include <variant>
#include <vector>
#include <map>

#include "Logger.hpp" // log,  logError
#include "Const.hpp"

typedef struct sockaddr_in 						t_sockaddr_in;
typedef std::chrono::steady_clock::time_point 	t_time;
typedef std::map<std::string, std::string>		mapStr;

// TODO: move inline function to their own .cpp file

using formMap = std::unordered_map<std::string, std::vector<std::string>>;

class Client
{

	private:
		mapStr											m_env;
		bool											m_alive;
		struct pollfd&									m_pollfd; // shortcut
		// int												m_id;
		t_sockaddr_in									m_addr;
		// unsigned int									m_addr_len = sizeof(t_sockaddr_in);
		unsigned int									m_files_sent;
		t_time											m_last_activity;
<<<<<<< HEAD
		std::string										m_html_body;

		std::string										m_boundary;
		multipart										m_multipartData;
		std::unordered_map<std::string, std::string>	m_formData;
		std::ofstream									m_file;
=======
		// formMap											m_formData;
		// std::ofstream									m_file;
		bool											m_close_connection = false;
>>>>>>> origin/master

	public:

		Client(struct pollfd& pollfd) : m_pollfd(pollfd)
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
		// bool	fileIsOpen() { return m_file.is_open(); }

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

			Logger::getInstance().log("Client connected!");

			return true;
		}


		void	disconnect()
		{
			close(m_pollfd.fd);
			m_pollfd.fd = -1;
			m_pollfd.revents = 0;
			m_alive = false;

			Logger::getInstance().log("Client disconnected!");
		}

		void	update(t_time time)
		{
			m_last_activity = time;
		}

		int	respond(const std::string& response)
		{
			#ifdef MSG_NOSIGNAL
				int bytes_sent = send(m_pollfd.fd, response.c_str(), response.size(), MSG_NOSIGNAL);
			#else
				int bytes_sent = send(m_pollfd.fd, response.c_str(), response.size(), 0);
			#endif
			Logger::getInstance().log("-- BYTES SENT " + std::to_string(bytes_sent) + "--\n\n");
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

<<<<<<< HEAD
		// Env functions
		void setEnv(std::string envp);
		void setEnvValue(std::string envp, std::string value);
		void unsetEnv();
		mapStr getEnv();
		std::string getEnvValue(std::string envp);
		void populateEnv();
		void freeEnv(char** env);
		char** mallocEnv();
		
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

		void	setFormData(std::string key, std::string value)
		{
			while (value.find('%') != std::string::npos) {
				size_t pos = value.find('%');
				value.insert(pos, 1, stoi(value.substr(pos + 1, 2), nullptr, 16));
				value.erase(pos + 1, 3);
			}
			m_formData.insert_or_assign(key, value);
		}

		std::string	getFormData(std::string key)
		{
			return (m_formData[key]);
		}

		void	displayFormData() // for debugging
		{
			for (auto& [key, value] : m_formData)
				std::cout << key << "=" << value << "\n";
		}
		
		std::ofstream&	openFile(std::string name)
		{
			// auto now = std::chrono::steady_clock::now();
			// auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
			m_file.open(std::to_string(m_id) + "_" + name, std::ios::out | std::ios::app | std::ios::binary);
=======
		// std::ofstream&	openFile(std::string name)
		// {
		// 	// auto now = std::chrono::steady_clock::now();
		// 	// auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		// 	m_file.open(std::to_string(m_id) + "_" + name, std::ios::out | std::ios::binary);
>>>>>>> origin/master
			
		// 	return m_file;
		// }
		
		// std::ofstream&	getFileStream()
		// {
		// 	return m_file;
		// }

		// void	closeFile()
		// {
		// 	m_file.close();
		// }

		void	setCloseConnection()
		{
			m_close_connection = true;
		}
		
		// void	addFormData(std::string key, std::string value)
		// {
		// 	m_formData[key].push_back(value);
		// }

		// std::vector<std::string>	getFormData(std::string key)
		// {
		// 	return (m_formData[key]);
		// }

		// void	displayFormData() // debug
		// {
		// 	for (auto& [key, values] : m_formData) {
		// 		std::cout << key << "=";
		// 		for (std::string value: values)
		// 			std::cout << value << "\n";
		// 	}
		// }
};
