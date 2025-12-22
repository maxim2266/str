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

#include "mite/mite.h"
#include "../str.h"

// status string
static
const char* status_str(uint32_t status) {
	switch (status) {
		case STR_UTF8_OK: return "OK";
		case STR_UTF8_ERROR: return "ERROR";
		case STR_UTF8_INCOMPLETE: return "INCOMPLETE";
		default: return "UNKNOWN";
	}
}

// basic functionality
TEST_CASE(test_utf8_basic_functionality) {
	// null/empty input
	str_decode_result res = str_decode_utf8(NULL, 5);

	TESTF(res.status == STR_UTF8_OK,
		  "NULL pointer: expected OK, got %s", status_str(res.status));
	TESTF(res.num_bytes == 0,
		  "NULL pointer: expected num_bytes=0, got %u", res.num_bytes);

	res = str_decode_utf8("test", 0);

	TESTF(res.status == STR_UTF8_OK,
		  "len=0: expected OK, got %s", status_str(res.status));

	// ASCII (single byte)
	res = str_decode_utf8("A", 1);

	TESTF(res.status == STR_UTF8_OK,
		  "ASCII 'A': expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 'A',
		  "ASCII 'A': expected codepoint 'A' (0x41), got 0x%X", res.codepoint);
	TESTF(res.num_bytes == 1,
		  "ASCII 'A': expected num_bytes=1, got %u", res.num_bytes);
	TESTF(res.utf8_len == 1,
		  "ASCII 'A': expected utf8_len=1, got %u", res.utf8_len);
}

// valid utf-8 sequences
TEST_CASE(test_utf8_valid_sequences) {
	// 2-byte sequences
	str_decode_result res = str_decode_utf8("\xC2\xA2", 2);  // ¢

	TESTF(res.status == STR_UTF8_OK,
		  "U+00A2: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x00A2,
		  "U+00A2: expected codepoint 0xA2, got 0x%X", res.codepoint);
	TESTF(res.num_bytes == 2,
		  "U+00A2: expected num_bytes=2, got %u", res.num_bytes);
	TESTF(res.utf8_len == 2,
		  "U+00A2: expected utf8_len=2, got %u", res.utf8_len);

	res = str_decode_utf8("\xDF\xBF", 2);  // U+07FF

	TESTF(res.status == STR_UTF8_OK,
		  "U+07FF: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x07FF,
		  "U+07FF: expected codepoint 0x7FF, got 0x%X", res.codepoint);

	// 3-byte sequences
	res = str_decode_utf8("\xE0\xA0\x80", 3);  // U+0800

	TESTF(res.status == STR_UTF8_OK,
		  "U+0800: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x0800,
		  "U+0800: expected codepoint 0x800, got 0x%X", res.codepoint);
	TESTF(res.num_bytes == 3,
		  "U+0800: expected num_bytes=3, got %u", res.num_bytes);

	res = str_decode_utf8("\xE2\x82\xAC", 3);  // €

	TESTF(res.status == STR_UTF8_OK,
		  "U+20AC: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x20AC,
		  "U+20AC: expected codepoint 0x20AC, got 0x%X", res.codepoint);

	res = str_decode_utf8("\xEF\xBF\xBF", 3);  // U+FFFF

	TESTF(res.status == STR_UTF8_OK,
		  "U+FFFF: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0xFFFF,
		  "U+FFFF: expected codepoint 0xFFFF, got 0x%X", res.codepoint);

	// 4-byte sequences
	res = str_decode_utf8("\xF0\x90\x80\x80", 4);  // U+10000

	TESTF(res.status == STR_UTF8_OK,
		  "U+10000: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x10000,
		  "U+10000: expected codepoint 0x10000, got 0x%X", res.codepoint);
	TESTF(res.num_bytes == 4,
		  "U+10000: expected num_bytes=4, got %u", res.num_bytes);
	TESTF(res.utf8_len == 4,
		  "U+10000: expected utf8_len=4, got %u", res.utf8_len);

	res = str_decode_utf8("\xF0\x9F\x98\x80", 4);  // 😀

	TESTF(res.status == STR_UTF8_OK,
		  "U+1F600: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x1F600,
		  "U+1F600: expected codepoint 0x1F600, got 0x%X", res.codepoint);

	res = str_decode_utf8("\xF4\x8F\xBF\xBF", 4);  // U+10FFFF

	TESTF(res.status == STR_UTF8_OK,
		  "U+10FFFF: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0x10FFFF,
		  "U+10FFFF: expected codepoint 0x10FFFF, got 0x%X", res.codepoint);
}

// invalid start bytes
TEST_CASE(test_utf8_invalid_start_bytes) {
	// continuation bytes as start (0x80-0xBF)
	str_decode_result res = str_decode_utf8("\x80", 1);

	TESTF(res.status == STR_UTF8_ERROR,
		  "0x80: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 1,
		  "0x80: expected num_bytes=1, got %u", res.num_bytes);
	TESTF(res.utf8_len == 1,
		  "0x80: expected utf8_len=1, got %u", res.utf8_len);

	res = str_decode_utf8("\xBF", 1);

	TESTF(res.status == STR_UTF8_ERROR,
		  "0xBF: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 1,
		  "0xBF: expected num_bytes=1, got %u", res.num_bytes);

	// invalid start bytes 0xF5-0xFF
	res = str_decode_utf8("\xF5\x80\x80\x80", 4);

	TESTF(res.status == STR_UTF8_ERROR,
		  "0xF5: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 1,
		  "0xF5: expected num_bytes=1, got %u", res.num_bytes);

	res = str_decode_utf8("\xFF", 1);

	TESTF(res.status == STR_UTF8_ERROR,
		  "0xFF: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 1,
		  "0xFF: expected num_bytes=1, got %u", res.num_bytes);
}

// invalid second bytes
TEST_CASE(test_utf8_invalid_second_bytes) {
	// valid start byte but invalid continuation
	str_decode_result res = str_decode_utf8("\xC2\x00", 2);

	TESTF(res.status == STR_UTF8_ERROR,
		  "C2 00: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "C2 00: expected num_bytes=2, got %u", res.num_bytes);
	TESTF(res.utf8_len == 2,
		  "C2 00: expected utf8_len=2, got %u", res.utf8_len);

	res = str_decode_utf8("\xC2\xC0", 2);

	TESTF(res.status == STR_UTF8_ERROR,
		  "C2 C0: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "C2 C0: expected num_bytes=2, got %u", res.num_bytes);

	// invalid third byte
	res = str_decode_utf8("\xE2\x82\x00", 3);

	TESTF(res.status == STR_UTF8_ERROR,
		  "E2 82 00: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 3,
		  "E2 82 00: expected num_bytes=3, got %u", res.num_bytes);
	TESTF(res.utf8_len == 3,
		  "E2 82 00: expected utf8_len=3, got %u", res.utf8_len);

	// invalid fourth byte
	res = str_decode_utf8("\xF0\x9F\x98\x00", 4);

	TESTF(res.status == STR_UTF8_ERROR,
		  "F0 9F 98 00: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 4,
		  "F0 9F 98 00: expected num_bytes=4, got %u", res.num_bytes);
}

// overlong encodings
TEST_CASE(test_utf8_overlong_encodings) {
	// overlong for ASCII (should use 1 byte)
	str_decode_result res = str_decode_utf8("\xC0\x81", 2);

	TESTF(res.status == STR_UTF8_ERROR,
		  "C0 81: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 1,
		  "C0 81: expected num_bytes=1, got %u", res.num_bytes);

	res = str_decode_utf8("\xC0\x80", 2);

	TESTF(res.status == STR_UTF8_ERROR,
		  "C0 80: expected ERROR, got %s", status_str(res.status));

	// overlong 3-byte sequences
	res = str_decode_utf8("\xE0\x82\x80", 3);

	TESTF(res.status == STR_UTF8_ERROR,
		  "E0 82 80: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "E0 82 80: expected num_bytes=2, got %u", res.num_bytes);
	TESTF(res.utf8_len == 3,
		  "E0 82 80: expected utf8_len=3, got %u", res.utf8_len);

	res = str_decode_utf8("\xE0\x9F\xBF", 3);

	TESTF(res.status == STR_UTF8_ERROR,
		  "E0 9F BF: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "E0 9F BF: expected num_bytes=2, got %u", res.num_bytes);

	// overlong 4-byte sequences
	res = str_decode_utf8("\xF0\x80\x80\x80", 4);

	TESTF(res.status == STR_UTF8_ERROR,
		  "F0 80 80 80: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "F0 80 80 80: expected num_bytes=2, got %u", res.num_bytes);
	TESTF(res.utf8_len == 4,
		  "F0 80 80 80: expected utf8_len=4, got %u", res.utf8_len);
}

// surrogate code points
TEST_CASE(test_utf8_surrogate_codepoints) {
	// UTF-16 surrogates encoded as UTF-8 (should error)
	str_decode_result res = str_decode_utf8("\xED\xA0\x80", 3);  // U+D800

	TESTF(res.status == STR_UTF8_ERROR,
		  "ED A0 80: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "ED A0 80: expected num_bytes=2, got %u", res.num_bytes);
	TESTF(res.utf8_len == 3,
		  "ED A0 80: expected utf8_len=3, got %u", res.utf8_len);

	res = str_decode_utf8("\xED\xBF\xBF", 3);  // U+DFFF

	TESTF(res.status == STR_UTF8_ERROR,
		  "ED BF BF: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "ED BF BF: expected num_bytes=2, got %u", res.num_bytes);

	// ED with valid second byte should work
	res = str_decode_utf8("\xED\x9F\xBF", 3);  // U+D7FF

	TESTF(res.status == STR_UTF8_OK,
		  "ED 9F BF: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0xD7FF,
		  "ED 9F BF: expected codepoint 0xD7FF, got 0x%X", res.codepoint);

	res = str_decode_utf8("\xEE\x80\x80", 3);  // U+E000

	TESTF(res.status == STR_UTF8_OK,
		  "EE 80 80: expected OK, got %s", status_str(res.status));
	TESTF(res.codepoint == 0xE000,
		  "EE 80 80: expected codepoint 0xE000, got 0x%X", res.codepoint);
}

// out of range (> U+10FFFF)
TEST_CASE(test_utf8_out_of_range) {
	// just above maximum
	str_decode_result res = str_decode_utf8("\xF4\x90\x80\x80", 4);  // U+110000

	TESTF(res.status == STR_UTF8_ERROR,
		  "F4 90 80 80: expected ERROR, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,
		  "F4 90 80 80: expected num_bytes=2, got %u", res.num_bytes);
	TESTF(res.utf8_len == 4,
		  "F4 90 80 80: expected utf8_len=4, got %u", res.utf8_len);
}

// incomplete sequences
TEST_CASE(test_utf8_incomplete_sequences) {
	// 2-byte sequence with only 1 byte
	str_decode_result res = str_decode_utf8("\xC2", 1);

	TESTF(res.status == STR_UTF8_INCOMPLETE,
		  "C2: expected INCOMPLETE, got %s", status_str(res.status));
	TESTF(res.num_bytes == 1,  // Changed from 0 to available bytes
		  "C2: expected num_bytes=1 (available bytes), got %u", res.num_bytes);
	TESTF(res.utf8_len == 2,
		  "C2: expected utf8_len=2, got %u", res.utf8_len);

	// 3-byte sequence with only 2 bytes
	res = str_decode_utf8("\xE2\x82", 2);

	TESTF(res.status == STR_UTF8_INCOMPLETE,
		  "E2 82: expected INCOMPLETE, got %s", status_str(res.status));
	TESTF(res.num_bytes == 2,  // Changed from 0 to available bytes
		  "E2 82: expected num_bytes=2 (available bytes), got %u", res.num_bytes);
	TESTF(res.utf8_len == 3,
		  "E2 82: expected utf8_len=3, got %u", res.utf8_len);

	// 4-byte sequence with only 3 bytes
	res = str_decode_utf8("\xF0\x9F\x98", 3);

	TESTF(res.status == STR_UTF8_INCOMPLETE,
		  "F0 9F 98: expected INCOMPLETE, got %s", status_str(res.status));
	TESTF(res.num_bytes == 3,  // Changed from 0 to available bytes
		  "F0 9F 98: expected num_bytes=3 (available bytes), got %u", res.num_bytes);
	TESTF(res.utf8_len == 4,
		  "F0 9F 98: expected utf8_len=4, got %u", res.utf8_len);
}

// iteration
TEST_CASE(test_utf8_iteration) {
	const char* text = "Hello 世界 🌍";
	size_t total_len = strlen(text);
	size_t pos = 0;
	int valid_count = 0;
	int error_count = 0;
	int incomplete_count = 0;

	while (pos < total_len) {
		const str_decode_result res = str_decode_utf8(&text[pos], total_len - pos);

		switch(res.status) {
		case STR_UTF8_OK:
			valid_count++;
			break;

		case STR_UTF8_ERROR:
			error_count++;
			break;

		case STR_UTF8_INCOMPLETE:
			incomplete_count++;

			TESTF(res.num_bytes > 0,
					"INCOMPLETE should give num_bytes>0 to skip available bytes, got %u",
					res.num_bytes);
			break;
		}

		TESTF(res.num_bytes > 0 || pos == total_len,
			  "num_bytes should be >0 except at end, got %u at position %zu",
			  res.num_bytes, pos);

		pos += res.num_bytes;
	}

	TESTF(valid_count == 10,
		  "Expected 10 valid codepoints, got %d", valid_count);
	TESTF(error_count == 0,
		  "Expected 0 errors in valid text, got %d", error_count);
	TESTF(incomplete_count == 0,
		  "Expected 0 incomplete in complete text, got %d", incomplete_count);
	TESTF(pos == total_len,
		  "Expected consumed all %zu bytes, consumed %zu", total_len, pos);
}

// edge case: max/min values
TEST_CASE(test_utf8_edge_cases) {
	// minimum/maximum values for each length
	str_decode_result res;

	// 1-byte max
	res = str_decode_utf8("\x7F", 1);

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0x7F,
		  "U+007F: expected OK with 0x7F, got %s with 0x%X",
		  status_str(res.status), res.codepoint);

	// 2-byte min/max
	res = str_decode_utf8("\xC2\x80", 2);  // U+0080

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0x80,
		  "U+0080: expected OK with 0x80, got %s with 0x%X",
		  status_str(res.status), res.codepoint);

	res = str_decode_utf8("\xDF\xBF", 2);  // U+07FF

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0x7FF,
		  "U+07FF: expected OK with 0x7FF, got %s with 0x%X",
		  status_str(res.status), res.codepoint);

	// 3-byte min/max
	res = str_decode_utf8("\xE0\xA0\x80", 3);  // U+0800

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0x800,
		  "U+0800: expected OK with 0x800, got %s with 0x%X",
		  status_str(res.status), res.codepoint);

	res = str_decode_utf8("\xEF\xBF\xBF", 3);  // U+FFFF

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0xFFFF,
		  "U+FFFF: expected OK with 0xFFFF, got %s with 0x%X",
		  status_str(res.status), res.codepoint);

	// 4-byte min/max
	res = str_decode_utf8("\xF0\x90\x80\x80", 4);  // U+10000

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0x10000,
		  "U+10000: expected OK with 0x10000, got %s with 0x%X",
		  status_str(res.status), res.codepoint);

	res = str_decode_utf8("\xF4\x8F\xBF\xBF", 4);  // U+10FFFF

	TESTF(res.status == STR_UTF8_OK && res.codepoint == 0x10FFFF,
		  "U+10FFFF: expected OK with 0x10FFFF, got %s with 0x%X",
		  status_str(res.status), res.codepoint);
}

// mixed valid/invalid iteration
TEST_CASE(test_utf8_mixed_iteration) {
	// string with invalid UTF-8 in middle
	const char* text = "Hello\xC0\x80World";
	size_t total_len = 12;
	size_t pos = 0;
	int valid_count = 0;
	int error_count = 0;
	int incomplete_count = 0;

	while (pos < total_len) {
		const str_decode_result res = str_decode_utf8(&text[pos], total_len - pos);

		switch(res.status) {
		case STR_UTF8_OK:
			valid_count++;
			break;

		case STR_UTF8_ERROR:
			error_count++;

			TESTF(res.num_bytes > 0,
					"Error should give num_bytes>0 to skip, got %u", res.num_bytes);
			break;

		case STR_UTF8_INCOMPLETE:
			incomplete_count++;

			TESTF(res.num_bytes > 0,
					"INCOMPLETE should give num_bytes>0 to skip available bytes, got %u",
					res.num_bytes);
			break;
		}

		pos += res.num_bytes;
	}

	TESTF(valid_count == 10, "Expected 10 valid codepoints, got %d", valid_count);
	TESTF(error_count == 2, "Expected 2 errors, got %d", error_count);
	TESTF(incomplete_count == 0, "Expected 0 incomplete, got %d", incomplete_count);
	TESTF(pos == total_len, "Expected consumed all %zu bytes, consumed %zu", total_len, pos);
}

// encoder test -----------------------------------------------------------------------------------
typedef struct {
	uint32_t cp;
	size_t n;
	const char* seq;
} encoder_test;

static
const encoder_test encoder_test_cases[] = {
    // 1-byte
    { 0x0041, 1, "A" },
    { 0x007F, 1, "\x7F" },

    // 2-byte UTF-8
    { 0x0080, 2, "\xC2\x80" },
    { 0x00A9, 2, "\xC2\xA9" },
    { 0x07FF, 2, "\xDF\xBF" },

    // 3-byte UTF-8
    { 0x0800, 3, "\xE0\xA0\x80" },
    { 0x20AC, 3, "\xE2\x82\xAC" },
    { 0xD7FF, 3, "\xED\x9F\xBF" },
    { 0xE000, 3, "\xEE\x80\x80" },

    // 4-byte UTF-8
    { 0x10000,  4, "\xF0\x90\x80\x80" },
    { 0x1F600,  4, "\xF0\x9F\x98\x80" },
    { 0x10FFFF, 4, "\xF4\x8F\xBF\xBF" },

    // invalid codepoints
    { 0xD800,   0, NULL },
    { 0xDFFF,   0, NULL },
    { 0x110000, 0, NULL },
};

TEST_CASE(test_encode_codepoint) {
	const encoder_test* p = encoder_test_cases;
	const encoder_test* const end = p + sizeof(encoder_test_cases)/sizeof(encoder_test_cases[0]);

	for(; p < end; ++p) {
		char buff[10];
		const size_t n = str_encode_codepoint(buff, p->cp);

		TESTF(n == p->n,
			  "[%zu] unexpected sequence length: %zu instead of %zu",
			  end - p, n, p->n);

		if(n > 0)
			TESTF(memcmp(buff, p->seq, n) == 0, "[%zu] sequence mismatch", end - p);
	}
}
