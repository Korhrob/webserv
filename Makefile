CXX			=	c++ -std=c++17
CXXFLAGS	=	-Iinc -Wall -Werror -Wextra
LDFLAG		=	#-g -fsanitize=address
SNAME		=	server
SSRC		=	src/main.cpp			src/Server.cpp			src/Client.cpp		\
				src/Logger.cpp			src/Config.cpp			src/ConfigNode.cpp	\
				src/HttpResponse.cpp	src/HttpRequest.cpp		src/Directory.cpp	\
				src/CGI.cpp
SOBJ		=	$(patsubst src/%.cpp, obj/%.o, $(SSRC))
CONST		=	inc/Const.hpp

all: $(SNAME) logs

$(SNAME): $(SOBJ)
	$(CXX) $(CXXFLAGS) -o $(SNAME) $(SOBJ) $(LDFLAG)
	touch cgi-bin/people.txt
	@echo "Built $(SNAME)"

obj/%.o: src/%.cpp $(CONST) | obj/
	$(CXX) $(CXXFLAGS)  -c $< -o $@

obj/:
	mkdir -p obj

logs:
	mkdir -p logs
	@if [ ! -f logs/log ]; then touch logs/log; fi
	@if [ ! -f logs/error ]; then touch logs/error; fi

clean:
	rm -f $(SOBJ)
	@if [ -f logs/log ]; then rm logs/log; fi
	@if [ -f logs/error ]; then rm logs/error; fi

fclean: clean
	rm -f $(SNAME)
	rm -rf obj
	rm -rf cgi-bin/people.txt
	rm -rf www/upload/*

re: fclean all

.PHONY: all clean fclean re

#valgrind --leak-check=full --track-origins=yes --track-fds=yes ./server 
#curl -X POST http://localhost:8080/people.php -F "first_name=bob" -F "last_name=marley" --trace-ascii -
