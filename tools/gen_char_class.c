/*
BSD 3-Clause License

Copyright (c) 2020,2021,2022,2023 Maxim Konakov and contributors
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wctype.h>
#include <errno.h>
#include <stdbool.h>

// platform checks
#ifndef __STDC_ISO_10646__
#error "this platform does not support UNICODE (\"__STDC_ISO_10646__\" is not defined)"
#endif

#if __SIZEOF_WCHAR_T__ < 4 || __SIZEOF_WINT_T__ < 4
#error "this platform does not have a usable wchar_t (both sizeof(wchar_t) and sizeof(wint_t) should be at least 4)"
#endif

// i/o helpers
static __attribute((noinline, noreturn))
void die(const char* const msg)
{
	perror(msg);
	exit(1);
}

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
		"Usage: gen-char-class SELECTOR\n"
		"  Generate a character classification C function that does the same as its\n"
		"  isw*() counterpart under the current locale as specified by LC_ALL\n"
		"  environment variable. SELECTOR specifies the classification function\n"
		"  to generate, it must be any one of:\n"
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

	fputs(usage, stderr);
	exit(1);
}

static
selector fn;

static
const char* fn_name;

static
const char* loc;

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

	fprintf(stderr, "unknown option: \"%s\"\n", argv[1]);
	exit(1);
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
	"/* LC_ALL = \"%s\" */\n"
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

int main(int argc, char* const argv[])
{
	read_opts(argc, argv);

	loc = getenv("LC_ALL");

	if(loc && !setlocale(LC_ALL, loc))
		die("cannot change current locale");

	errno = 0;
	do_printf(header, loc ? loc : "", fn_name);

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
