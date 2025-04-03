#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpException.hpp"

void	Client::handleRequest(Config& config, std::vector<char>& vec)
{
	if (m_request.state() == COMPLETE)
		m_request = {};

	appendRequest(vec);

	try {
		m_request.parseRequest(config);

		if (m_request.state() == COMPLETE)
		{
			setTimeoutDuration(m_request.timeoutDuration());
			m_response = m_request.processRequest(getTimeoutDuration());
		}

	} catch (HttpException& e) {
		m_request.setState(COMPLETE);

		if (!m_request.server())
			m_request.setServer(config.findServerNode(std::to_string(m_port)));

		m_response = HttpResponse(e.code(), e.what(), m_request.ePage(e.code()), e.redir(), m_request.closeConnection(), getTimeoutDuration());

    } catch (...) {
		m_request.setState(COMPLETE);

		m_response = HttpResponse(500, "Internal Server Error: unknown error occurred", m_request.ePage(500), "", true, getTimeoutDuration());
	}
}

void	Client::appendRequest(std::vector<char>& request)
{
	m_request.appendRequest(request);
}

const HttpResponse&	Client::remoteClosedConnection()
{
	static const HttpResponse instance(0, "Remote Closed Connection", "", "", 2, (t_ms)5000);
	return instance;
}

int	Client::closeConnection()
{
	return m_response.closeConnection();
}

e_state	Client::requestState()
{
	return m_request.state();
}

e_type	Client::sendType()
{
	return m_response.sendType();
}

std::string	Client::response()
{
	return m_response.response();
}

std::string	Client::header()
{
	return m_response.header();
}

std::string	Client::path()
{
	return m_request.path();
}
