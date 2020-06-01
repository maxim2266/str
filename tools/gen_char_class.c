#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wctype.h>
#include <error.h>
#include <errno.h>
#include <stdbool.h>

// platform checks
#ifndef __STDC_ISO_10646__
#error "this platform does not seem to support UNICODE (\"__STDC_ISO_10646__\" is not defined)"
#endif

#if __SIZEOF_WCHAR_T__ < 4 || __SIZEOF_WINT_T__ < 4
#error "this platform does not seem to have usable wchar_t (both sizeof(wchar_t) and sizeof(wint_t) should be at least 4)"
#endif

// i/o helpers
#define die(msg, ...)	error(1, errno, "" msg, ##__VA_ARGS__)

#define do_printf(fmt, ...)	\
	do {	\
		if(printf(fmt, ##__VA_ARGS__) < 0)	\
			die("error writing output");	\
	} while(0)

#define do_write(str)	\
	do {	\
		if(fwrite((str), 1, sizeof(str) - 1, stdout) != sizeof(str) - 1)	\
			die("error writing output");	\
	} while(0)

// char type selector (isw*() functions)
typedef int (*selector)(wint_t wc);

// option parser
static __attribute__((noreturn))
void usage_exit(void)
{
	static const char usage[] =
		"Usage: %s SELECTOR\n"
		"  Generates a character classification function that does the same as its\n"
		"  isw*() counterpart under en_US.UTF8 locale on the current platform.\n"
		"  SELECTOR specifies the classification function to use, it should be one of:\n"
		"    --alnum  -> use iswalnum()\n"
		"    --alpha  -> use iswalpha()\n"
		"    --blank  -> use iswblank()\n"
		"    --cntrl  -> use iswcntrl()\n"
		"    --digit  -> use iswdigit()\n"
		"    --graph  -> use iswgraph()\n"
		"    --lower  -> use iswlower()\n"
		"    --print  -> use iswprint()\n"
		"    --punct  -> use iswpunct()\n"
		"    --space  -> use iswspace()\n"
		"    --upper  -> use iswupper()\n"
		"    --xdigit -> use iswxdigit()\n";

	fprintf(stderr, usage, program_invocation_short_name);
	exit(1);
}

static
selector fn;

static
const char* fn_name;

#define ARG(name)	\
	if(strcmp(argv[1], "--" #name) == 0) {	\
		fn = isw ## name; fn_name = #name;	\
		return;	\
	}

static
void read_opts(int argc, char* const argv[])
{
	if(argc != 2)
		usage_exit();

	ARG(alnum)
	ARG(alpha)
	ARG(blank)
	ARG(cntrl)
	ARG(digit)
	ARG(graph)
	ARG(lower)
	ARG(print)
	ARG(punct)
	ARG(space)
	ARG(upper)
	ARG(xdigit)

	if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
		usage_exit();

	error(1, 0, "unknown option: \"%s\"", argv[1]);
}

#undef ARG

// range printing
static
void print_range(const wint_t first, const wint_t last)
{
	if(first == last)
		do_printf("\t\tcase 0x%.2X:\n", first);
	else
		do_printf("\t\tcase 0x%.2X ... 0x%.2X:\n", first, last);
}

// header/footer
static
const char header[] =
	"bool is_%s(const char32_t c)\n"
	"{\n"
	"	switch(c)\n"
	"	{\n";

static
const char footer[] =
	"			return true;\n"
	"		default:\n"
	"			return false;\n"
	"	}\n"
	"}\n";

// main
#define UTF32_MAX_CHAR	0x10ffff
#define LOC				"en_US.UTF8"

int main(int argc, char* const argv[])
{
	read_opts(argc, argv);

	errno = 0;

	if(!setlocale(LC_ALL, LOC))
		die("cannot change current locale to \"" LOC "\"");

	do_printf(header, fn_name);

	wint_t first = 0;
	bool in_range = false;

	for(wint_t c = 0; c <= UTF32_MAX_CHAR; ++c)
	{
		const bool match = (fn(c) != 0);

		if(in_range && !match)
			print_range(first, c - 1);
		else if(!in_range && match)
			first = c;

		in_range = match;
	}

	if(in_range)
		print_range(first, UTF32_MAX_CHAR);

	do_write(footer);

	if(fflush(stdout))
		die("error writing output");

	return 0;
}
