//#define _GNU_SOURCE

#include "str.h"

#include <stdlib.h>
#include <stdio.h>

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
	if(str_is_owner(s))
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

	return (str){ p, _owner_info(n) };
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

// concatenate strings
void str_cat_range(str* const dest, const str* const src, const size_t n)
{
	// test for simple cases
	if(simple_cat(dest, src, n))
		return;

	// calculate total length
	size_t num = 0;

	for(size_t i = 0; i < n; ++i)
		num += str_len(src[i]);

	if(num == 0)
	{
		str_clear(dest);
		return;
	}

	// allocate
	char* const buff = mem_alloc(num + 1);

	// copy bytes
	char* p = buff;

	for(size_t i = 0; i < n; ++i)
		p = append_str(p, src[i]);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire_range(buff, num));
}

// join strings
void str_join_range(str* const dest, const str sep, const str* const src, const size_t n)
{
	// test for simple cases
	if(str_is_empty(sep))
	{
		str_cat_range(dest, src, n);
		return;
	}

	if(simple_cat(dest, src, n))
		return;

	// calculate total length
	size_t num = 0;

	for(size_t i = 0; i < n; ++i)
		num += str_len(src[i]);

	if(num == 0)
	{
		str_clear(dest);
		return;
	}

	num += str_len(sep) * (n - 1);

	// allocate
	char* const buff = mem_alloc(num + 1);

	// copy bytes
	char* p = append_str(buff, src[0]);

	for(size_t i = 1; i < n; ++i)
		p = append_str(append_str(p, sep), src[i]);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire_range(buff, num));
}