# flags
CC_WARN := -Wall -Wextra -Werror=implicit-function-declaration -Wformat -Werror=format-security

ifeq ($(CC),musl-gcc)
# musl is ISO 10646 compliant but doesn't define __STDC_ISO_10646__
CC_EXTRA := -D__STDC_ISO_10646__=201706L
else
# sanitisers only work for non-musl builds
CC_SAN := -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope
endif

test:      CFLAGS := -ggdb -std=c11 -pipe $(CC_WARN) $(CC_EXTRA) -fno-omit-frame-pointer $(CC_SAN)
flto-test: CFLAGS := -s -O2 -pipe -std=c11 $(CC_WARN) $(CC_EXTRA) -flto -march=native -mtune=native
tools:     CFLAGS := -s -O2 -pipe -std=c11 $(CC_WARN) $(CC_EXTRA)

# str library source files
SRC := str.c str.h str_test.c

# all
.PHONY: all
all: tools test flto-test

.PHONY: clean
clean: clean-test clean-tools

# test
test: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
	./$@

flto-test: $(SRC)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
	./$@

.PHONY: clean-test
clean-test:
	rm -f test flto-test

# tools
GEN_CHAR_CLASS := tools/gen-char-class

.PHONY: tools
tools: $(GEN_CHAR_CLASS)

# gen-char-class
$(GEN_CHAR_CLASS): tools/gen_char_class.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

.PHONY: clean-tools
clean-tools:
	rm -f $(GEN_CHAR_CLASS)
