#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpException.hpp"

void	Client::handleRequest(Config& config, std::vector<char>& vec)
{
	try {
		if (m_request.state() == COMPLETE)
			m_request.reset();

		m_request.appendRequest(vec);
		m_request.parseRequest(config);

		if (m_request.state() != HEADERS)
			setTimeoutDuration(m_request.timeoutDuration());

		if (m_request.state() == COMPLETE)
			m_response = m_request.processRequest(getTimeoutDuration());

	} catch (HttpException& e) {
		m_request.setState(COMPLETE);

		if (!m_request.server())
			m_request.setServer(config.findServerNode(std::to_string(m_port)));

		m_response = HttpResponse(e.code(), e.what(), m_request.ePage(e.code()), e.redir(), m_request.closeConnection(), getTimeoutDuration());

    } catch (std::exception& e) {
		m_request.setState(COMPLETE);

		m_response = HttpResponse(500, "Internal Server Error: unknown error occurred", m_request.ePage(500), "", true, getTimeoutDuration());
	}
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
