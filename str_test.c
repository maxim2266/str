#define _POSIX_C_SOURCE 200809L

#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>

// make sure assert is always enabled
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

#define passed	printf("passed: %s\n", __func__)

static
void test_str_lit(void)
{
	const str s = str_lit("ZZZ");

	assert(str_len(s) == 3);
	assert(str_is_ref(s));
	assert(!str_is_owner(s));
	assert(str_eq(s, str_lit("ZZZ")));

	passed;
}

static
void test_str_dup(void)
{
	str s = str_null;

	assert(str_cpy(&s, str_lit("ZZZ")) == 0);

	assert(str_len(s) == 3);
	assert(!str_is_ref(s));
	assert(str_is_owner(s));
	assert(str_eq(s, str_lit("ZZZ")));
	assert(*str_end(s) == 0);

	str_free(s);
	passed;
}

static
void test_str_clear(void)
{
	str s = str_null;

	assert(str_cpy(&s, str_lit("ZZZ")) == 0);

	assert(str_len(s) == 3);
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_clear(&s);

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_str_move(void)
{
	str s1 = str_null;

	assert(str_cpy(&s1, str_lit("ZZZ")) == 0);

	str s2 = str_move(&s1);

	assert(str_is_empty(s1));
	assert(str_is_ref(s1));

	assert(str_is_owner(s2));
	assert(str_eq(s2, str_lit("ZZZ")));

	str_free(s2);
	passed;
}

static
void test_str_ref(void)
{
	str s = str_ref("ZZZ");

	assert(str_len(s) == 3);
	assert(str_is_ref(s));

	s = str_ref(s);

	assert(str_is_ref(s));
	assert(str_eq(s, str_lit("ZZZ")));

	const char* const p = "ZZZ";

	s = str_ref(p);

	assert(str_is_ref(s));
	assert(str_eq(s, str_lit("ZZZ")));

	passed;
}

static
void test_str_cmp(void)
{
	const str s = str_lit("zzz");

	assert(str_cmp(s, s) == 0);
	assert(str_cmp(s, str_lit("zzz")) == 0);
	assert(str_cmp(s, str_lit("zz")) > 0);
	assert(str_cmp(s, str_lit("zzzz")) < 0);
	assert(str_cmp(s, str_null) > 0);
	assert(str_cmp(str_null, s) < 0);
	assert(str_cmp(str_null, str_null) == 0);
	assert(str_eq(s, str_lit("zzz")));

	passed;
}

static
void test_str_cmp_ci(void)
{
	const str s = str_lit("zzz");

	assert(str_cmp_ci(s, s) == 0);
	assert(str_cmp_ci(s, str_lit("zzz")) == 0);
	assert(str_cmp_ci(s, str_lit("zz")) > 0);
	assert(str_cmp_ci(s, str_lit("zzzz")) < 0);
	assert(str_cmp_ci(s, str_null) > 0);
	assert(str_cmp_ci(str_null, s) < 0);
	assert(str_cmp_ci(str_null, str_null) == 0);
	assert(str_cmp_ci(s, str_lit("ZZZ")) == 0);
	assert(str_cmp_ci(s, str_lit("ZZ")) > 0);
	assert(str_cmp_ci(s, str_lit("ZZZZ")) < 0);
	assert(str_eq_ci(s, str_lit("ZZZ")));

	passed;
}

static
void test_str_acquire(void)
{
	str s = str_acquire(strdup("ZZZ"));

	assert(str_is_owner(s));
	assert(str_eq(s, str_lit("ZZZ")));
	assert(*str_end(s) == 0);

	str_free(s);
	passed;
}

static
void test_str_cat(void)
{
	str s = str_null;

	assert(str_cat(&s, str_lit("AAA"), str_lit("BBB"), str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("AAABBBCCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_cat(&s, str_null, str_null, str_null) == 0);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_str_join(void)
{
	str s = str_null;

	assert(str_join(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("AAA_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_null, str_lit("BBB"), str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_lit("AAA"), str_null, str_lit("CCC")) == 0);

	assert(str_eq(s, str_lit("AAA__CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_null) == 0);

	assert(str_eq(s, str_lit("AAA_BBB_")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_lit("_"), str_null, str_null, str_null) == 0);

	assert(str_eq(s, str_lit("__")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	assert(str_join(&s, str_null) == 0);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_composition(void)
{
	str s = str_lit(", ");

	assert(str_join(&s, s, str_lit("Here"), str_lit("there"), str_lit("and everywhere")) == 0);
	assert(str_cat(&s, s, str_lit("...")) == 0);

	assert(str_eq(s, str_lit("Here, there, and everywhere...")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_free(s);
	passed;
}

static
void test_sort(void)
{
	str src[] = { str_lit("z"), str_lit("zzz"), str_lit("aaa"), str_lit("bbb") };

	str_sort_range(str_order_asc, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq(src[0], str_lit("aaa")));
	assert(str_eq(src[1], str_lit("bbb")));
	assert(str_eq(src[2], str_lit("z")));
	assert(str_eq(src[3], str_lit("zzz")));

	str_sort_range(str_order_desc, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq(src[0], str_lit("zzz")));
	assert(str_eq(src[1], str_lit("z")));
	assert(str_eq(src[2], str_lit("bbb")));
	assert(str_eq(src[3], str_lit("aaa")));

	passed;
}

static
void test_sort_ci(void)
{
	str src[] = { str_lit("ZZZ"), str_lit("zzz"), str_lit("aaa"), str_lit("AAA") };

	str_sort_range(str_order_asc_ci, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq_ci(src[0], str_lit("aaa")));
	assert(str_eq_ci(src[1], str_lit("aaa")));
	assert(str_eq_ci(src[2], str_lit("zzz")));
	assert(str_eq_ci(src[3], str_lit("zzz")));

	str_sort_range(str_order_desc_ci, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq_ci(src[0], str_lit("zzz")));
	assert(str_eq_ci(src[1], str_lit("zzz")));
	assert(str_eq_ci(src[2], str_lit("aaa")));
	assert(str_eq_ci(src[3], str_lit("aaa")));

	passed;
}

static
void test_search(void)
{
	str src[] = { str_lit("z"), str_lit("zzz"), str_lit("aaa"), str_lit("bbb") };
	const size_t count = sizeof(src)/sizeof(src[0]);

	str_sort_range(str_order_asc, src, count);

	assert(str_search_range(src[0], src, count) == &src[0]);
	assert(str_search_range(src[1], src, count) == &src[1]);
	assert(str_search_range(src[2], src, count) == &src[2]);
	assert(str_search_range(src[3], src, count) == &src[3]);
	assert(str_search_range(str_lit("xxx"), src, count) == NULL);

	passed;
}

static
void test_prefix(void)
{
	const str s = str_lit("abcd");

	assert(str_has_prefix(s, str_null));
	assert(str_has_prefix(s, str_lit("a")));
	assert(str_has_prefix(s, str_lit("ab")));
	assert(str_has_prefix(s, str_lit("abc")));
	assert(str_has_prefix(s, str_lit("abcd")));

	assert(!str_has_prefix(s, str_lit("zzz")));
	assert(!str_has_prefix(s, str_lit("abcde")));

	passed;
}

static
void test_suffix(void)
{
	const str s = str_lit("abcd");

	assert(str_has_suffix(s, str_null));
	assert(str_has_suffix(s, str_lit("d")));
	assert(str_has_suffix(s, str_lit("cd")));
	assert(str_has_suffix(s, str_lit("bcd")));
	assert(str_has_suffix(s, str_lit("abcd")));

	assert(!str_has_suffix(s, str_lit("zzz")));
	assert(!str_has_suffix(s, str_lit("_abcd")));

	passed;
}

static
void test_cpy_to_fd(void)
{
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_cpy(fileno(tmp), str_lit("ZZZ")) == 0);

	rewind(tmp);

	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == 3);
	assert(memcmp(buff, "ZZZ", 3) == 0);

	fclose(tmp);
	passed;
}

static
void test_cpy_to_stream(void)
{
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_cpy(tmp, str_lit("ZZZ")) == 0);

	assert(fflush(tmp) == 0);
	rewind(tmp);

	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == 3);
	assert(memcmp(buff, "ZZZ", 3) == 0);

	fclose(tmp);
	passed;
}

static
void test_cat_range_to_fd(void)
{
	const str src[] = {
		str_lit("aaa"),
		str_lit("bbb"),
		str_null,
		str_lit("ccc"),
		str_lit("ddd"),
		str_null,
		str_null
	};

	const size_t num_items = sizeof(src)/sizeof(src[0]);

	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_cat_range(fileno(tmp), src, num_items) == 0);

	rewind(tmp);

	const char res[] = "aaabbbcccddd";
	const size_t len = sizeof(res) - 1;
	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == len);
	assert(memcmp(buff, res, len) == 0);

	fclose(tmp);
	passed;
}

static
void test_cat_large_range_to_fd(void)
{
	// prepare data
	const size_t n = 100000;
	str* const src = calloc(n, sizeof(str));

	assert(src != NULL);

	char buff[100];

	for(unsigned i = 0; i < n; i++)
		assert(str_cpy(&src[i], str_ref_chars(buff, sprintf(buff, "%u\n", i))) == 0);

	// write to file
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_cat_range(fileno(tmp), src, n) == 0);

	// clear input data
	for(unsigned i = 0; i < n; ++i)
		str_free(src[i]);

	free(src);

	// validate
	rewind(tmp);

	char* line = NULL;
	size_t cap = 0;
	ssize_t len;
	int i = 0;

	while((len = getline(&line, &cap, tmp)) >= 0)
		assert(atoi(line) == i++);

	assert(i == (int)n);

	// all done
	fclose(tmp);
	free(line);
	passed;
}

static
void test_cat_range_to_stream(void)
{
	const str src[] = {
		str_lit("aaa"),
		str_lit("bbb"),
		str_null,
		str_lit("ccc"),
		str_lit("ddd"),
		str_null,
		str_null
	};

	const size_t num_items = sizeof(src)/sizeof(src[0]);

	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_cat_range(tmp, src, num_items) == 0);

	assert(fflush(tmp) == 0);
	rewind(tmp);

	const char res[] = "aaabbbcccddd";
	const size_t len = sizeof(res) - 1;
	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == len);
	assert(memcmp(buff, res, len) == 0);

	fclose(tmp);
	passed;
}

static
void test_join_to_fd(void)
{
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_join(fileno(tmp), str_lit("_"), str_lit("aaa"), str_lit("bbb"), str_lit("ccc")) == 0);

	rewind(tmp);

	const char res[] = "aaa_bbb_ccc";
	const size_t len = sizeof(res) - 1;
	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == len);
	assert(memcmp(buff, res, len) == 0);

	fclose(tmp);
	passed;
}

static
void test_join_large_range_to_fd(void)
{
	// prepare data
	const size_t n = 100000;
	str* const src = calloc(n, sizeof(str));

	assert(src != NULL);

	char buff[100];

	for(unsigned i = 0; i < n; i++)
		assert(str_cpy(&src[i], str_ref_chars(buff, sprintf(buff, "%u", i))) == 0);

	// write to file
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_join_range(fileno(tmp), str_lit("\n"), src, n) == 0);

	// clear input data
	for(unsigned i = 0; i < n; ++i)
		str_free(src[i]);

	free(src);

	// validate
	rewind(tmp);

	char* line = NULL;
	size_t cap = 0;
	ssize_t len;
	int i = 0;

	while((len = getline(&line, &cap, tmp)) >= 0)
		assert(atoi(line) == i++);

	assert(i == (int)n);

	// all done
	fclose(tmp);
	free(line);
	passed;
}

static
void test_join_to_stream(void)
{
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_join(tmp, str_lit("_"), str_lit("aaa"), str_lit("bbb"), str_lit("ccc")) == 0);

	assert(fflush(tmp) == 0);
	rewind(tmp);

	const char res[] = "aaa_bbb_ccc";
	const size_t len = sizeof(res) - 1;
	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == len);
	assert(memcmp(buff, res, len) == 0);

	fclose(tmp);
	passed;
}

static
bool part_pred(const str s) { return str_len(s) < 2; }

static
void test_partition_range(void)
{
	str src[] = { str_lit("aaa"), str_lit("a"), str_lit("aaaa"), str_lit("z") };

	assert(str_partition_range(part_pred, src, 1) == 0);

	assert(str_partition_range(part_pred, src, sizeof(src)/sizeof(src[0])) == 2);
	assert(str_eq(src[0], str_lit("a")));
	assert(str_eq(src[1], str_lit("z")));
	assert(str_partition_range(part_pred, src, 1) == 1);

	src[0] = str_lit("?");
	src[2] = str_lit("*");

	assert(str_partition_range(part_pred, src, sizeof(src)/sizeof(src[0])) == 3);
	assert(str_eq(src[0], str_lit("?")));
	assert(str_eq(src[1], str_lit("z")));
	assert(str_eq(src[2], str_lit("*")));
	assert(str_eq(src[3], str_lit("aaa")));

	assert(str_partition_range(part_pred, NULL, 42) == 0);
	assert(str_partition_range(part_pred, src, 0) == 0);

	passed;
}

static
void test_unique_range(void)
{
	str src[] = {
		str_lit("zzz"),
		str_lit("aaa"),
		str_lit("zzz"),
		str_lit("bbb"),
		str_lit("aaa"),
		str_lit("ccc"),
		str_lit("ccc"),
		str_lit("aaa"),
		str_lit("ccc"),
		str_lit("zzz")
	};

	assert(str_unique_range(src, sizeof(src)/sizeof(src[0])) == 4);
	assert(str_eq(src[0], str_lit("aaa")));
	assert(str_eq(src[1], str_lit("bbb")));
	assert(str_eq(src[2], str_lit("ccc")));
	assert(str_eq(src[3], str_lit("zzz")));

	passed;
}

static
void test_from_file(void)
{
	str fname = str_null;

	assert(str_cat(&fname, str_lit("tmp_"), str_ref_chars(__func__, sizeof(__func__) - 1)) == 0);

	FILE* const stream = fopen(str_ptr(fname), "w");

	assert(stream);
	assert(str_join(stream, str_lit(" "), str_lit("aaa"), str_lit("bbb"), str_lit("ccc")) == 0);
	assert(fclose(stream) == 0);

	str res = str_null;

	assert(str_from_file(&res, str_ptr(fname)) == 0);
	unlink(str_ptr(fname));
	assert(str_eq(res, str_lit("aaa bbb ccc")));
	assert(str_is_owner(res));

	// test errors
	assert(str_from_file(&res, ".") == EISDIR);
	assert(str_from_file(&res, "/dev/null") == EOPNOTSUPP);
	assert(str_from_file(&res, "does-not-exist") == ENOENT);

	str_free(res);
	str_free(fname);
	passed;
}

#ifdef __STDC_UTF_32__

static
void test_codepoint_iterator(void)
{
	const str src = str_lit(u8"жёлтый");	// means "yellow" in Russian
	static const char32_t src32[] = { U'ж', U'ё', U'л', U'т', U'ы', U'й' };
	size_t i = 0;
	char32_t c;

	for_each_codepoint(c, src)
	{
		assert(i < sizeof(src32)/sizeof(src32[0]));
		assert(c == src32[i++]);
	}

	assert(c == CPI_END_OF_STRING);
	assert(i == sizeof(src32)/sizeof(src32[0]));

	// empty string iteration
	c = 0;

	for_each_codepoint(c, str_null)
		assert(0);

	assert(c == CPI_END_OF_STRING);
	passed;
}

#endif	// ifdef __STDC_UTF_32__

static
void test_tok(void)
{
	typedef struct
	{
		const str src, delim;
		const unsigned n_tok;
		const str tok[3];
	} test_data;

	static const test_data t[] =
	{
		{
			str_lit("a,b,c"),
			str_lit(","),
			3,
			{ str_lit("a"), str_lit("b"), str_lit("c") }
		},
		{
			str_lit(",,a,b,,c,"),
			str_lit(","),
			3,
			{ str_lit("a"), str_lit("b"), str_lit("c") }
		},
		{
			str_lit("aaa;=~bbb~,=ccc="),
			str_lit(",;=~"),
			3,
			{ str_lit("aaa"), str_lit("bbb"), str_lit("ccc") }
		},
		{
			str_lit(""),
			str_lit(","),
			0,
			{ }
		},
		{
			str_lit(""),
			str_lit(""),
			0,
			{ }
		},
		{
			str_lit(",.;,.;;.,;.,"),
			str_lit(",.;"),
			0,
			{ }
		},
		{
			str_lit("aaa,bbb,ccc"),
			str_lit(""),
			1,
			{ str_lit("aaa,bbb,ccc") }
		},
		{
			str_lit("aaa,bbb,ccc"),
			str_lit(";-="),
			1,
			{ str_lit("aaa,bbb,ccc") }
		}
	};

	for(unsigned i = 0; i < sizeof(t)/sizeof(t[0]); ++i)
	{
		unsigned tok_count = 0;

		str tok = str_null;
		str_tok_state state;

		str_tok_init(&state, t[i].src, t[i].delim);

		while(str_tok(&tok, &state))
		{
// 			printf("%u-%u: \"%.*s\" %zu\n",
// 					i, tok_count, (int)str_len(tok), str_ptr(tok), str_len(tok));
// 			fflush(stdout);

			assert(tok_count < t[i].n_tok);
			assert(str_eq(tok, t[i].tok[tok_count]));

			++tok_count;
		}

		assert(tok_count == t[i].n_tok);
	}

	passed;
}

static
void test_partition(void)
{
	typedef struct
	{
		const bool res;
		const str src, patt, pref, suff;
	} test_data;

	static const test_data t[] =
	{
		{ true, str_lit("...abc..."), str_lit("abc"), str_lit("..."), str_lit("...") },
		{ true, str_lit("......abc"), str_lit("abc"), str_lit("......"), str_null },
		{ true, str_lit("abc......"), str_lit("abc"), str_null, str_lit("......") },

		{ true, str_lit("...a..."), str_lit("a"), str_lit("..."), str_lit("...") },
		{ true, str_lit("......a"), str_lit("a"), str_lit("......"), str_null },
		{ true, str_lit("a......"), str_lit("a"), str_null, str_lit("......") },

		{ false, str_lit("zzz"), str_null, str_lit("zzz"), str_null },
		{ false, str_null, str_lit("zzz"), str_null, str_null },
		{ false, str_null, str_null, str_null, str_null },

		{ false, str_lit("...zzz..."), str_lit("xxx"), str_lit("...zzz..."), str_null },
		{ false, str_lit("...xxz..."), str_lit("xxx"), str_lit("...xxz..."), str_null },
		{ true, str_lit("...xxz...xxx."), str_lit("xxx"), str_lit("...xxz..."), str_lit(".") },
		{ true, str_lit(u8"...цифры___"), str_lit(u8"цифры"), str_lit("..."), str_lit("___") }
	};

	for(unsigned i = 0; i < sizeof(t)/sizeof(t[0]); ++i)
	{
		str pref = str_lit("???"), suff = str_lit("???");

		assert(str_partition(t[i].src, t[i].patt, &pref, &suff) == t[i].res);
		assert(str_eq(pref, t[i].pref));
		assert(str_eq(suff, t[i].suff));
	}

	passed;
}

int main(void)
{
	// tests
	test_str_lit();
	test_str_dup();
	test_str_clear();
	test_str_move();
	test_str_ref();
	test_str_cmp();
	test_str_cmp_ci();
	test_str_acquire();
	test_str_cat();
	test_str_join();
	test_composition();
	test_sort();
	test_sort_ci();
	test_search();
	test_prefix();
	test_suffix();
	test_cpy_to_fd();
	test_cpy_to_stream();
	test_cat_range_to_fd();
	test_cat_large_range_to_fd();
	test_cat_range_to_stream();
	test_join_to_fd();
	test_join_large_range_to_fd();
	test_join_to_stream();
	test_partition_range();
	test_unique_range();
	test_from_file();
	test_tok();
	test_partition();

#ifdef __STDC_UTF_32__
	assert(setlocale(LC_ALL, "C.UTF-8"));

	test_codepoint_iterator();
#endif

	return puts("OK.") < 0;
}
