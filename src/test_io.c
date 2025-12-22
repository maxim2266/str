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

#define _POSIX_C_SOURCE	200809L

#include "mite/mite.h"
#include "../str.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define Lit str_lit

TEST_CASE(test_concat_to_stream) {
	FILE* const stream = tmpfile();

	TESTF(stream, "tmpfile: %m");

	const int err = str_concat_to_stream(stream, Lit("123"), Lit("456"), Lit("7890"));

	TESTF(err == 0, "str_concat_to_stream: %s", strerror(err));
	TESTF(fseek(stream, 0, SEEK_SET) == 0, "fseek: %m");

	char buff[32];

	TEST(fread(buff, 1, sizeof(buff), stream) == 10);
	TEST(memcmp(buff, "1234567890", 10) == 0);

	fclose(stream);
}

#define FILE_NAME			"test-data-file.tmp"
#define FILE_DATA_CHUNK		"0123456789ABCDEF"
#define FILE_DATA_CHUNK_LEN	(sizeof(FILE_DATA_CHUNK) - 1)

static
void del_test_file(void) {
	unlink(FILE_NAME);
}

static
void create_file(const size_t num_chunks) {
	FILE* const stream = fopen(FILE_NAME, "w");

	TESTF(stream, "fopen: %m");

	for(size_t i = 0; i < num_chunks; ++i) {
		if(fwrite(FILE_DATA_CHUNK, 1, FILE_DATA_CHUNK_LEN, stream) < FILE_DATA_CHUNK_LEN) {
			fclose(stream);
			TESTF(false, "fwrite: %m");
		}
	}

	TESTF(fclose(stream) == 0, "fclose: %m");
}

TEST_CASE(test_read_all_file) {
	atexit(del_test_file);

	// file with content
	create_file(1);

	// read it back
	str_auto s = STR_NULL;
	int err = str_read_all_file(&s, FILE_NAME);

	TESTF(err == 0, "str_read_all_file: %s", strerror(err));
	TEST(str_eq(s, Lit(FILE_DATA_CHUNK)));
	TEST(str_is_owner(s));

	// empty file
	TESTF(truncate(FILE_NAME, 0) == 0, "truncate: %m");

	err = str_read_all_file(&s, FILE_NAME);

	TESTF(err == 0, "str_read_all_file: %s", strerror(err));
	TEST(str_is_empty(s));

	// directory
	err = str_read_all_file(&s, ".");

	TESTF(err == EISDIR, "str_read_all_file: %s", strerror(err));
}

TEST_CASE(test_concat_to_fd) {
	const int oflag = O_WRONLY | O_CREAT | O_TRUNC;
	const int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	int fd, err;

	// one record file
	while((fd = open(FILE_NAME, oflag, mode)) < 0)
		TESTF(errno == EINTR, "open: %m");

	err = str_concat_to_fd(fd, Lit(FILE_DATA_CHUNK));

	TESTF(err == 0, "str_concat_to_fd: %s", strerror(err));
	TESTF(close(fd) == 0, "close: %m");

	str_auto s = STR_NULL;

	err = str_read_all_file(&s, FILE_NAME);

	TESTF(err == 0, "str_read_all_file: %s", strerror(err));
	TEST(str_eq(s, Lit(FILE_DATA_CHUNK)));

	// big file
	const size_t N = 2000;
	str input[N];

	for(size_t i = 0; i < N; ++i)
		input[i] = Lit(FILE_DATA_CHUNK);

	while((fd = open(FILE_NAME, oflag, mode)) < 0)
		TESTF(errno == EINTR, "open: %m");

	err = str_concat_array_to_fd(fd, input, N);

	TESTF(err == 0, "str_concat_array_to_fd: %s", strerror(err));
	TESTF(close(fd) == 0, "close: %m");

	str_auto exp = STR_NULL;

	str_repeat(&exp, Lit(FILE_DATA_CHUNK), N);

	err = str_read_all_file(&s, FILE_NAME);

	TESTF(err == 0, "str_read_all_file: %s", strerror(err));
	TEST(str_eq(s, exp));

	// test for error
	while((fd = open(FILE_NAME, O_RDONLY)) < 0)
		TESTF(errno == EINTR, "open: %m");

	TESTF(str_concat_to_fd(fd, Lit("xxx")) == EBADF, "str_concat_to_fd: %m");
	// and the fd must be closed here
	TEST(close(fd) < 0);
}

TEST_CASE(test_get_line) {
	FILE* const stream = tmpfile();

	TESTF(stream, "tmpfile: %m");

	// read empty file
	str_auto s = STR_NULL;
	int err;

	TESTF((err = str_get_line(&s, stream, '\n')) == -1, "str_get_line: %s", strerror(err));

	// add content
	err = str_concat_to_stream(stream, Lit("123\n"), Lit("456\n"), Lit("789"));

	TESTF(err == 0, "str_concat_to_stream: %s", strerror(err));
	TESTF(fseek(stream, 0, SEEK_SET) == 0, "fseek: %m");

	// read content
	TESTF((err = str_get_line(&s, stream, '\n')) == 0, "str_get_line(1): %s", strerror(err));
	TEST(str_eq(s, Lit("123\n")));
	TESTF((err = str_get_line(&s, stream, '\n')) == 0, "str_get_line(2): %s", strerror(err));
	TEST(str_eq(s, Lit("456\n")));
	TESTF((err = str_get_line(&s, stream, '\n')) == 0, "str_get_line(3): %s", strerror(err));
	TEST(str_eq(s, Lit("789")));
	TESTF((err = str_get_line(&s, stream, '\n')) == -1, "str_get_line(4): %s", strerror(err));

	fclose(stream);
}
