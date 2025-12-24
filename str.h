/*
BSD 3-Clause License

Copyright (c) 2025 Maxim Konakov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// string type ------------------------------------------------------------------------------------
typedef struct {
	const char* ptr;
	size_t prop;
} str;

// empty string
#define str_null ((str){ 0 })

// helper macros (not for general use)
#define str_ref_prop(n)			((n) << 1)
#define str_owner_prop(n)		(str_ref_prop(n) | 1)
#define str_mask_owner(prop)	((prop) & ~(size_t)1)

// string properties ------------------------------------------------------------------------------
// length of the string
static inline
size_t str_len(const str s) { return s.prop >> 1; }

// pointer to the string
static inline
const char* str_ptr(const str s) { return s.ptr ? s.ptr : ""; }

// end of the string
static inline
const char* str_end(const str s) { return str_ptr(s) + str_len(s); }

// test if the string is empty
static inline
bool str_is_empty(const str s) { return str_len(s) == 0; }

// test if the string is allocated on the heap
static inline
bool str_is_owner(const str s) { return (s.prop & 1) != 0; }

// test if the string is a reference
static inline
bool str_is_ref(const str s) { return !str_is_owner(s); }

// hash the string
uint64_t str_hash(const str s);

// string memory control --------------------------------------------------------------------------
// free memory allocated for the string
static inline
void str_free(const str s) {
	if(str_is_owner(s))
		free((void*)s.ptr);
}

// clear string
static inline
void str_clear(str* const s) {
	str_free(*s);
	*s = str_null;
}

// free target string, then assign a new value to it
static inline
void str_assign(str* const dest, const str s) {
	str_free(*dest);
	*dest = s;
}

// automatic cleanup
#define str_auto	str __attribute__((cleanup(str_clear)))

// string constructors ----------------------------------------------------------------------------
// create a reference to the given string literal
#define str_lit(s)	((str){ "" s, str_ref_prop(sizeof(s) - 1) })

// create a reference to the given string
static inline
str str_ref(const str s) {
	return (str){ s.ptr, str_mask_owner(s.prop) };
}

// create a reference to the given range of chars
static inline
str str_ref_mem(const char* const s, const size_t n) {
	return (s && n > 0) ? ((str){ s, str_ref_prop(n) }) : str_null;
}

// create a reference to the given C string
static inline
str str_ref_ptr(const char* const s) { return str_ref_mem(s, s ? strlen(s) : 0); }

// create a reference to a slice of a string
static inline
str str_ref_slice(const str s, const size_t i, const size_t j) {
	size_t len = str_len(s);

	len = (j <= i || i >= len) ? 0
		: (j > len) ? (len - i)
		: (j - i);

	return str_ref_mem(str_ptr(s) + i, len);
}

// take ownership of the given string
static inline
str str_acquire(str* const ps) {
	const str t = *ps;

	ps->prop = str_mask_owner(ps->prop);
	return t;
}

// take ownership of the given memory area
static inline
str str_acquire_mem(const char* const s, const size_t n) {
	if(s && n > 0)
		return (str){ s, str_owner_prop(n) };

	free((void*)s);
	return str_null;
}

// take ownership of the given C string
static inline
str str_acquire_ptr(const char* const s) { return str_acquire_mem(s, s ? strlen(s) : 0);}

// string comparison ------------------------------------------------------------------------------
// compare two strings
static inline
int str_cmp(const str s1, const str s2) {
	const size_t n1 = str_len(s1), n2 = str_len(s2);
	const int res = memcmp(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

	return (res != 0 || n1 == n2) ? res : (n1 < n2) ? -1 : 1;
}

// test if the two strings match
static inline
bool str_eq(const str s1, const str s2) { return str_cmp(s1, s2) == 0; }

// test for prefix
static inline
bool str_has_prefix(const str s, const str prefix) {
	const size_t n = str_len(prefix);

	return (n == 0)
		|| (str_len(s) >= n && memcmp(str_ptr(s), str_ptr(prefix), n) == 0);
}

// test for suffix
static inline
bool str_has_suffix(const str s, const str suffix) {
	const size_t n = str_len(suffix);

	return (n == 0)
		|| (str_len(s) >= n && memcmp(str_end(s) - n, str_ptr(suffix), n) == 0);
}

// operations on strings --------------------------------------------------------------------------
// swap two string objects
static inline
void str_swap(str* const s1, str* const s2) {
	const str tmp = *s1;

	*s1 = *s2;
	*s2 = tmp;
}

// sprintf and assign
bool str_sprintf(str* const dest, const char* const fmt, ...) __attribute__((format(printf,2,3)));

// allocate and assign a copy of the given string
void str_clone(str* const dest, const str s);

// concatenate array of strings
void str_concat_array(str* const dest, const str* array, const size_t count);

// concatenate string arguments
#define str_concat(dest, ...) ({	\
	const str args[] = { __VA_ARGS__ };	\
	str_concat_array((dest), args, sizeof(args)/sizeof(args[0]));	\
})

// join array of strings around a separator
void str_join_array(str* const dest, const str sep, const str* array, size_t count);

// join string arguments around a separator
#define str_join(dest, sep, ...) ({	\
	const str args[] = { __VA_ARGS__ };	\
	str_join_array((dest), (sep), args, sizeof(args)/sizeof(args[0]));	\
})

// repeat the given string `n` times
void str_repeat(str* const s, size_t n);

// search -----------------------------------------------------------------------------------------
// span the initial part of the string `s` as long as the characters from `s` occur
// in string `charset`, and return the number of characters spanned
size_t str_span_chars(const str s, const str charset);

// span the initial part of the string `s` as long as the characters from `s` do not occur
// in string `charset`, and return the number of characters spanned
size_t str_span_nonmatching_chars(const str s, const str charset);

// span the initial part of the string `s` until an instance of `substr` is found,
// and return the number of characters spanned
size_t str_span_until_substring(const str s, const str substr);

// search & replace -------------------------------------------------------------------------------
// replace every occurrence of `patt` with `repl`
size_t str_replace_substring(str* const dest, const str patt, const str repl);

// replace every occurrence of any byte from `charset` with `repl`
size_t str_replace_chars(str* const dest, const str charset, const str repl);

// replace every span of bytes from `charset` with `repl`
size_t str_replace_char_spans(str* const dest, const str charset, const str repl);

// Unicode ----------------------------------------------------------------------------------------
// result bitfield
typedef struct {
	uint32_t status		: 2;   // status: OK, ERROR, or INCOMPLETE
	uint32_t num_bytes	: 3;   // bytes to advance (1-4, 0=stop)
	uint32_t utf8_len	: 3;   // UTF-8 sequence length (1-4)
	uint32_t codepoint	: 24;  // Unicode codepoint (0-0x10FFFF), 0 on error
} str_decode_result;	// 32 bits total

// status codes
#define STR_UTF8_OK			0
#define STR_UTF8_ERROR		1
#define STR_UTF8_INCOMPLETE	2

static inline
str_decode_result str_decode_utf8(const char* src, size_t len) {
	extern str_decode_result str_decode_utf8_impl(const uint8_t* const, const size_t);

	if (!src || len == 0)
		return (str_decode_result){ .status = STR_UTF8_OK };

	const uint8_t c = (uint8_t)*src;

	if(c < 0x80)
		return (str_decode_result){
			.status = STR_UTF8_OK,
			.num_bytes = 1,
			.utf8_len = 1,
			.codepoint = c
		};

	return str_decode_utf8_impl((const uint8_t*)src, len);
}

// count number of codepoints in a string
size_t str_count_codepoints(const str s);

// convert a string to valid UTF-8 encoded string
size_t str_to_valid_utf8(str* const dest);

// convert codepoint to UTF-8 sequence
size_t str_encode_codepoint(char* const p, uint32_t cp);

// I/O --------------------------------------------------------------------------------------------
// write array of strings to the file stream
int str_concat_array_to_stream(FILE* const stream, const str* src, const size_t count);

// write strings to the file stream
#define str_concat_to_stream(stream, ...) ({	\
	const str args[] = { __VA_ARGS__ };	\
	str_concat_array_to_stream((stream), args, sizeof(args)/sizeof(args[0]));	\
})

// write array of strings to the file descriptor
int str_concat_array_to_fd(const int fd, const str* src, const size_t count);

// write strings to the file descriptor
#define str_concat_to_fd(fd, ...) ({	\
	const str args[] = { __VA_ARGS__ };	\
	str_concat_array_to_fd((fd), args, sizeof(args)/sizeof(args[0]));	\
})

// read the entire file into a string
int str_read_all_file(str* const dest, const char* const file_name);

// read one line from a stream
int str_get_line(str* const dest, FILE* const stream, const int delim);

// sorting ----------------------------------------------------------------------------------------
// comparison functions
typedef int (*str_cmp_func)(const void*, const void*);

int str_order_asc(const void* const s1, const void* const s2);
int str_order_desc(const void* const s1, const void* const s2);

// sort array
static inline
void str_sort_array(const str_cmp_func cmp, const str* const array, const size_t count) {
	if(array && count > 1)
		qsort((void*)array, count, sizeof(array[0]), cmp);
}

// partitioning
size_t str_partition_array(bool (*pred)(const str), str* const array, const size_t count);

// unique partitioning
size_t str_unique_partition_array(str* const array, const size_t count);

#ifdef __cplusplus
}
#endif
