NAME = ircserv

CC = c++

CFLAGS = -std=c++98 -Wall -Wextra -Werror -MMD -g -O0  #-fsanitize=address
SRC_FILES = main.cpp utils.cpp serverIRC.cpp channel.cpp
OBJ_DIR = objs/
OBJ_FILES = $(SRC_FILES:.cpp=.o)
OBJS = $(addprefix $(OBJ_DIR), $(OBJ_FILES))
DEP_FILES = $(SRC_FILES:.cpp=.d)
DEPS = $(addprefix $(OBJ_DIR), $(DEP_FILES))
INCLUDE = -I ./
RM = rm -f

all: $(NAME)

$(NAME): $(OBJ_DIR) $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) $(OBJS) -o $@
	@echo "Executable ready!"

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

-include $(DEPS)

clean:
	@$(RM) -r $(OBJ_DIR)
	@echo "Objects and dependencies successfully removed"

fclean: clean
	@$(RM) $(NAME)
	@echo "Executable and objects successfully removed"

re: fclean all


.PHONY: all clean fclean re