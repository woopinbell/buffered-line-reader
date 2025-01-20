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
TEST_SRC := tests/test_main.c tests/test_reader.c tests/test_boundaries.c
MATRIX_SIZES := 1 2 42 1024

FAULT_OBJ_DIR := build/fault/$(BUFFER_SIZE)
FAULT_READER_OBJ := $(FAULT_OBJ_DIR)/get_next_line.o
FAULT_RUNTIME_OBJ := $(FAULT_OBJ_DIR)/fault_runtime.o
FAULT_TEST_OBJ := $(FAULT_OBJ_DIR)/test_failure.o
FAULT_OBJ := $(FAULT_READER_OBJ) $(FAULT_RUNTIME_OBJ) $(FAULT_TEST_OBJ)
FAULT_DEP := $(FAULT_OBJ:.o=.d)
FAULT_BIN := tests/bin/test_failure_$(BUFFER_SIZE)
FAULT_CPPFLAGS := $(CPPFLAGS) -Itests/support
FAULT_DEFINES := -Dmalloc=test_malloc -Dfree=test_free -Dread=test_read

.PHONY: all bonus clean fclean re test-run test failure-run failure-test check

all: $(NAME)

bonus: all

$(NAME): $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

$(OBJ_DIR)/%.o: %.c get_next_line.h
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJ) $(TEST_SRC) tests/test.h
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRC) $(OBJ) -o $@

test-run: $(TEST_BIN)
	./$(TEST_BIN)

test:
	@set -e; for size in $(MATRIX_SIZES); do \
		$(MAKE) --no-print-directory test-run BUFFER_SIZE=$$size; \
	done

$(FAULT_READER_OBJ): get_next_line.c get_next_line.h
	@$(MKDIR) $(dir $@)
	$(CC) $(FAULT_CPPFLAGS) $(FAULT_DEFINES) $(CFLAGS) $(DEPFLAGS) \
		-c $< -o $@

$(FAULT_RUNTIME_OBJ): tests/support/fault_runtime.c \
		tests/support/fault_runtime.h
	@$(MKDIR) $(dir $@)
	$(CC) $(FAULT_CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(FAULT_TEST_OBJ): tests/failure/test_failure.c get_next_line.h \
		tests/support/fault_runtime.h
	@$(MKDIR) $(dir $@)
	$(CC) $(FAULT_CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(FAULT_BIN): $(FAULT_OBJ)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(FAULT_OBJ) -o $@

failure-run: $(FAULT_BIN)
	./$(FAULT_BIN)

failure-test:
	@set -e; for size in $(MATRIX_SIZES); do \
		$(MAKE) --no-print-directory failure-run BUFFER_SIZE=$$size; \
	done

check:
	git diff --check
	$(MAKE) fclean
	$(MAKE) all
	$(MAKE) test
	$(MAKE) failure-test
	$(MAKE) -q all

clean:
	$(RMDIR) build tests/bin

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEP) $(FAULT_DEP)
