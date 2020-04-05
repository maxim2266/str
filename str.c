//#define _GNU_SOURCE

#include "str.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// memory allocation wrappers
static __attribute__((malloc))
void* mem_alloc(const size_t n)
{
	void* const p = malloc(n);

	if(!p)
	{
		perror("fatal error");
		exit(EXIT_FAILURE);
	}

	return p;
}

static inline
void mem_free(void* p)
{
	if(p)
		free(p);
}

// string deallocation
void str_free(const str s)
{
	if(str_is_alloc(s))
		mem_free((void*)s.ptr);
}

// compare two strings lexicographically
int str_cmp(const str s1, const str s2)
{
	const size_t
		n1 = str_len(s1),
		n2 = str_len(s2);

	// either string may be missing a null terminator, hence "memcmp"
	const int res = memcmp(str_ptr(s1), str_ptr(s2), (n1 < n2) ? n1 : n2);

	if(res != 0 || n1 == n2)
		return res;

	return (n1 < n2) ? -1 : 1;
}

// allocate a copy of the given string
str str_dup(const str s)
{
	const size_t n = str_len(s);

	if(n == 0)
		return str_null;

	char* const p = memcpy(mem_alloc(n + 1), str_ptr(s), n);

	p[n] = 0;

	return (str){ p, (n << 1) | 1 };
}

// string reference from pointer
str _str_ref_form_ptr(const char* const s)
{
	const size_t n = s ? strlen(s) : 0;

	return (n > 0) ? ((str){ s, n << 1 }) : str_null;
}

// take ownership of the given string s of n chars
str str_acquire(const char* const s, size_t n)
{
	if(!s)
		return str_null;

	if(n == (size_t)-1)	// special value to trigger the string length calculation
		n = strlen(s);

	// take ownership even if the string is empty, as its memory is still allocated.
	return (str){ s, (n << 1) | 1 };
}

// compatibility
#ifndef _GNU_SOURCE
static inline
void* mempcpy(void* dest, const void* src, const size_t n)
{
	return memcpy(dest, src, n) + n;
}
#endif

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
		str_assign(dest, str_dup(src[0]));
		return true;
	}

	return false;
}

static
size_t sum_str_len(const str* const src, const size_t n)
{
	size_t num = 0;

	for(size_t i = 0; i < n; ++i)
		num += str_len(src[i]);

	return num;
}

// concatenate strings
void str_cat(str* const dest, const str* const src, const size_t n)
{
	// test for simple cases
	if(simple_cat(dest, src, n))
		return;

	// calculate total length
	const size_t num = sum_str_len(src, n);

	// allocate
	char* const buff = mem_alloc(num + 1);

	// copy bytes
	char* p = buff;

	for(size_t i = 0; i < n; ++i)
		p = append_str(p, src[i]);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire(buff, num));
}

// join strings
void str_join(str* const dest, const str sep, const str* const src, const size_t n)
{
	// test for simple cases
	if(str_is_empty(sep))
	{
		str_cat(dest, src, n);
		return;
	}

	if(simple_cat(dest, src, n))
		return;

	// calculate total length
	const size_t num = sum_str_len(src, n) + str_len(sep) * (n - 1);

	// allocate
	char* const buff = mem_alloc(num + 1);

	// copy bytes
	char* p = append_str(buff, src[0]);

	for(size_t i = 1; i < n; ++i)
		p = append_str(append_str(p, sep), src[i]);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire(buff, num));
}