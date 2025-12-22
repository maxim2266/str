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
#include "str_impl.h"

#define Lit str_lit

typedef struct {
	str src, res;
} test_case;

// U+FFFD
#define REPL "\xEF\xBF\xBD"

static
const test_case tests[] = {
	// valid utf-8 sequences (should pass through unchanged)
	{ Lit("Hello World"), Lit("Hello World") },
	{ Lit("\xC3\xA9 caf\xC3\xA9"), Lit("\xC3\xA9 caf\xC3\xA9") },	// "é café"
	{ Lit("\xE2\x82\xAC 100"), Lit("\xE2\x82\xAC 100") },			// "€ 100"
	{ Lit("\xF0\x9F\x98\x80"), Lit("\xF0\x9F\x98\x80") },			// "😀" U+1F600

	// single invalid bytes
	{ Lit("\x80"), Lit(REPL) },  // lone continuation byte
	{ Lit("\xFF"), Lit(REPL) },  // invalid start byte
	{ Lit("\xC0"), Lit(REPL) },  // overlong 2-byte start (incomplete)
	{ Lit("\xF5"), Lit(REPL) },  // invalid 4-byte start

	// invalid start bytes in stream
	{ Lit("a\x80""b"), Lit("a" REPL "b") },	// continuation in middle (split string)
	{ Lit("\xC1\x80"), Lit(REPL REPL) },	// overlong encoding
	{ Lit("\xF5\x80\x80\x80"), Lit(REPL REPL REPL REPL) },	// invalid 4-byte start

	// truncated sequences (missing continuation bytes)
	{ Lit("\xC2"), Lit(REPL) },			// 2-byte start at end
	{ Lit("\xE0\xA0"), Lit(REPL) },		// 3-byte truncated
	{ Lit("\xF0\x90\x80"), Lit(REPL) },	// 4-byte truncated

	// invalid continuation bytes
	{ Lit("\xC2\xC0"), Lit(REPL REPL) },					// C0 not valid continuation
	{ Lit("\xE0\x80\x41"), Lit(REPL REPL "A") },			// 3-byte with ASCII as 3rd byte
	{ Lit("\xF0\x80\x80\x41"), Lit(REPL REPL REPL "A") },	// 4-byte with ASCII as 4th byte

	// maximal subpart examples
	{ Lit("\xF1\x80\x80\x41"), Lit(REPL "A") },				// F1 80 80 is subpart (3 bytes), then A
	{ Lit("\xF1\x80\x41\x80"), Lit(REPL "A" REPL) },		// F1 80 is subpart (2 bytes), then A, then lone 80
	{ Lit("\xF1\x41\x80\x80"), Lit(REPL "A" REPL REPL) },	// F1 is subpart (1 byte), then A, then two lone 80s

	// multiple errors in sequence
	{ Lit("\xC0\x80\x80\x41"), Lit(REPL REPL REPL "A") },	// each byte separate
	{ Lit("\xE0\xC0\x41\x80"), Lit(REPL REPL "A" REPL) },	// E0 subpart, C0 subpart, A valid, 80 subpart
	{ Lit("test\x80\xC0\xE0\x80""end"), Lit("test" REPL REPL REPL REPL "end") },

	// overlong encodings
	{ Lit("\xC0\xAF"), Lit(REPL REPL) },					// overlong for '/' (U+002F)
	{ Lit("\xE0\x80\xAF"), Lit(REPL REPL REPL) },			// overlong for '/'
	{ Lit("\xF0\x80\x80\xAF"), Lit(REPL REPL REPL REPL) },	// overlong for '/'

	// surrogates (invalid in UTF-8)
	{ Lit("\xED\xA0\x80"), Lit(REPL REPL REPL) },	// U+D800 (high surrogate)
	{ Lit("\xED\xBF\xBF"), Lit(REPL REPL REPL) },	// U+DFFF (low surrogate)

	// out of range (beyond U+10FFFF)
	{ Lit("\xF4\x90\x80\x80"), Lit(REPL REPL REPL REPL) },	// U+110000
	{ Lit("\xF5\x80\x80\x80"), Lit(REPL REPL REPL REPL) },	// U+140000

	// mixed valid and invalid
	{ Lit("Hello\x80""World\xC2\x41"), Lit("Hello" REPL "World" REPL "A") },
	{ Lit("\xC3\xA9\x80\xE2\x82\xAC"), Lit("\xC3\xA9" REPL "\xE2\x82\xAC") },	// é, invalid, €
	{ Lit("\x41\xC2\x80\xC3\xBF"), Lit("\x41\xC2\x80\xC3\xBF") },				// all valid: A, C2 80, C3 BF

	// all continuation bytes
	{ Lit("\x80\x80\x80\x80"), Lit(REPL REPL REPL REPL) },			// each byte separate subpart
	{ Lit("\x80\x80\x41\x80\x80"), Lit(REPL REPL "A" REPL REPL) },	// continuations, A, continuations

	// valid after invalid (resynchronization test)
	{ Lit("\xFF\x41"), Lit(REPL "A") },						// invalid start, then valid ASCII
	{ Lit("\xE0\x80\x41\x42\x43"), Lit(REPL REPL "ABC") },	// invalid, then ABC
	{ Lit("\xC0\xF4\x80\x80\x41"), Lit(REPL REPL "A") },	// two invalid starts, then A

	// null bytes and control characters
	{ Lit("\x00\x80\x00"), Lit("\x00" REPL "\x00") },	// null, invalid, null
	{ Lit("test\x00\x80""test"), Lit("test\x00" REPL "test") },

	// mixed example
	{
		Lit("Valid: \xC3\xA9, Invalid: \x80\xC0, Truncated: \xE0\xA0"),
		Lit("Valid: \xC3\xA9, Invalid: " REPL REPL ", Truncated: " REPL)
	},

	// 4-byte sequence with late error
	{ Lit("\xF0\x90\x90\xC0\x41\x42"), Lit(REPL REPL "AB") },

	// maximal subpart length
	{ Lit("\xF1\x80\x80\x80\x41"), Lit("\xF1\x80\x80\x80\x41") },	// valid: U+100000 followed by 'A'
	{ Lit("\xF1\x80\x80\x80\x80\x41"), Lit("\xF1\x80\x80\x80" REPL "A") },

	// boundary cases
	{ Lit("\x7F\x80"), Lit("\x7F" REPL) },			// last ASCII, then continuation
	{ Lit("\xC2\x7F"), Lit(REPL "\x7F") },			// invalid 2-byte (7F not continuation), then valid ASCII
	{ Lit("\xE0\x7F\x80"), Lit(REPL "\x7F" REPL) },	// E0 subpart, 7F valid, 80 subpart

	// random bytes (simulating corruption)
	{ Lit("\xFE\xFE\xFF\xFF"), Lit(REPL REPL REPL REPL) },	// common invalid bytes

	// empty and minimal
	{ STR_NULL, STR_NULL },
	{ Lit("\x41"), Lit("\x41") },	// just 'A'

	// longer strings with multiple patterns
	{
		Lit("Start\xC0\x80\xE0\x80\x41" "Middle\xF1\x80\x80" "End"),
		Lit("Start" REPL REPL REPL REPL "AMiddle" REPL "End")
	},

	// valid UTF-8 with single byte errors between
	{
		Lit("Text: \xC3\xA9\x80\xE2\x82\xAC\xC0\xF0\x9F\x98\x80"),
		Lit("Text: \xC3\xA9" REPL "\xE2\x82\xAC" REPL "\xF0\x9F\x98\x80")
	},

	// chain of different error types
	{
		Lit("\x80\xC0\xE0\x80\xF0\x80\x80" "OK"),
		Lit(REPL REPL REPL REPL REPL REPL REPL "OK")
	},

	// overlong sequences that look valid initially
	{
		Lit("Test\xC0\x80\xE0\x80\x80\xF0\x80\x80\x80" "End"),
		Lit("Test" REPL REPL REPL REPL REPL REPL REPL REPL REPL "End")
	},

	// real-world looking path with errors
	{
		Lit("/home/\xC0" "user/\xE0\x80" "file\x80" ".txt"),
		Lit("/home/" REPL "user/" REPL REPL "file" REPL ".txt")
	}
};

static
const char* str_to_hex(const str src) {
	static char buff[1000];

	const char* s = str_ptr(src);
	const char* const end = str_end(src);

	if(s == end)
		return "<empty>";

	char* p = buff + sprintf(buff, "%02X", (unsigned char)*s);

	while(++s < end)
		p += sprintf(p, " %02X", (unsigned char)*s);

	return buff;
}

TEST_CASE(test_utf8_validator) {
	const test_case* const end = tests + sizeof(tests)/sizeof(tests[0]);
	str_auto s = STR_NULL;

	for(const test_case* p = tests; p < end; ++p) {
		str_assign(&s, p->src);
		str_to_valid_utf8(&s);

		if(!str_eq(s, p->res)) {
			printf("\tsrc: %s\n", str_to_hex(p->src));
			printf("\texp: %s\n", str_to_hex(p->res));
			printf("\tgot: %s\n", str_to_hex(s));
			TESTF(false, "[%zu] failed", p - tests);
		}
	}
}

TEST_CASE(test_utf8_real_text) {
	str_auto s = STR_NULL;
	const int err = str_read_all_file(&s, "test-data/unicode-test.txt");

	TESTF(err == 0, "str_read_all_file: %s", strerror(err));
	TEST(str_to_valid_utf8(&s) == 0);
}
