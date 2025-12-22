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

size_t str_to_valid_utf8(str* const dest) {
	const char* p = str_ptr(*dest);
	const char* const end = str_end(*dest);

	if(p == end)
		return 0;

	str_builder sb;

	sb_init(&sb);

	size_t nrep = 0;
	const char* s = p;

	while(p < end) {
		const str_decode_result r = str_decode_utf8(p, end - p);

		if(r.status == STR_UTF8_OK) {
			p += r.num_bytes;
			continue;
		}

		sb_append_mem(&sb, s, p - s);
		sb_append(&sb, str_lit("\xEF\xBF\xBD")); // U+FFFD

		++nrep;

		p += (r.status == STR_UTF8_INCOMPLETE || r.num_bytes == 1)
		   ? r.num_bytes
		   : (r.num_bytes - 1);

		s = p;
	}

	sb_append_mem(&sb, s, end - s);

	if(nrep > 0)
		assign_sb(dest, &sb);

	return nrep;
}
