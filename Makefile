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

TEST_BIN := tests/bin/test_reader_$(BUFFER_SIZE)
TEST_SRC := tests/test_main.c tests/test_reader.c

.PHONY: all bonus clean fclean re test check

all: $(NAME)

bonus: all

$(NAME): $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

$(OBJ_DIR)/%.o: %.c get_next_line.h
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_BIN): $(NAME) $(TEST_SRC) tests/test.h
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRC) $(NAME) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

check:
	git diff --check
	$(MAKE) fclean
	$(MAKE) all
	$(MAKE) test
	$(MAKE) -q all

clean:
	$(RMDIR) build tests/bin

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP)
