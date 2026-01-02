# flags
_CFLAGS := -O2 -std=c11 -pipe \
           -Wall -Wextra -Wformat    \
           -Werror=implicit-function-declaration -Werror=int-conversion

override CFLAGS := $(_CFLAGS) $(CFLAGS)

# files
SRC :=	src/str_mem.c \
	src/str_clone.c \
	src/str_hash.c \
	src/str_concat_array.c \
	src/str_join_array.c \
	src/str_span_chars.c \
	src/str_span_nonmatching_chars.c \
	src/str_span_until_substring.c \
	src/str_sprintf.c \
	src/str_repeat.c \
	src/str_replace_substring.c \
	src/str_replace_chars.c \
	src/str_replace_char_spans.c \
	src/str_decode_utf8.c \
	src/str_count_codepoints.c \
	src/str_to_valid_utf8.c \
	src/str_encode_codepoint.c \
	src/str_concat_array_to_stream.c \
	src/str_read_all_file.c \
	src/str_concat_array_to_fd.c \
	src/str_get_line.c \
	src/str_sort.c \
	src/str_partition_array.c \
	src/str_unique_partition_array.c

OBJ := $(SRC:.c=.o)
LIB := libstr.a

# targets
.PHONY: lib clean test stat

# clear targets on error
.DELETE_ON_ERROR:

# library target
lib: $(LIB)

# library
$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $?

# header dependencies
$(OBJ): str.h src/str_impl.h
src/str_hash.o: src/rapidhash/rapidhash.h

# testing
TBIN := test-str
TSRC := src/test.c \
	src/test_utf8.c \
	src/test_utf8_validator.c \
	src/test_io.c \
	src/mite/mite.c \
	$(LIB)

# sanitisers only work for non-musl builds
ifneq ($(CC),musl-gcc)
CC_SAN := -fsanitize=address \
	  -fsanitize=leak  \
	  -fsanitize=null \
	  -fsanitize=undefined \
	  -fsanitize-address-use-after-scope
endif

test: $(TBIN)

$(TBIN): $(TSRC) str.h src/mite/mite.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(CC_SAN) -o $@ $(TSRC)
	chmod 0700 $@
	./$@

# cleanup
clean:
	$(RM) $(LIB) src/*.o $(TBIN)

# statistics
stat:
	@echo SRC = $(SRC)
	@echo LIB = $(LIB)
	@nm $(LIB)
