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

#include "str_impl.h"

// size check
_Static_assert(sizeof(str_decode_result) == 4, "str_decode_result must be 32 bits");

// result constructors
static inline
str_decode_result result_ok(uint32_t cp, uint32_t bytes) {
	return (str_decode_result){
		.status = STR_UTF8_OK,
		.num_bytes = bytes,
		.utf8_len = bytes,
		.codepoint = cp
	};
}

static inline
str_decode_result result_error(uint32_t advance, uint32_t seq_len) {
	return (str_decode_result){
		.status = STR_UTF8_ERROR,
		.num_bytes = advance,
		.utf8_len = seq_len
	};
}

static inline
str_decode_result result_incomplete(uint32_t available, uint32_t seq_len) {
	return (str_decode_result){
		.status = STR_UTF8_INCOMPLETE,
		.num_bytes = available,
		.utf8_len = seq_len
	};
}

// utf-8 sequence info table
typedef struct {
	uint8_t len;       // sequence length (2-4, 0=invalid)
	uint8_t mask;      // bitmask for first byte
	uint8_t min_cont2; // minimum valid second byte
	uint8_t max_cont2; // maximum valid second byte
} utf8_seq_info;

// lookup table for bytes 0x80-0xFF (index = byte - 0x80)
static
const utf8_seq_info utf8_info[128] = {
	// 0x80-0xBF: continuation bytes (invalid as start)
	[0x00 ... 0x3F] = {0, 0, 0, 0},

	// 0xC0-0xC1: overlong 2-byte sequences (invalid)
	[0x40] = {0, 0, 0, 0},  // 0xC0
	[0x41] = {0, 0, 0, 0},  // 0xC1

	// 0xC2-0xDF: valid 2-byte sequences (U+0080-U+07FF)
	[0x42 ... 0x5F] = {2, 0x1F, 0x80, 0xBF},

	// 0xE0: 3-byte with restricted range (U+0800-U+0FFF)
	[0x60] = {3, 0x0F, 0xA0, 0xBF},

	// 0xE1-0xEC: normal 3-byte (U+1000-U+CFFF)
	[0x61 ... 0x6C] = {3, 0x0F, 0x80, 0xBF},

	// 0xED: 3-byte excluding surrogates (U+D000-U+D7FF)
	[0x6D] = {3, 0x0F, 0x80, 0x9F},

	// 0xEE-0xEF: normal 3-byte (U+E000-U+FFFF)
	[0x6E ... 0x6F] = {3, 0x0F, 0x80, 0xBF},

	// 0xF0: 4-byte with restricted range (U+10000-U+3FFFF)
	[0x70] = {4, 0x07, 0x90, 0xBF},

	// 0xF1-0xF3: normal 4-byte (U+40000-U+FFFFF)
	[0x71 ... 0x73] = {4, 0x07, 0x80, 0xBF},

	// 0xF4: 4-byte limited to U+100000-U+10FFFF
	[0x74] = {4, 0x07, 0x80, 0x8F},

	// 0xF5-0xFF: invalid (> U+10FFFF)
	[0x75 ... 0x7F] = {0, 0, 0, 0},
};

// utf-8 decoder
str_decode_result str_decode_utf8_impl(const uint8_t* s, size_t n) {
	const uint8_t b0 = s[0];
	const utf8_seq_info* const info = &utf8_info[b0 - 0x80];
	const uint32_t seq_len = info->len;

	if(seq_len == 0)
		return result_error(1, 1);

	if(n < seq_len)
		return result_incomplete(n, seq_len);

	const uint8_t b1 = s[1];

	if((b1 & 0xC0) != 0x80 || b1 < info->min_cont2 || b1 > info->max_cont2)
		return result_error(2, seq_len);

	uint32_t cp = ((b0 & info->mask) << 6) | (b1 & 0x3F);

	for(uint32_t i = 2; i < seq_len; ++i) {
		const uint8_t b = s[i];

		if((b & 0xC0) != 0x80)
			return result_error(i + 1, seq_len);

		cp = (cp << 6) | (b & 0x3F);
	}

	return result_ok(cp, seq_len);
}
