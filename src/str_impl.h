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

#include "../str.h"

// terminator
void mem_failure(void) __attribute__((noinline, noreturn));

// memory allocator
static inline
void* mem_alloc(const size_t n) {
	void* const p = malloc(n);

	if(p)
		return p;

	mem_failure();
}

static inline
void* mem_realloc(void* const p, const size_t n) {
	void* const pp = realloc(p, n);

	if(pp || n == 0)
		return pp;

	free(p);
	mem_failure();
}

// allocate memory and copy string with null terminator appended
static inline
const char* mem_alloc_copy(const char* const s, const size_t n) {
	char* const p = memcpy(mem_alloc(n + 1), s, n);

	p[n] = 0;
	return p;
}

// append to destination and return the end pointer
static inline
void* mem_append(void* const dest, const void* const src, const size_t n) {
	return memcpy(dest, src, n) + n;
}

// append string
static inline
char* append_str(char* const p, const str s) {
	return mem_append(p, str_ptr(s), str_len(s));
}

// calculate total length of strings
static inline
size_t calc_total_length(const str* src, const size_t count) {
    const str* const end = src + count;
	size_t sum = 0;

    while(src < end)
      sum += str_len(*src++);

	return sum;
}

// set matcher functions
#define BITSET_BUFF_SIZE (256 / sizeof(uint8_t))

static inline
void bitset_init(uint8_t* buff, const str charset) {
	memset(buff, 0, BITSET_BUFF_SIZE);

	const char* const end = str_end(charset);

	for(const char* s = str_ptr(charset); s < end; ++s)
		buff[(uint8_t)*s / 8] |= (1 << ((uint8_t)*s % 8));
}

static inline
uint8_t bitset_match(const uint8_t* const buff, const char c) {
	return buff[(uint8_t)c / 8] & (1 << ((uint8_t)c % 8));
}

static inline
const char* bitset_search(const uint8_t* const bitset, const char* p, const char* const end) {
	while(p < end && !bitset_match(bitset, *p))
		++p;

	return p;
}

static inline
const char* bitset_span(const uint8_t* const bitset, const char* p, const char* const end) {
	while(p < end && bitset_match(bitset, *p))
		++p;

	return p;
}

// string builder
#define SB_SIZE	64

typedef struct {
	size_t count;
	str buff[SB_SIZE];
} str_builder;

static inline
void sb_init(str_builder* const self) {
	self->count = 0;
	self->buff[0] = str_null;
}

static inline
void sb_inc_index(str_builder* const self) {
	if(++self->count == SB_SIZE) {
		str_concat_array(self->buff, self->buff, SB_SIZE);
		self->count = 1;
	}
}

static inline
void sb_append_mem(str_builder* const self, const char* const s, const size_t n) {
	if(n > 0) {
		self->buff[self->count] = str_ref_mem(s, n);
		sb_inc_index(self);
	}
}

static inline
void sb_append(str_builder* const self, const str s) {
	if(!str_is_empty(s)) {
		self->buff[self->count] = str_ref(s);
		sb_inc_index(self);
	}
}

static inline
void assign_sb(str* const dest, str_builder* const self) {
	str_concat_array(self->buff, self->buff, self->count);
	str_assign(dest, self->buff[0]);
}
