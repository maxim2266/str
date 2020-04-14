#define _XOPEN_SOURCE 500

#include "str.h"

#include <string.h>
#include <stdio.h>

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

	str_dup(&s, str_lit("ZZZ"));

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

	str_dup(&s, str_lit("ZZZ"));

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

	str_dup(&s1, str_lit("ZZZ"));

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
	str s = str_null;

	str_acquire(&s, strdup("ZZZ"));

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

	str_cat(&s, str_lit("AAA"), str_lit("BBB"), str_lit("CCC"));

	assert(str_eq(s, str_lit("AAABBBCCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_cat(&s, str_null, str_null, str_null);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_str_join(void)
{
	str s = str_null;

	str_join(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_lit("CCC"));

	assert(str_eq(s, str_lit("AAA_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join(&s, str_lit("_"), str_null, str_lit("BBB"), str_lit("CCC"));

	assert(str_eq(s, str_lit("_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join(&s, str_lit("_"), str_lit("AAA"), str_null, str_lit("CCC"));

	assert(str_eq(s, str_lit("AAA__CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_null);

	assert(str_eq(s, str_lit("AAA_BBB_")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join(&s, str_lit("_"), str_null, str_null, str_null);

	assert(str_eq(s, str_lit("__")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join(&s, str_null);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_str_join_ignore_empty(void)
{
	str s = str_null;

	str_join_ignore_empty(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_lit("CCC"));

	assert(str_eq(s, str_lit("AAA_BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join_ignore_empty(&s, str_lit("_"), str_lit("AAA"), str_null, str_lit("CCC"));

	assert(str_eq(s, str_lit("AAA_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join_ignore_empty(&s, str_lit("_"), str_lit("AAA"), str_lit("BBB"), str_null);

	assert(str_eq(s, str_lit("AAA_BBB")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join_ignore_empty(&s, str_lit("_"), str_null, str_lit("BBB"), str_lit("CCC"));

	assert(str_eq(s, str_lit("BBB_CCC")));
	assert(str_is_owner(s));
	assert(*str_end(s) == 0);

	str_join_ignore_empty(&s, str_null);	// this simply clears the target string

	assert(str_is_empty(s));
	assert(str_is_ref(s));

	passed;
}

static
void test_composition(void)
{
	str s = str_lit(", ");

	str_join(&s, s, str_lit("Here"), str_lit("there"), str_lit("and everywhere"));
	str_cat(&s, s, str_lit("..."));

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

	str_sort(str_order_asc, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq(src[0], str_lit("aaa")));
	assert(str_eq(src[1], str_lit("bbb")));
	assert(str_eq(src[2], str_lit("z")));
	assert(str_eq(src[3], str_lit("zzz")));

	str_sort(str_order_desc, src, sizeof(src)/sizeof(src[0]));

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

	str_sort(str_order_asc_ci, src, sizeof(src)/sizeof(src[0]));

	assert(str_eq_ci(src[0], str_lit("aaa")));
	assert(str_eq_ci(src[1], str_lit("aaa")));
	assert(str_eq_ci(src[2], str_lit("zzz")));
	assert(str_eq_ci(src[3], str_lit("zzz")));

	str_sort(str_order_desc_ci, src, sizeof(src)/sizeof(src[0]));

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

	str_sort(str_order_asc, src, count);

	assert(str_search(src[0], src, count) == &src[0]);
	assert(str_search(src[1], src, count) == &src[1]);
	assert(str_search(src[2], src, count) == &src[2]);
	assert(str_search(src[3], src, count) == &src[3]);
	assert(str_search(str_lit("xxx"), src, count) == NULL);

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
void test_write(void)
{
	FILE* const tmp = tmpfile();

	assert(tmp != NULL);
	assert(str_write(fileno(tmp), str_lit("ZZZ")) == 0);

	rewind(tmp);

	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == 3);
	assert(memcmp(buff, "ZZZ", 3) == 0);

	fclose(tmp);
	passed;
}

static
void test_write_range(void)
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
	assert(str_write_range(fileno(tmp), src, num_items) == 0);

	rewind(tmp);

	const char res[] = "aaabbbcccddd";
	const size_t len = sizeof(res) - 1;
	char buff[32];

	assert(fread(buff, 1, sizeof(buff), tmp) == len);
	assert(memcmp(buff, res, len) == 0);

	fclose(tmp);
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
	test_str_join_ignore_empty();
	test_composition();
	test_sort();
	test_sort_ci();
	test_search();
	test_prefix();
	test_suffix();
	test_write();
	test_write_range();

	return puts("OK.") < 0;
}
