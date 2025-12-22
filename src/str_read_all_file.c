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

// max. size of the file str_read_all_file function can read
#ifndef STR_MAX_FILE_SIZE
	#define STR_MAX_FILE_SIZE	(64 * 1024 * 1024 - 1)
#endif

#define _POSIX_C_SOURCE 200809L

#include "str_impl.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int str_read_all_file(str* const dest, const char* const file_name) {
	// open the file
	int fd, err;

	while((fd = open(file_name, O_RDONLY | O_CLOEXEC)) < 0)
		if((err = errno) != EINTR)
			return err;

	// stat the file
	struct stat info;

	if(fstat(fd, &info) < 0) {
		err = errno;
		close(fd);
		return err;
	}

	// check what we've got
	if(!S_ISREG(info.st_mode)) {
		close(fd);
		return S_ISDIR(info.st_mode) ? EISDIR : ENOTSUP;
	}

	if(info.st_size == 0) { // empty file
		if(close(fd) < 0)
			return errno;

		str_assign(dest, STR_NULL);
		return 0;
	}

	if(info.st_size > STR_MAX_FILE_SIZE) {
		close(fd);
		return EFBIG;
	}

	// allocate buffer
	char* buff = mem_alloc(info.st_size + 1); // +1 for null-terminator

	// read the file
	const char* const end = buff + info.st_size;
	char* p = buff;
	ssize_t n;

	do {
		while((n = read(fd, p, end - p)) < 0) {
			if((err = errno) != EINTR) {
				close(fd);
				free(buff);
				return err;
			}
		}
	} while(n > 0 && (p += n) < end);

	// close the file
	if(close(fd) < 0) {
		free(buff);
		return errno;
	}

	// null-terminate
	*p = 0;

	// realloc if we have read less bytes than expected
	if(p > buff && p < end)
		buff = mem_realloc(buff, p - buff + 1);

	// assign result
	str_assign(dest, str_acquire_mem(buff, p - buff));
	return 0;
}
