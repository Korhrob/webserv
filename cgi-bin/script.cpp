#include <iostream>

int main()
{
	std::cout << "<html><body>";
	std::cout << "<h1>Hello from CGI!</h1>";
	std::cout << "</body></html>";
	return 0;
}

// compile with "g++ -o script.cgi script.cpp"