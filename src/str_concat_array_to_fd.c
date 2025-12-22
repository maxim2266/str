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

#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BATCH_SIZE 256

#define RETURN_ON_ERROR(fd) ({ \
	const int err = errno; \
	if(err != EINTR) { \
		close(fd); \
		return err; \
	} \
})

static
int write_one(const int fd, const char* p, const size_t count) {
	const char* const end = p + count;

	while(p < end) {
		ssize_t n;

		while((n = write(fd, p, end - p)) < 0)
			RETURN_ON_ERROR(fd);

		p += n;
	}

	return 0;
}

static
int write_batch(const int fd, struct iovec* pv, const size_t count) {
	const struct iovec* const end = pv + count;

	for(;;) {
		ssize_t n;

		// write
		while((n = writev(fd, pv, end - pv)) < 0)
			RETURN_ON_ERROR(fd);

		// scroll buffers
		for(; pv < end && n >= (ssize_t)pv->iov_len; ++pv)
			n -= (ssize_t)pv->iov_len;

		// check if anything is left to write
		if(pv == end)
			return 0;

		// update buffer
		pv->iov_base += n;
		pv->iov_len -= n;
	}
}

static
int write_vec(const int fd, const str* src, const size_t count) {
	struct iovec v[BATCH_SIZE];
	size_t n = 0;
	int err;

	for(size_t i = 0; i < count; ++i) {
		if(!str_is_empty(src[i])) {
			v[n] = (struct iovec){
				.iov_base = (void*)str_ptr(src[i]),
				.iov_len = str_len(src[i])
			};

			if(++n == BATCH_SIZE) {
				if((err = write_batch(fd, v, n)) != 0)
					return err;

				n = 0;
			}
		}
	}

	return (n > 0) ? write_batch(fd, v, n) : 0;
}

int str_concat_array_to_fd(const int fd, const str* src, const size_t count) {
	switch(count) {
	case 0:
		return 0;
	case 1:
		return write_one(fd, str_ptr(*src), str_len(*src));
	default:
		return write_vec(fd, src, count);
	}
}
