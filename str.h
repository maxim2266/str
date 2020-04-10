/*
BSD 3-Clause License

Copyright (c) 2020, Maxim Konakov
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

#include <stddef.h>
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

// end of string pointer
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
int str_case_cmp(const str s1, const str s2);

// case-insensitive match
static inline
bool str_case_eq(const str s1, const str s2) { return str_case_cmp(s1, s2) == 0; }

// concatenate strings
void str_cat_range(str* const dest, const str* const src, const size_t n);

// concatenate string arguments
#define str_cat(dest, ...)	\
	do {	\
		const str args[] = { __VA_ARGS__ };	\
		str_cat_range((dest), args, sizeof(args)/sizeof(args[0]));	\
	} while(0)

// join strings around the separator
void str_join_range(str* const dest, const str sep, const str* const src, const size_t n);

// join string arguments around the separator
#define str_join(dest, sep, ...)	\
	do {	\
		const str args[] = { __VA_ARGS__ };	\
		str_join_range((dest), (sep), args, sizeof(args)/sizeof(args[0]));	\
	} while(0)

// join strings around the separator ignoring empty ones
void str_join_range_ignore_empty(str* const dest, const str sep, const str* const src, const size_t n);

// join string arguments around the separator ignoring empty ones
#define str_join_ignore_empty(dest, sep, ...)	\
	do {	\
		const str args[] = { __VA_ARGS__ };	\
		str_join_range_ignore_empty((dest), (sep), args, sizeof(args)/sizeof(args[0]));	\
	} while(0)

// constructors ----------------------------------------------------------------------------
// string reference from a string literal
#define str_lit(s)	((str){ "" s, _ref_info(sizeof(s) - 1) })

// make a copy of the given string
void str_dup(str* const dest, const str s);

static inline
str _str_ref(const str s) { return (str){ s.ptr, s.info & ~(size_t)1 }; }

str _str_ref_form_ptr(const char* const s);

// string reference from anything
#define str_ref(s) _Generic((s),	\
		str:	_str_ref,	\
		char*:	_str_ref_form_ptr	\
	)(s)

// create a reference to the given range of chars
str str_ref_chars(const char* const s, const size_t n);

// take ownership of the given range of chars; totally unsafe, use at your own risk.
void str_acquire_chars(str* const dest, const char* const s, size_t n);

// take ownership of the given string; totally unsafe, use at your own risk
void str_acquire(str* const dest, const char* const s);

#ifdef __cplusplus
}
#endif
