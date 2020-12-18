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
#define _XOPEN_SOURCE

#include "str.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// compatibility
#ifndef _GNU_SOURCE
static inline
void* mempcpy(void* dest, const void* src, const size_t n)
{
	return memcpy(dest, src, n) + n;
}
#endif

static inline
void str_mem_free(void* p)
{
	if(p)
		free(p);
}

// string deallocation
void str_free(const str s)
{
	if(str_is_owner(s))
		str_mem_free((void*)s.ptr);
}

// memory allocation helpers
#define ALLOC(n)	\
({	\
	void* const ___p = malloc(n);	\
	if(!___p) return ENOMEM;	\
	___p;	\
})

#define REALLOC(p, n)	\
({	\
	void* const ___s = (p);	\
	void* const ___p = realloc(___s, (n));	\
	if(!___p) { str_mem_free(___s); return ENOMEM; }	\
	___p;	\
})

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
str str_acquire_chars(const char* const s, const size_t n)
{
	if(!s)
		return str_null;

	if(n == 0)
	{
		free((void*)s);
		return str_null;
	}

	return (str){ s, _owner_info(n) };
}

// take ownership of the given C string
str str_acquire(const char* const s)
{
	return s ? str_acquire_chars(s, strlen(s)) : str_null;
}

// allocate a copy of the given string
int _str_dup(str* const dest, const str s)
{
	const size_t n = str_len(s);

	if(n == 0)
		str_clear(dest);
	else
	{
		char* const p = memcpy(ALLOC(n + 1), str_ptr(s), n);

		p[n] = 0;
		str_assign(dest, str_acquire_chars(p, n));
	}

	return 0;
}

#ifndef STR_MAX_FILE_SIZE
#define STR_MAX_FILE_SIZE	(64 * 1024 * 1024 - 1)
#endif

static
int get_file_size(const int fd, off_t* const size)
{
	// stat the file
	struct stat info;

	if(fstat(fd, &info) == -1)
		return errno;

	*size = info.st_size;

	// only regular files are allowed
	switch(info.st_mode & S_IFMT)
	{
		case S_IFREG:
			return (info.st_size > STR_MAX_FILE_SIZE) ? EFBIG : 0;
		case S_IFDIR:
			return EISDIR;
		default:
			return EOPNOTSUPP;
	}
}

static
int read_from_fd(const int fd, void* p, off_t* const psize)
{
	const void* const end = p + *psize;
	ssize_t n;

	if((n = read(fd, p, end - p)) < 0)
		return errno;

	for(p += n; n > 0 && p < end; p += n)
		if((n = read(fd, p, end - p)) < 0)
			return errno;

	*psize -= end - p;
	return 0;
}

static
int str_from_fd(const int fd, const off_t size, str* const dest)
{
	if(size == 0)
	{
		str_clear(dest);
		return 0;
	}

	char* buff = ALLOC(size + 1);
	off_t n = size;
	const int err = read_from_fd(fd, buff, &n);

	if(err != 0)
	{
		free(buff);
		return err;
	}

	if(n == 0)
	{
		free(buff);
		str_clear(dest);
		return 0;
	}

	if(n < size)
		buff = REALLOC(buff, n + 1);

	buff[n] = 0;
	str_assign(dest, str_acquire_chars(buff, n));
	return 0;
}

int str_from_file(str* const dest, const char* const file_name)
{
	const int fd = open(file_name, O_CLOEXEC | O_RDONLY);

	if(fd < 0)
		return errno;

	off_t size = 0;
	int err = get_file_size(fd, &size);

	if(err == 0)
		err = str_from_fd(fd, size, dest);

	close(fd);
	return err;
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
int _str_cat_range(str* const dest, const str* src, size_t count)
{
	if(!src)
	{
		str_clear(dest);
		return 0;
	}

	// calculate total length
	const size_t num = total_length(src, count);

	if(num == 0)
	{
		str_clear(dest);
		return 0;
	}

	// allocate
	char* const buff = ALLOC(num + 1);

	// copy bytes
	char* p = buff;

	for(; count > 0; --count)
		p = append_str(p, *src++);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire_chars(buff, num));
	return 0;
}

// writing to file descriptor
int _str_cpy_to_fd(const int fd, const str s)
{
	size_t n = str_len(s);
	const void* p = str_ptr(s);

	while(n > 0)
	{
		const ssize_t m = write(fd, p, n);

		if(m < 0)
			return errno;

		n -= m;
		p += m;
	}

	return 0;
}

// writing to byte stream
int _str_cpy_to_stream(FILE* const stream, const str s)
{
	const size_t n = str_len(s);

	return (n > 0 && fwrite(str_ptr(s), 1, n, stream) < n) ? EIO : 0;
}

// write iovec
static
int write_iovec(const int fd, struct iovec* pv, unsigned nv)
{
	while(nv > 0)
	{
		ssize_t n = writev(fd, pv, nv);

		if(n < 0)
			return errno;

		// discard items already written
		for(; nv > 0; ++pv, --nv)
		{
			if(n < (ssize_t)pv->iov_len)
			{
				pv->iov_base += n;
				pv->iov_len -= n;
				break;
			}

			n -= (ssize_t)pv->iov_len;
		}
	}

	return 0;
}

// concatenate to file descriptor
static
struct iovec* vec_append(struct iovec* const pv, const str s)
{
	*pv = (struct iovec){ (void*)str_ptr(s), str_len(s) };

	return pv + 1;
}

static
struct iovec* vec_append_nonempty(struct iovec* const pv, const str s)
{
	return str_is_empty(s) ? pv : vec_append(pv, s);
}

int _str_cat_range_to_fd(const int fd, const str* src, size_t count)
{
	if(!src)
		return 0;

	struct iovec v[IOV_MAX];

	while(count > 0)
	{
		struct iovec* p = vec_append_nonempty(v, *src++);

		while(--count > 0 && p < v + IOV_MAX)
			p = vec_append_nonempty(p, *src++);

		const size_t n = p - v;

		if(n == 0)
			break;

		const int ret = write_iovec(fd, v, n);

		if(ret != 0)
			return ret;
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
int _str_join_range(str* const dest, const str sep, const str* src, size_t count)
{
	// test for simple cases
	if(str_is_empty(sep))
		return str_cat_range(dest, src, count);

	if(!src || count == 0)
	{
		str_clear(dest);
		return 0;
	}

	if(count == 1)
		return str_cpy(dest, *src);

	// calculate total length
	const size_t num = total_length(src, count) + str_len(sep) * (count - 1);

	// allocate
	char* const buff = ALLOC(num + 1);

	// copy bytes
	char* p = append_str(buff, *src++);

	while(--count > 0)
		p = append_str(append_str(p, sep), *src++);

	// null-terminate and assign
	*p = 0;
	str_assign(dest, str_acquire_chars(buff, num));
	return 0;
}

int _str_join_range_to_fd(const int fd, const str sep, const str* src, size_t count)
{
	if(str_is_empty(sep))
		return str_cat_range(fd, src, count);

	if(!src || count == 0)
		return 0;

	if(count == 1)
		return str_cpy(fd, *src);

	struct iovec v[IOV_MAX];

	struct iovec* p = vec_append_nonempty(v, *src++);

	for(--count; count > 0; p = v)
	{
		p = vec_append_nonempty(vec_append(p, sep), *src++);

		while(--count > 0 && p < v + IOV_MAX - 1)
			p = vec_append_nonempty(vec_append(p, sep), *src++);

		const size_t n = p - v;

		if(n == 0)
			break;

		const int ret = write_iovec(fd, v, n);

		if(ret != 0)
			return ret;
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
	if(array && count > 1)
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

// unique partitioning
size_t str_unique_range(str* const array, const size_t count)
{
	if(!array || count == 0)
		return 0;

	if(count == 1)
		return 1;

	str_sort_range(str_order_asc, array, count);

	const str* const end = array + count;
	str* p = array;

	for(str* s = array + 1; s < end; ++s)
		if(!str_eq(*p, *s) && (++p < s))
			str_swap(p, s);

	return p + 1 - array;
}

// string iterator function
#ifdef __STDC_UTF_32__

char32_t _cp_iterator_next(_cp_iterator* const it)
{
	if(it->curr >= it->end)
		return CPI_END_OF_STRING;

	char32_t c;
	const size_t n = mbrtoc32(&c, it->curr, it->end - it->curr, &it->state);

	switch(n)	// see https://en.cppreference.com/w/c/string/multibyte/mbrtoc32
	{
		case 0:				// null character (U+0000) is allowed
			++it->curr;
			return 0;
		case (size_t)-1:	// encoding error
		case (size_t)-3:	// surrogate pair detected
			return CPI_ERR_INVALID_ENCODING;
		case (size_t)-2:	// incomplete sequence
			return CPI_ERR_INCOMPLETE_SEQ;
		default:			// ok
			it->curr += n;
			return c;
	}
}

#endif	// ifdef __STDC_UTF_32__
