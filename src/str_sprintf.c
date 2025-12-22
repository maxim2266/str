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

#include <stdio.h>
#include <stdarg.h>

#define SMALL_BUFF_SIZE	256

bool str_sprintf(str* const dest, const char* const fmt, ...) {
	va_list ap;

	// try small buffer first
	char small_buff[SMALL_BUFF_SIZE];

	va_start(ap, fmt);

	const int n = vsnprintf(small_buff, SMALL_BUFF_SIZE, fmt, ap);

	va_end(ap);

	if(n < 0)
		return false;

	if(n == 0) {
		str_assign(dest, STR_NULL);
		return true;
	}

	if(n < SMALL_BUFF_SIZE) {
		str_clone(dest, str_ref_mem(small_buff, n));
		return true;
	}

	// allocate and use a big buffer
	char* const big_buff = mem_alloc(n + 1);

	va_start(ap, fmt);
	str_assign(dest, str_acquire_mem(big_buff, vsnprintf(big_buff, n + 1, fmt, ap)));
	va_end(ap);

	return true;
}
