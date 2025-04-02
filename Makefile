# Suprimir la salida de los comandos make
MAKEFLAGS += --silent

################################################################################
### COLORS
################################################################################

DEL_LINE =              \033[2K
DEF_COLOR =             \033[0;39m
RED =                   \033[0;91m
GREEN =                 \033[0;92m
YELLOW =                \033[0;93m
BLUE =                  \033[0;94m
MAGENTA =               \033[0;95m
CYAN =                  \033[0;96m
BROWN =                 \033[38;2;184;143;29m

################################################################################

# Variables
NAME            = ircserv
CC              = c++
FLAGS           = -g -Wall -Werror -Wextra -std=c++98 -MMD -MP #-fsanitize=address
RM              = rm -rf

# Directorios
DIR_OBJ         = temp/

# Archivos de cabecera y fuentes
HEADERS         = Channel.hpp Client.hpp Command.hpp Server.hpp
SRC             = Channel.cpp Client.cpp Command.cpp Server.cpp main.cpp

# Objetos y dependencias
OBJ             = $(addprefix $(DIR_OBJ), $(SRC:.cpp=.o))
DEPS            = $(SRC:%.cpp=$(DIR_OBJ)%.d)

# Incluir archivos de dependencias generados
-include $(DEPS)

################################################################################
### RULES
################################################################################

all: $(DIR_OBJ) $(NAME)
	@echo "$(GREEN)$(NAME) is up to date ✓$(DEF_COLOR)\n"

$(DIR_OBJ):
	@mkdir -p $(DIR_OBJ)

$(DIR_OBJ)%.o: %.cpp $(HEADERS) Makefile | $(DIR_OBJ)
	@$(CC) $(FLAGS) -c $< -o $@
	@echo "${BLUE} ◎ $(BROWN)Compiling   ${MAGENTA}→   $(CYAN)$< $(DEF_COLOR)"

$(NAME): $(OBJ)
	@$(CC) $(FLAGS) $(OBJ) -o $(NAME) || $(RM) $(OBJ)
	@echo "\n$(GREEN)Created $(NAME) ✓$(DEF_COLOR)"

clean:
	@$(RM) $(DIR_OBJ)
	@echo "${RED}Objects and dependencies successfully removed${DEF_COLOR}"

fclean: clean
	@$(RM) $(NAME)
	@echo "${RED}Executables successfully removed${DEF_COLOR}"

re: fclean all

# Establecer el objetivo predeterminado
.DEFAULT_GOAL := all

.PHONY: all clean fclean re