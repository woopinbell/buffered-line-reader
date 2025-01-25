NAME := libbuffered_line_reader.a

CC          ?= cc
CFLAGS      ?= -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude -MMD -MP
CPPFLAGS    ?= -DBUFFER_SIZE=$(BUFFER_SIZE)
AR          ?= ar
ARFLAGS     ?= rcs
BUFFER_SIZE ?= 42

SRC         := $(wildcard src/*.c)
BIN_DIR     := build
OBJ_DIR     := $(BIN_DIR)/obj/$(BUFFER_SIZE)
OBJS        := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEPS        := $(OBJS:.o=.d)
TEST_SRC    := $(wildcard tests/test_*.c)
TEST_BIN    := $(BIN_DIR)/test/test_reader_$(BUFFER_SIZE)
TEST_ASAN_BIN := $(BIN_DIR)/test/test_reader_$(BUFFER_SIZE)_asan

.PHONY: all clean fclean re test test-asan

all: $(BIN_DIR)/$(NAME)

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: %.c include/get_next_line.h | $(BIN_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/$(NAME): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(TEST_BIN): $(TEST_SRC) tests/test.h $(OBJS) | $(BIN_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRC) $(OBJS) -o $@

test: $(TEST_BIN)
	@$(TEST_BIN)
	@printf 'get_next_line tests (BUFFER_SIZE=%s): PASS\n' $(BUFFER_SIZE)

$(TEST_ASAN_BIN): $(TEST_SRC) tests/test.h $(SRC) include/get_next_line.h | $(BIN_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined -g -O0 \
		$(TEST_SRC) $(SRC) -o $@

test-asan: $(TEST_ASAN_BIN)
	@$(TEST_ASAN_BIN)
	@printf 'get_next_line tests (ASAN, BUFFER_SIZE=%s): PASS\n' $(BUFFER_SIZE)

clean:
	rm -rf $(BIN_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)
