NAME := libbuffered_line_reader.a

CC := cc
override CFLAGS := -Wall -Wextra -Werror -Wpedantic -std=c99 \
	-fno-builtin
BUFFER_SIZE ?= 42
override CPPFLAGS := -I. -DBUFFER_SIZE=$(BUFFER_SIZE)
DEPFLAGS := -MMD -MP
AR := ar
ARFLAGS := rcs
RM := rm -f
RMDIR := rm -rf
MKDIR := mkdir -p

SRC := get_next_line.c
OBJ_DIR := build/obj/$(BUFFER_SIZE)
OBJ := $(SRC:%.c=$(OBJ_DIR)/%.o)
DEP := $(OBJ:.o=.d)

.PHONY: all bonus clean fclean re

all: $(NAME)

bonus: all

$(NAME): $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

$(OBJ_DIR)/%.o: %.c get_next_line.h
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	$(RMDIR) build

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP)
