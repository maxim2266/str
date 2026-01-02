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

#define Lit str_lit

TEST_CASE(test_lit) {
	const str s = Lit("ZZZ");

	TEST(str_len(s) == 3);
	TEST(str_is_ref(s));
	TEST(!str_is_owner(s));
	TEST(str_eq(s, Lit("ZZZ")));
}

TEST_CASE(test_clear) {
	str s = Lit("ZZZ");

	str_clear(&s);

	TEST(str_is_empty(s));
	TEST(str_is_ref(s));
	TEST(str_eq(s, str_null));
	TEST(strlen(str_ptr(s)) == str_len(s));
}

TEST_CASE(test_ref) {
	str s1 = str_null;

	str_clone(&s1, Lit("ZZZ"));

	str s2 = str_ref(s1);

	TEST(str_is_owner(s1));
	TEST(str_eq(s1, Lit("ZZZ")));
	TEST(str_is_ref(s2));
	TEST(str_eq(s2, Lit("ZZZ")));

	str_free(s1);
}

TEST_CASE(test_ref_slice) {
	str s = Lit("abcd");

	TEST(str_eq(str_ref_slice(s, 0, 0), str_null));
	TEST(str_eq(str_ref_slice(s, 2, 2), str_null));
	TEST(str_eq(str_ref_slice(s, 100, 100), str_null));
	TEST(str_eq(str_ref_slice(s, __SIZE_MAX__, __SIZE_MAX__), str_null));
	TEST(str_eq(str_ref_slice(s, 2, 0), str_null));

	TEST(str_eq(str_ref_slice(s, 0, 2), Lit("ab")));
	TEST(str_eq(str_ref_slice(s, 2, 3), Lit("c")));
	TEST(str_eq(str_ref_slice(s, 2, 4), Lit("cd")));
	TEST(str_eq(str_ref_slice(s, 2, 100), Lit("cd")));
}

TEST_CASE(test_acquire) {
	str s1 = str_null;

	str_clone(&s1, Lit("ZZZ"));

	str s2 = str_acquire(&s1);

	TEST(str_is_ref(s1));
	TEST(str_eq(s1, Lit("ZZZ")));
	TEST(str_is_owner(s2));
	TEST(str_eq(s2, Lit("ZZZ")));

	str_free(s2);
}

TEST_CASE(test_clone) {
	str_auto s = str_null;

	str_clone(&s, Lit("ZZZ"));

	TEST(str_len(s) == 3);
	TEST(!str_is_ref(s));
	TEST(str_is_owner(s));
	TEST(str_eq(s, Lit("ZZZ")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_clone(&s, str_ref_ptr("XXXXX"));

	TEST(str_len(s) == 5);
	TEST(!str_is_ref(s));
	TEST(str_is_owner(s));
	TEST(str_eq(s, Lit("XXXXX")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_clone(&s, s);

	TEST(str_len(s) == 5);
	TEST(!str_is_ref(s));
	TEST(str_is_owner(s));
	TEST(str_eq(s, Lit("XXXXX")));
	TEST(strlen(str_ptr(s)) == str_len(s));
}

TEST_CASE(test_swap) {
	str s1 = Lit("x"), s2 = Lit("y");

	str_swap(&s1, &s2);

	TEST(str_eq(s1, Lit("y")));
	TEST(str_eq(s2, Lit("x")));

	str_swap(&s1, &s2);

	TEST(str_eq(s1, Lit("x")));
	TEST(str_eq(s2, Lit("y")));
}

TEST_CASE(test_auto) {
	str_auto s1 = str_null, s2 = str_null;

	str_clone(&s1, Lit("XXX"));
	str_clone(&s2, Lit("ZZZ"));

	// both strings should be free'd here, or sanitizer will complain
}

static inline
bool same_sign(const int v1, const int v2) {
	return (v1 == 0 && v2 == 0) || (v1 > 0 && v2 > 0) || (v1 < 0 && v2 < 0);
}

TEST_CASE(test_cmp) {
	TEST(same_sign(strcmp("", ""), str_cmp(str_null, str_null)));
	TEST(same_sign(strcmp("xxx", ""), str_cmp(Lit("xxx"), str_null)));
	TEST(same_sign(strcmp("", "xxx"), str_cmp(str_null, Lit("xxx"))));

	TEST(same_sign(strcmp("xxx", "xxx"), str_cmp(Lit("xxx"), Lit("xxx"))));
	TEST(same_sign(strcmp("xxz", "xxz"), str_cmp(Lit("xxz"), Lit("xxz"))));
	TEST(same_sign(strcmp("xxxx", "xxx"), str_cmp(Lit("xxxx"), Lit("xxx"))));
	TEST(same_sign(strcmp("xxx", "xxxx"), str_cmp(Lit("xxx"), Lit("xxxx"))));
}

TEST_CASE(test_sort) {
	str array[] = {
		Lit("xxx"),
		Lit("xxxx"),
		Lit("aaa"),
		Lit("bbb")
	};

	const size_t N = sizeof(array)/sizeof(array[0]);

	str_sort_array(str_order_asc, array, N);

	TEST(str_eq(array[0], Lit("aaa")));
	TEST(str_eq(array[1], Lit("bbb")));
	TEST(str_eq(array[2], Lit("xxx")));
	TEST(str_eq(array[3], Lit("xxxx")));

	str_sort_array(str_order_desc, array, N);

	TEST(str_eq(array[3], Lit("aaa")));
	TEST(str_eq(array[2], Lit("bbb")));
	TEST(str_eq(array[1], Lit("xxx")));
	TEST(str_eq(array[0], Lit("xxxx")));
}

TEST_CASE(test_prefix) {
	const str s = Lit("xxx_yyy_zzz");

	TEST(str_has_prefix(s, Lit("xxx")));
	TEST(str_has_prefix(s, str_null));
	TEST(str_has_prefix(s, Lit("xxx_yyy_zzz")));

	TEST(!str_has_prefix(s, Lit("xxx_yyy_zzz_")));
	TEST(!str_has_prefix(s, Lit("zzz")));
}

TEST_CASE(test_suffix) {
	const str s = Lit("xxx_yyy_zzz");

	TEST(str_has_suffix(s, Lit("zzz")));
	TEST(str_has_suffix(s, str_null));
	TEST(str_has_suffix(s, Lit("xxx_yyy_zzz")));

	TEST(!str_has_suffix(s, Lit("_xxx_yyy_zzz")));
	TEST(!str_has_suffix(s, Lit("xxx")));
}

TEST_CASE(test_concat) {
	str s = str_null;

	str_clone(&s, Lit("123"));
	str_concat(&s, Lit("aaa"), Lit("-"), Lit("bbb"));

	TEST(str_eq(s, Lit("aaa-bbb")));
	TEST(str_is_owner(s));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_concat(&s);

	TEST(str_is_empty(s));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_concat(&s, Lit("aaa"));

	TEST(str_eq(s, Lit("aaa")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_concat(&s, s, Lit("bbb"));

	TEST(str_eq(s, Lit("aaabbb")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_free(s);
}

TEST_CASE(test_join) {
	str s = str_null;

	str_clone(&s, Lit("123"));
	str_join(&s, Lit("-"), Lit("aaa"), Lit("bbb"), Lit("ccc"), Lit("ddd"), s);

	TEST(str_eq(s, Lit("aaa-bbb-ccc-ddd-123")));
	TEST(str_is_owner(s));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_join(&s, Lit("-"));

	TEST(str_is_empty(s));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_join(&s, Lit("-"), Lit("aaa"));

	TEST(str_eq(s, Lit("aaa")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_join(&s, Lit("-"), s, Lit("bbb"));

	TEST(str_eq(s, Lit("aaa-bbb")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_join(&s, str_null, Lit("aaa"), Lit("bbb"));

	TEST(str_eq(s, Lit("aaabbb")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_free(s);
}

TEST_CASE(test_sprintf) {
	str_auto s = Lit("xxx");

	TEST(str_sprintf(&s, "string \"%.*s\" of length %zu", (int)str_len(s), str_ptr(s), str_len(s)));
	TEST(str_eq(s, Lit("string \"xxx\" of length 3")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_assign(&s, Lit("XXXX"));

	for(int i = 0; i < 10; ++i)
		str_concat(&s, s, s);

	TEST(str_sprintf(&s, "%.*s", (int)str_len(s), str_ptr(s)));
	TEST(str_len(s) == 4 * 1024);
	TEST(strlen(str_ptr(s)) == str_len(s));

	for(const char* p = str_ptr(s); p < str_end(s); ++p)
		TEST(*p == 'X');

	TEST(str_sprintf(&s, "%s", ""));
	TEST(str_is_empty(s));
	TEST(strlen(str_ptr(s)) == str_len(s));

	TEST(str_sprintf(&s, "XXX"));
	TEST(str_eq(s, Lit("XXX")));
	TEST(strlen(str_ptr(s)) == str_len(s));
}

TEST_CASE(test_repeat) {
	str_auto s = Lit("xxx");

	str_repeat(&s, 3);

	TEST(str_eq(s, Lit("xxxxxxxxx")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_assign(&s, Lit("xxx"));
	str_repeat(&s, 1);

	TEST(str_eq(s, Lit("xxx")));
	TEST(strlen(str_ptr(s)) == str_len(s));

	str_assign(&s, str_null);
	str_repeat(&s, 10);

	TEST(str_is_empty(s));
	TEST(strlen(str_ptr(s)) == str_len(s));
}

TEST_CASE(test_hash) {
	// better ideas on how to test it?
	TEST(str_hash(Lit("xxx")) == str_hash(Lit("xxx")));
	TEST(str_hash(Lit("yyy")) == str_hash(Lit("yyy")));
	TEST(str_hash(Lit("zzz")) == str_hash(Lit("zzz")));

	// this may fail on hash collision
	TEST(str_hash(Lit("xxx")) != str_hash(Lit("yyy")));
	TEST(str_hash(Lit("yyy")) != str_hash(Lit("zzz")));
	TEST(str_hash(Lit("zzz")) != str_hash(Lit("xxx")));

	TEST(str_hash(Lit("zzz")) != 0ull);
}

TEST_CASE(test_span_chars) {
	// empty strings
	TEST(str_span_chars(str_null, str_null) == 0);
	TEST(str_span_chars(Lit("xxx"), str_null) == 0);
	TEST(str_span_chars(str_null, Lit("xyz")) == 0);

	// one byte pattern
	TEST(str_span_chars(Lit("_"), Lit("_")) == 1);
	TEST(str_span_chars(Lit("_x"), Lit("_")) == 1);
	TEST(str_span_chars(Lit("__x"), Lit("_")) == 2);
	TEST(str_span_chars(Lit("___x"), Lit("_")) == 3);
	TEST(str_span_chars(Lit("___"), Lit("_")) == 3);

	// multi-byte pattern
	TEST(str_span_chars(Lit("__"), Lit("_/-")) == 2);
	TEST(str_span_chars(Lit("\0*"), Lit("_/-\0")) == 1);
	TEST(str_span_chars(Lit("\xFF*"), Lit("_/-\xFF")) == 1);
	TEST(str_span_chars(Lit("ZZ"), Lit("_/-Z")) == 2);
	TEST(str_span_chars(Lit("///"), Lit("_/-")) == 3);
	TEST(str_span_chars(Lit("//-_x"), Lit("_/-")) == 4);
}

TEST_CASE(test_span_nonmatching_chars) {
	// empty strings
	TEST(str_span_nonmatching_chars(str_null, str_null) == 0);
	TEST(str_span_nonmatching_chars(Lit("xxx"), str_null) == 3);
	TEST(str_span_nonmatching_chars(str_null, Lit("xyz")) == 0);

	// one byte pattern
	TEST(str_span_nonmatching_chars(Lit("_"), Lit("_")) == 0);
	TEST(str_span_nonmatching_chars(Lit("x_"), Lit("_")) == 1);
	TEST(str_span_nonmatching_chars(Lit("xx_"), Lit("_")) == 2);
	TEST(str_span_nonmatching_chars(Lit("xxx_"), Lit("_")) == 3);
	TEST(str_span_nonmatching_chars(Lit("xxx"), Lit("_")) == 3);

	// multi-byte pattern
	TEST(str_span_nonmatching_chars(Lit("*_"), Lit("_/-")) == 1);
	TEST(str_span_nonmatching_chars(Lit("x\0"), Lit("_/-\0")) == 1);
	TEST(str_span_nonmatching_chars(Lit("x\xFF"), Lit("_/-\xFF")) == 1);
	TEST(str_span_nonmatching_chars(Lit("YZa"), Lit("_/-Z")) == 1);
	TEST(str_span_nonmatching_chars(Lit("xxx/"), Lit("_/-")) == 3);
	TEST(str_span_nonmatching_chars(Lit("xxx\0-x"), Lit("_/-")) == 4);
}

TEST_CASE(test_span_until_str) {
	TEST(str_span_until_substring(str_null, Lit("xxx")) == 0);
	TEST(str_span_until_substring(Lit("xxx"), str_null) == 0);
	TEST(str_span_until_substring(str_null, str_null) == 0);
	TEST(str_span_until_substring(Lit("xxx-yyy-zzz"), Lit("xxx")) == 0);
	TEST(str_span_until_substring(Lit("xxx-yyy-zzz"), Lit("yyy")) == 4);
	TEST(str_span_until_substring(Lit("xxx-yyy-zzz"), Lit("zzz")) == 8);
	TEST(str_span_until_substring(Lit("xxx-yyy-zzz"), Lit("???")) == 11);
}

TEST_CASE(test_replace_substring) {
	str_auto s = str_null;

	// corner cases
	TEST(str_replace_substring(&s, str_null, str_null) == 0);
	TEST(str_is_empty(s));

	s = Lit("xxx");

	TEST(str_replace_substring(&s, str_null, str_null) == 0);
	TEST(str_eq(s, Lit("xxx")));
	TEST(str_is_ref(s));

	s = str_null;

	TEST(str_replace_substring(&s, Lit("xxx"), str_null) == 0);
	TEST(str_is_empty(s));
	TEST(str_replace_substring(&s, str_null, Lit("xxx")) == 0);
	TEST(str_is_empty(s));

	// single replacement
	s = Lit("xxx_");

	TEST(str_replace_substring(&s, Lit("xxx"), Lit("zzz")) == 1);
	TEST(str_eq(s, Lit("zzz_")));

	str_assign(&s, Lit("_xxx"));

	TEST(str_replace_substring(&s, Lit("xxx"), Lit("zzz")) == 1);
	TEST(str_eq(s, Lit("_zzz")));

	str_assign(&s, Lit("_xxx_"));

	TEST(str_replace_substring(&s, Lit("xxx"), Lit("zzz")) == 1);
	TEST(str_eq(s, Lit("_zzz_")));

	// multiple replacements
	str_assign(&s, Lit("x_x_x_x_x_x_x_x_x"));

	TEST(str_replace_substring(&s, Lit("_"), str_null) == 8);
	TEST(str_eq(s, Lit("xxxxxxxxx")));

	// big string
	const size_t N = 10000;

	str_assign(&s, Lit("x_"));
	str_repeat(&s, N);

	TEST(str_replace_substring(&s, Lit("_"), Lit("X")) == N);
	TEST(str_span_chars(s, Lit("xX")) == 2 * N);

	str_auto s2 = Lit("xX");

	str_repeat(&s2, N);
	TEST(str_eq(s, s2));

	TEST(str_replace_substring(&s2, Lit("xX"), str_null) == N);
	TEST(str_is_empty(s2));
	TEST(str_is_ref(s2));

	TEST(str_replace_substring(&s, Lit("X"), Lit("xxx")) == N);
	TEST(str_len(s) == 4 * N);
	TEST(strlen(str_ptr(s)) == 4 * N);	// just in case
	TEST(str_span_chars(s, Lit("x")) == 4 * N);
}

TEST_CASE(test_replace_chars) {
	str_auto s = str_null;

	// corner cases
	TEST(str_replace_chars(&s, str_null, str_null) == 0);
	TEST(str_is_empty(s));

	s = Lit("xyz");

	TEST(str_replace_chars(&s, str_null, str_null) == 0);
	TEST(str_eq(s, Lit("xyz")));
	TEST(str_is_ref(s));

	s = str_null;

	TEST(str_replace_chars(&s, Lit("xyz"), str_null) == 0);
	TEST(str_is_empty(s));
	TEST(str_replace_chars(&s, str_null, Lit("xyz")) == 0);
	TEST(str_is_empty(s));

	s = Lit("xyz");

	TEST(str_replace_chars(&s, str_null, Lit("xyz")) == 0);
	TEST(str_eq(s, Lit("xyz")));
	TEST(str_is_ref(s));
	TEST(str_replace_chars(&s, Lit("xyz"), str_null) == 3);
	TEST(str_is_empty(s));

	// other replacements
	s = Lit("xyz");

	TEST(str_replace_chars(&s, Lit("y"), Lit("_")) == 1);
	TEST(str_eq(s, Lit("x_z")));
	TEST(str_is_owner(s));

	str_assign(&s, Lit("xyz"));

	TEST(str_replace_chars(&s, Lit("x"), Lit("_")) == 1);
	TEST(str_eq(s, Lit("_yz")));
	TEST(str_is_owner(s));

	str_assign(&s, Lit("xyz"));

	TEST(str_replace_chars(&s, Lit("z"), Lit("_")) == 1);
	TEST(str_eq(s, Lit("xy_")));
	TEST(str_is_owner(s));

	// big string
	const size_t N = 10000;

	str_assign(&s, Lit("xX"));
	str_repeat(&s, N);

	TEST(str_replace_chars(&s, Lit("XYZ"), str_null) == N);
	TEST(str_len(s) == N);

	for(size_t i = 0; i < str_len(s); ++i)
		TEST(*(str_ptr(s) + i) == 'x');

	str_assign(&s, Lit("xX"));
	str_repeat(&s, N);

	TEST(str_replace_chars(&s, Lit("xyz"), str_null) == N);
	TEST(str_len(s) == N);

	for(size_t i = 0; i < str_len(s); ++i)
		TEST(*(str_ptr(s) + i) == 'X');

	str_assign(&s, Lit("xX"));
	str_repeat(&s, N);

	TEST(str_replace_chars(&s, Lit("xX"), Lit("z")) == 2 * N);
	TEST(str_len(s) == 2 * N);

	for(size_t i = 0; i < str_len(s); ++i)
		TEST(*(str_ptr(s) + i) == 'z');

	str_assign(&s, Lit("xx"));
	str_repeat(&s, N);

	TEST(str_replace_chars(&s, Lit("?"), Lit("z")) == 0);
	TEST(str_len(s) == 2 * N);

	for(size_t i = 0; i < str_len(s); ++i)
		TEST(*(str_ptr(s) + i) == 'x');
}

TEST_CASE(test_replace_char_spans) {
	str_auto s = str_null;

	// corner cases
	TEST(str_replace_char_spans(&s, str_null, str_null) == 0);
	TEST(str_is_empty(s));
	TEST(str_is_ref(s));
	TEST(str_replace_char_spans(&s, Lit("xyz"), str_null) == 0);
	TEST(str_is_empty(s));
	TEST(str_is_ref(s));
	TEST(str_replace_char_spans(&s, str_null, Lit("xyz")) == 0);
	TEST(str_is_empty(s));
	TEST(str_is_ref(s));

	s = Lit("xyz");

	TEST(str_replace_char_spans(&s, str_null, str_null) == 0);
	TEST(str_eq(s, Lit("xyz")));
	TEST(str_is_ref(s));
	TEST(str_replace_char_spans(&s, str_null, Lit("xyz")) == 0);
	TEST(str_eq(s, Lit("xyz")));
	TEST(str_is_ref(s));

	// other replacements
	str_assign(&s, Lit("x__y  _z"));

	TEST(str_replace_char_spans(&s, Lit("_ "), Lit("|")) == 2);
	TEST(str_eq(s, Lit("x|y|z")));

	str_assign(&s, Lit(" x__y  _z  "));

	TEST(str_replace_char_spans(&s, Lit("_ "), Lit("|")) == 4);
	TEST(str_eq(s, Lit("|x|y|z|")));

	// big string
	const size_t N = 10000;

	str_assign(&s, Lit(" x\t\n"));
	str_repeat(&s, N);

	TEST(str_replace_char_spans(&s, Lit(" \t\r\n"), str_null) == N + 1);
	TEST(str_len(s) == N);

	for(size_t i = 0; i < str_len(s); ++i)
		TEST(*(str_ptr(s) + i) == 'x');

	str_assign(&s, Lit(" x\t\n"));
	str_repeat(&s, N);

	TEST(str_replace_char_spans(&s, Lit(" \t\r\n"), Lit("x")) == N + 1);
	TEST(str_len(s) == 2 * N + 1);

	for(size_t i = 0; i < str_len(s); ++i)
		TEST(*(str_ptr(s) + i) == 'x');
}

static
bool part_pred(const str s) {
	return str_len(s) < 2;
}

TEST_CASE(test_partition_array) {
	str src[] = {
		str_lit("aaa"),
		str_lit("a"),
		str_lit("aaaa"),
		str_lit("z")
	};

	const size_t N = sizeof(src)/sizeof(src[0]);

	TEST(str_partition_array(part_pred, src, 1) == 0);

	TEST(str_partition_array(part_pred, src, N) == 2);
	TEST(str_eq(src[0], str_lit("a")));
	TEST(str_eq(src[1], str_lit("z")));
	TEST(str_partition_array(part_pred, src, 1) == 1);

	src[0] = str_lit("?");
	src[2] = str_lit("*");

	TEST(str_partition_array(part_pred, src, N) == 3);
	TEST(str_eq(src[0], str_lit("?")));
	TEST(str_eq(src[1], str_lit("z")));
	TEST(str_eq(src[2], str_lit("*")));
	TEST(str_eq(src[3], str_lit("aaa")));

	TEST(str_partition_array(part_pred, NULL, 42) == 0);
	TEST(str_partition_array(part_pred, src, 0) == 0);
}

TEST_CASE(test_unique_partition_array) {
	str src[] = {
		str_lit("aaa"),
		str_lit("aaa"),
		str_lit("aaa"),
		str_lit("bbb"),
		str_lit("ccc"),
		str_lit("ccc"),
		str_lit("ccc"),
		str_lit("ddd"),
	};

	const size_t N = sizeof(src)/sizeof(src[0]);

	TEST(str_unique_partition_array(src, N) == 4);
	TEST(str_eq(src[0], str_lit("aaa")));
	TEST(str_eq(src[1], str_lit("bbb")));
	TEST(str_eq(src[2], str_lit("ccc")));
	TEST(str_eq(src[3], str_lit("ddd")));
}
