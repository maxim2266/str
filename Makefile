# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# flags
CFLAGS := -ggdb -std=c11 -Wall -Wextra -Werror=implicit-function-declaration	\
	-fno-omit-frame-pointer	\
	-fsanitize=address -fsanitize=leak -fsanitize=undefined	\
	-fsanitize-address-use-after-scope

# source files
SRC := str.c str.h str_test.c

# compiler
CC := gcc
#CC := clang

# test
test: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
	./$@

# clean-up
.PHONY: clean
clean:
	rm -f test
