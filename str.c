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

#define _DEFAULT_SOURCE	// for strncasecmp()

#include "str.h"

#include <string.h>
#include <unistd.h>
#include <sys/uio.h>

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

// swap
void str_swap(str* const s1, str* const s2)
{
	const str tmp = *s1;

	*s1 = *s2;
	*s2 = tmp;
}

// string comparison ---------------------------------------------------------------------
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
int str_cmp_ci(const str s1, const str s2)
{
	const size_t n1 = str_len(s1), n2 = str_len(s2);

	// either string may be missing a null terminator, hence "strNcasecmp"
	const int res = strncasecmp(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

	if(res != 0 || n1 == n2)
		return res;

	return (n1 < n2) ? -1 : 1;
}

// test for prefix
bool str_has_prefix(const str s, const str prefix)
{
	const size_t n = str_len(prefix);

	return (n == 0)
		|| (str_len(s) >= n && memcmp(str_ptr(s), str_ptr(prefix), n) == 0);
}

// test for suffix
bool str_has_suffix(const str s, const str suffix)
{
	const size_t n = str_len(suffix);

	return (n == 0)
		|| (str_len(s) >= n && memcmp(str_end(s) - n, str_ptr(suffix), n) == 0);
}

// string constructors -----------------------------------------------------------------
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
str str_acquire_chars(const char* const s, size_t n)
{
	// take ownership even if the string is empty, because its memory is still allocated
	return s ? ((str){ s, _owner_info(n) }) : str_null;
}

// take ownership of the given C string
str str_acquire(const char* const s)
{
	// take ownership even if the string is empty, because its memory is still allocated
	return s ? ((str){ s, _owner_info(strlen(s)) }) : str_null;
}

// allocate a copy of the given string
void _str_dup(str* const dest, const str s)
{
	const size_t n = str_len(s);

	if(n == 0)
		str_clear(dest);
	else
	{
		char* const p = memcpy(str_mem_alloc(n + 1), str_ptr(s), n);

		p[n] = 0;
		str_assign(dest, str_acquire_chars(p, n));
	}
}

// string composition -----------------------------------------------------------------------
// append string
static inline
char* append_str(char* p, const str s)
{
	return mempcpy(p, str_ptr(s), str_len(s));
}

static
size_t total_length(const str* src, size_t count)
{
	size_t sum = 0;

	for(; count > 0; --count)
		sum += str_len(*src++);

	return sum;
}

// concatenate strings
void _str_cat_range(str* const dest, const str* src, size_t count)
{
	if(!src)
	{
		str_clear(dest);
		return;
	}

	// calculate total length
	const size_t num = total_length(src, count);

	if(num == 0)
	{
		str_clear(dest);
		return;
	}

	// allocate
	char* const buff = str_mem_alloc(num + 1);

	// copy bytes
	char* p = buff;

	for(; count > 0; --count)
		p = append_str(p, *src++);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire_chars(buff, num));
}

// writing to file descriptor
int _str_cpy_to_fd(const int fd, const str s)
{
	const size_t n = str_len(s);

	return (n > 0 && write(fd, str_ptr(s), n) < 0) ? errno : 0;
}

// writing to byte stream
int _str_cpy_to_stream(FILE* const stream, const str s)
{
	const size_t n = str_len(s);

	return (n > 0 && fwrite(str_ptr(s), 1, n, stream) < n) ? EIO : 0;
}

static
struct iovec* vec_append(struct iovec* pv, const str s)
{
	*pv = (struct iovec){ (void*)str_ptr(s), str_len(s) };

	return pv + 1;
}

static
struct iovec* vec_append_nonempty(struct iovec* pv, const str s)
{
	return str_is_empty(s) ? pv : vec_append(pv, s);
}

#define IOVEC_SIZE 1024

int _str_cat_range_to_fd(const int fd, const str* src, size_t count)
{
	if(!src)
		return 0;

	struct iovec v[IOVEC_SIZE];

	while(count > 0)
	{
		struct iovec* p = vec_append_nonempty(v, *src++);

		while(--count > 0 && p < v + IOVEC_SIZE)
			p = vec_append_nonempty(p, *src++);

		const size_t n = p - v;

		if(n == 0)
			break;

		if(writev(fd, v, n) < 0)
			return errno;
	}

	return 0;
}

int _str_cat_range_to_stream(FILE* const stream, const str* src, size_t count)
{
	if(!src)
		return 0;

	int err = 0;

	for(; count > 0 && err == 0; --count)
		err = str_cpy(stream, *src++);

	return err;
}

// join strings
void _str_join_range(str* const dest, const str sep, const str* src, size_t count)
{
	// test for simple cases
	if(str_is_empty(sep))
	{
		str_cat_range(dest, src, count);
		return;
	}

	if(!src || count == 0)
	{
		str_clear(dest);
		return;
	}

	if(count == 1)
	{
		str_cpy(dest, *src);
		return;
	}

	// calculate total length
	const size_t num = total_length(src, count) + str_len(sep) * (count - 1);

	// allocate
	char* const buff = str_mem_alloc(num + 1);

	// copy bytes
	char* p = append_str(buff, *src++);

	while(--count > 0)
		p = append_str(append_str(p, sep), *src++);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire_chars(buff, num));
}

int _str_join_range_to_fd(const int fd, const str sep, const str* src, size_t count)
{
	if(str_is_empty(sep))
		return str_cat_range(fd, src, count);

	if(!src || count == 0)
		return 0;

	if(count == 1)
		return str_cpy(fd, *src);

	struct iovec v[IOVEC_SIZE];

	struct iovec* p = vec_append_nonempty(v, *src++);

	for(--count; count > 0; p = v)
	{
		p = vec_append_nonempty(vec_append(p, sep), *src++);

		while(--count > 0 && p < v + IOVEC_SIZE - 1)
			p = vec_append_nonempty(vec_append(p, sep), *src++);

		const size_t n = p - v;

		if(n == 0)
			break;

		if(writev(fd, v, n) < 0)
			return errno;
	}

	return 0;
}

int _str_join_range_to_stream(FILE* const stream, const str sep, const str* src, size_t count)
{
	if(str_is_empty(sep))
		return str_cat_range(stream, src, count);

	if(!src || count == 0)
		return 0;

	int err = str_cpy(stream, *src++);

	while(--count > 0 && err == 0)
		err = str_cat(stream, sep, *src++);

	return err;
}

// sorting: comparison functions
int str_order_asc(const void* const s1, const void* const s2)
{
	return str_cmp(*(const str*)s1, *(const str*)s2);
}

int str_order_desc(const void* const s1, const void* const s2)
{
	return -str_cmp(*(const str*)s1, *(const str*)s2);
}

int str_order_asc_ci(const void* const s1, const void* const s2)
{
	return str_cmp_ci(*(const str*)s1, *(const str*)s2);
}

int str_order_desc_ci(const void* const s1, const void* const s2)
{
	return -str_cmp_ci(*(const str*)s1, *(const str*)s2);
}

// sorting
void str_sort_range(const str_cmp_func cmp, str* const array, const size_t count)
{
	if(array && cmp && count > 1)
		qsort(array, count, sizeof(array[0]), cmp);
}

// searching
const str* str_search_range(const str key, const str* const array, const size_t count)
{
	if(!array || count == 0)
		return NULL;

	if(count == 1)
		return str_eq(key, array[0]) ? array : NULL;

	return bsearch(&key, array, count, sizeof(str), str_order_asc);
}

// partitioning
size_t str_partition_range(bool (*pred)(const str), str* const array, const size_t count)
{
	if(!array)
		return 0;

	const str* const end = array + count;
	str* p = array;

	while(p < end && pred(*p))
		++p;

	for(str* s = p + 1; s < end; ++s)
		if(pred(*s))
			str_swap(p++, s);

	return p - array;
}