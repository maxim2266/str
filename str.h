/*
BSD 3-Clause License

Copyright (c) 2020,2021, Maxim Konakov
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

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// string type ----------------------------------------------------------------------------
typedef struct
{
	const char* ptr;
	size_t info;
} str;

// NULL string
#define str_null ((str){ 0 })

// helper macros
#define _ref_info(n)	((n) << 1)
#define _owner_info(n)	(_ref_info(n) | 1)

// string properties ----------------------------------------------------------------------
// length of the string
static inline
size_t str_len(const str s) { return s.info >> 1; }

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
bool str_is_owner(const str s) { return (s.info & 1) != 0; }

// test if the string is a reference
static inline
bool str_is_ref(const str s) { return !str_is_owner(s); }

// string memory control -------------------------------------------------------------------
// free memory allocated for the string
void str_free(const str s);

// string movements -----------------------------------------------------------------------
// free target string, then assign the new value to it
static inline
void str_assign(str* const ps, const str s) { str_free(*ps); *ps = s; }

// move the string, resetting the source to str_null
static inline
str str_move(str* const ps) { const str t = *ps; *ps = str_null; return t; }

// swap two string objects
void str_swap(str* const s1, str* const s2);

// string helpers --------------------------------------------------------------------------
// reset the string to str_null
static inline
void str_clear(str* const ps) { str_assign(ps, str_null); }

// compare two strings lexicographically
int str_cmp(const str s1, const str s2);

// test if two strings match
static inline
bool str_eq(const str s1, const str s2) { return str_cmp(s1, s2) == 0; }

// case-insensitive comparison
int str_cmp_ci(const str s1, const str s2);

// case-insensitive match
static inline
bool str_eq_ci(const str s1, const str s2) { return str_cmp_ci(s1, s2) == 0; }

// test for prefix
bool str_has_prefix(const str s, const str prefix);

// test for suffix
bool str_has_suffix(const str s, const str suffix);

// string composition ------------------------------------------------------------------
// implementation helpers
int _str_dup(str* const dest, const str s);
int _str_cpy_to_fd(const int fd, const str s);
int _str_cpy_to_stream(FILE* const stream, const str s);

// copy string
#define str_cpy(dest, src)	\
	_Generic((dest),	\
		str*:	_str_dup,	\
		int:	_str_cpy_to_fd,	\
		FILE*:	_str_cpy_to_stream	\
	)((dest), (src))

// implementation helpers
int _str_cat_range(str* const dest, const str* src, size_t count);
int _str_cat_range_to_fd(const int fd, const str* src, size_t count);
int _str_cat_range_to_stream(FILE* const stream, const str* src, size_t count);

// concatenate range of strings
#define str_cat_range(dest, src, count)	\
	_Generic((dest),	\
		str*:	_str_cat_range,	\
		int:	_str_cat_range_to_fd,	\
		FILE*:	_str_cat_range_to_stream	\
	)((dest), (src), (count))

// concatenate string arguments
#define str_cat(dest, ...)	\
({	\
	const str args[] = { __VA_ARGS__ };	\
	str_cat_range((dest), args, sizeof(args)/sizeof(args[0]));	\
})

// implementation helpers
int _str_join_range(str* const dest, const str sep, const str* src, size_t count);
int _str_join_range_to_fd(const int fd, const str sep, const str* src, size_t count);
int _str_join_range_to_stream(FILE* const stream, const str sep, const str* src, size_t count);

// join strings around the separator
#define str_join_range(dest, sep, src, count)	\
	_Generic((dest),	\
		str*:	_str_join_range,	\
		int:	_str_join_range_to_fd,	\
		FILE*:	_str_join_range_to_stream	\
	)((dest), (sep), (src), (count))

// join string arguments around the separator
#define str_join(dest, sep, ...)	\
({	\
	const str args[] = { __VA_ARGS__ };	\
	str_join_range((dest), (sep), args, sizeof(args)/sizeof(args[0]));	\
})

// constructors ----------------------------------------------------------------------------
// string reference from a string literal
#define str_lit(s)	((str){ "" s, _ref_info(sizeof(s) - 1) })

static inline
str _str_ref(const str s) { return (str){ s.ptr, s.info & ~(size_t)1 }; }

str _str_ref_form_ptr(const char* const s);

// string reference from anything
#define str_ref(s)	\
	_Generic((s),	\
		str:			_str_ref,	\
		char*:			_str_ref_form_ptr,	\
		const char*:	_str_ref_form_ptr	\
	)(s)

// create a reference to the given range of chars
str str_ref_chars(const char* const s, const size_t n);

// take ownership of the given range of chars
str str_acquire_chars(const char* const s, const size_t n);

// take ownership of the given string
str str_acquire(const char* const s);

// string from file
int str_from_file(str* const dest, const char* const file_name);

// searching and sorting --------------------------------------------------------------------
// string partitioning (substring search)
bool str_partition(const str src, const str patt, str* const prefix, str* const suffix);

// comparison functions
typedef int (*str_cmp_func)(const void*, const void*);

int str_order_asc(const void* const s1, const void* const s2);
int str_order_desc(const void* const s1, const void* const s2);
int str_order_asc_ci(const void* const s1, const void* const s2);
int str_order_desc_ci(const void* const s1, const void* const s2);

// sort array of strings
void str_sort_range(const str_cmp_func cmp, str* const array, const size_t count);

// searching
const str* str_search_range(const str key, const str* const array, const size_t count);

// partitioning
size_t str_partition_range(bool (*pred)(const str), str* const array, const size_t count);

// unique partitioning
size_t str_unique_range(str* const array, const size_t count);

// UTF-32 codepoint iterator ----------------------------------------------------------------
#ifdef __STDC_UTF_32__
#include <uchar.h>

// iterator
#define for_each_codepoint(var, src)	\
	_for_each_cp((var), (src), _CAT(__it_, __COUNTER__))

// iterator error codes
#define CPI_END_OF_STRING			((char32_t)-1)
#define CPI_ERR_INCOMPLETE_SEQ		((char32_t)-2)
#define CPI_ERR_INVALID_ENCODING	((char32_t)-3)

// implementation
#define _for_each_cp(var, src, it)	\
	for(_cp_iterator it = _make_cp_iterator(src); (var = _cp_iterator_next(&it)) <= 0x10FFFFu;)

#define _CAT(x, y)	__CAT(x, y)
#define __CAT(x, y)	x ## y

typedef struct
{
	const char* curr;
	const char* const end;
	mbstate_t state;
} _cp_iterator;

static inline
_cp_iterator _make_cp_iterator(const str s)
{
	return (_cp_iterator){ .curr = str_ptr(s), .end = str_end(s) };
}

char32_t _cp_iterator_next(_cp_iterator* const it);

#endif	// ifdef __STDC_UTF_32__

// tokeniser --------------------------------------------------------------------------------
typedef struct
{
	unsigned char bits[32];	// 256 / 8
	const char *src, *end;
} str_tok_state;

void str_tok_init(str_tok_state* const state, const str src, const str delim_set);
bool str_tok(str* const dest, str_tok_state* const state);
void str_tok_delim(str_tok_state* const state, const str delim_set);

#ifdef __cplusplus
}
#endif
