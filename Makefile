CXX			=	c++
CXXFLAGS	=	-Iinc #-Wall -Werror -Wextra -g
LDFLAG		=	#-g -fsanitize=address
SNAME		=	server
SSRC		=	src/main.cpp src/Server.cpp src/Parse.cpp src/Response.cpp
SOBJ		=	$(patsubst src/%.cpp, obj/%.o, $(SSRC)) # $(SSRC:.cpp=.o)

all: $(SNAME) 

$(SNAME): $(SOBJ)
	$(CXX) $(CXXFLAGS) -o $(SNAME) $(SOBJ) $(LDFLAG)
	@echo "Built $(SNAME)"

$(CNAME): $(COBJ)
	$(CXX) $(CXXFLAGS) -o $(CNAME) $(COBJ) $(LDFLAG)
	@echo "Built $(CNAME)"

obj/%.o: src/%.cpp | obj/
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj/:
	mkdir -p obj

clean:
	rm -f $(SOBJ)
	rm -f $(COBJ)

fclean: clean
	rm -f $(SNAME)
	rm -f $(CNAME)
	rm -rf obj

re: fclean all

.PHONY: all clean fclean re