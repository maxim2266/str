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

#include "str.h"

#define _DEFAULT_SOURCE	// for strncasecmp()
#include <string.h>

// compatibility
#ifndef _GNU_SOURCE
static inline
void* mempcpy(void* dest, const void* src, const size_t n)
{
	return memcpy(dest, src, n) + n;
}
#endif

// memory allocation
#ifdef STR_EXT_ALLOC

// these functions are defined elsewhere
void* str_mem_alloc(size_t);	// this should also handle out of memory condition
void str_mem_free(void*);

#else

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static __attribute__((malloc))
void* str_mem_alloc(const size_t n)
{
	void* const p = malloc(n);

	if(p)
		return p;

	if(errno == 0)	// a fix for some sub-standard memory allocators
		errno = ENOMEM;

	perror("fatal error");
	abort();
}

#define str_mem_free free

#endif	// #ifdef STR_EXT_ALLOC

// string deallocation
void str_free(const str s)
{
	if(str_is_owner(s) && s.ptr)
		str_mem_free((void*)s.ptr);
}

// compare two strings lexicographically
int str_cmp(const str s1, const str s2)
{
	const size_t n1 = str_len(s1), n2 = str_len(s2);

	// either string may be missing a null terminator, hence "memcmp"
	const int res = memcmp(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

	if(res != 0 || n1 == n2)
		return res;

	return (n1 < n2) ? -1 : 1;
}

// case-insensitive comparison
int str_case_cmp(const str s1, const str s2)
{
	const size_t n1 = str_len(s1), n2 = str_len(s2);

	// either string may be missing a null terminator, hence "strNcasecmp"
	const int res = strncasecmp(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

	if(res != 0 || n1 == n2)
		return res;

	return (n1 < n2) ? -1 : 1;
}

// create a reference to the given range of chars
str str_ref_chars(const char* const s, const size_t n)
{
	return (s && n > 0) ? ((str){ s, _ref_info(n) }) : str_null;
}

str _str_ref_form_ptr(const char* const s)
{
	return s ? str_ref_chars(s, strlen(s)) : str_null;
}

// take ownership of the given range of chars
void str_acquire_chars(str* const dest, const char* const s, size_t n)
{
	// take ownership even if the string is empty, because its memory is still allocated
	str_assign(dest, s ? ((str){ s, _owner_info(n) }) : str_null);
}

// take ownership of the given C string
void str_acquire(str* const dest, const char* const s)
{
	// take ownership even if the string is empty, because its memory is still allocated
	str_assign(dest, s ? ((str){ s, _owner_info(strlen(s)) }) : str_null);
}

// allocate a copy of the given string
void str_dup(str* const dest, const str s)
{
	const size_t n = str_len(s);

	if(n == 0)
		str_clear(dest);
	else
	{
		char* const p = memcpy(str_mem_alloc(n + 1), str_ptr(s), n);

		p[n] = 0;

		str_acquire_chars(dest, p, n);
	}
}

// append string
static inline
char* append_str(char* p, const str s)
{
	return mempcpy(p, str_ptr(s), str_len(s));
}

// handle simple cases for cat and join functions
static
bool simple_cat(str* const dest, const str* const src, const size_t n)
{
	if(!src || n == 0)
	{
		str_clear(dest);
		return true;
	}

	if(n == 1)
	{
		str_dup(dest, src[0]);
		return true;
	}

	return false;
}

static
size_t total_length(const str* const src, const size_t n)
{
	size_t sum = 0;

	for(size_t i = 0; i < n; ++i)
		sum += str_len(src[i]);

	return sum;
}

// concatenate strings
void str_cat_range(str* const dest, const str* const src, const size_t n)
{
	// test for simple cases
	if(simple_cat(dest, src, n))
		return;

	// calculate total length
	const size_t num = total_length(src, n);

	if(num == 0)
	{
		str_clear(dest);
		return;
	}

	// allocate
	char* const buff = str_mem_alloc(num + 1);

	// copy bytes
	char* p = buff;

	for(size_t i = 0; i < n; ++i)
		p = append_str(p, src[i]);

	// null-terminate and assign
	*p = 0;
	str_acquire_chars(dest, buff, num);
}

// join strings
static
bool simple_join(str* const dest, const str sep, const str* const src, const size_t n)
{
	if(str_is_empty(sep))
	{
		str_cat_range(dest, src, n);
		return true;
	}

	return simple_cat(dest, src, n);
}

void str_join_range(str* const dest, const str sep, const str* const src, const size_t n)
{
	// test for simple cases
	if(simple_join(dest, sep, src, n))
		return;

	// calculate total length
	const size_t num = total_length(src, n) + str_len(sep) * (n - 1);

	// allocate
	char* const buff = str_mem_alloc(num + 1);

	// copy bytes
	char* p = append_str(buff, src[0]);

	for(size_t i = 1; i < n; ++i)
		p = append_str(append_str(p, sep), src[i]);

	// null-terminate and assign
	*p = 0;
	str_acquire_chars(dest, buff, num);
}

void str_join_range_ignore_empty(str* const dest, const str sep, const str* const src, const size_t n)
{
	// test for simple cases
	if(simple_join(dest, sep, src, n))
		return;

	// calculate total length, also trimming empty strings from both ends of the range
	// 1. find first non-empty string index
	size_t num_bytes = str_len(src[0]), first = 0;

	while(num_bytes == 0 && ++first < n)
		num_bytes = str_len(src[first]);

	if(first == n)
	{
		str_clear(dest);
		return;
	}

	// 2. find last non-empty string index
	size_t last = n;

	while(str_is_empty(src[--last]));

	// 3. calculate total length ignoring empty strings
	for(size_t i = first + 1; i <= last; ++i)
		if(!str_is_empty(src[i]))
			num_bytes += str_len(sep) + str_len(src[i]);

	// allocate
	char* const buff = str_mem_alloc(num_bytes + 1);

	// copy bytes
	char* p = append_str(buff, src[first]);

	for(size_t i = first + 1; i <= last; ++i)
		if(!str_is_empty(src[i]))
			p = append_str(append_str(p, sep), src[i]);

	// null-terminate and acquire
	*p = 0;
	str_acquire_chars(dest, buff, num_bytes);
}