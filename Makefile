CXX			=	c++ -std=c++17
CXXFLAGS	=	-Iinc -Wall -Werror -Wextra -g
LDFLAG		=	#-g -fsanitize=address
SNAME		=	server
SSRC		=	src/main.cpp src/Server.cpp src/Parse.cpp src/Response.cpp src/ILog.cpp src/Config.cpp src/ConfigNode.cpp
SOBJ		=	$(patsubst src/%.cpp, obj/%.o, $(SSRC)) # $(SSRC:.cpp=.o)

all: $(SNAME) 

$(SNAME): $(SOBJ)
	$(CXX) $(CXXFLAGS) -o $(SNAME) $(SOBJ) $(LDFLAG)
	@echo "Built $(SNAME)"

obj/%.o: src/%.cpp | obj/
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/:
	mkdir -p obj

clean:
	rm -f $(SOBJ)

fclean: clean
	rm -f $(SNAME)
	rm -rf obj

re: fclean all

.PHONY: all clean fclean re