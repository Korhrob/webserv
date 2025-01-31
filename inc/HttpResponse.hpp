#pragma once

#include <string>

enum	e_type
{
	TYPE_BLANK,
	TYPE_SINGLE,
	TYPE_CHUNK,
	TYPE_NONE
};

class HttpResponse {
	private:
		int												m_code;
		std::string										m_message;
		std::string										m_body;
		e_type											m_send_type;
		// headers & addHeader function?

	public:
		HttpResponse(int code, std::string msg);
};
