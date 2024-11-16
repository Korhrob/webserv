CC			=	c++
CFLAGS		=	#-Wall -Werror -Wextra -g
LDFLAG		=	#-g -fsanitize=address
SNAME		=	server
CNAME		=	client
SSRC		=	Server.cpp
CSRC		=	Client.cpp
SOBJ		=	$(SSRC:.cpp=.o)
COBJ		=	$(CSRC:.cpp=.o)

all: $(SNAME) $(CNAME)

$(SNAME): $(SOBJ)
	$(CC) -o $(SNAME) $(SOBJ) $(LDFLAG)
	@echo $(SNAME)

$(CNAME): $(COBJ)
	$(CC) -o $(CNAME) $(COBJ) $(LDFLAG)
	@echo $(CNAME)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SOBJ)
	rm -f $(COBJ)

fclean: clean
	rm -f $(SNAME)
	rm -f $(CNAME)

re: fclean all

.PHONY: all clean fclean re