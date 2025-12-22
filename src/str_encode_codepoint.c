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

size_t str_encode_codepoint(char* const p, const uint32_t cp) {
	switch(cp) {
	case 0 ... 0x7F:
		p[0] = (char)cp;
		return 1;

	case 0x80 ... 0x7FF:
		p[0] = (char)(((cp >> 6) & 0x1F) | 0xC0);
		p[1] = (char)(((cp >> 0) & 0x3F) | 0x80);
		return 2;

	case 0x0800 ... 0xD7FF:
	case 0xE000 ... 0xFFFF:
		p[0] = (char)(((cp >> 12) & 0x0F) | 0xE0);
		p[1] = (char)(((cp >>  6) & 0x3F) | 0x80);
		p[2] = (char)(((cp >>  0) & 0x3F) | 0x80);
		return 3;

	case 0x10000 ... 0x10FFFF:
		p[0] = (char)(((cp >> 18) & 0x07) | 0xF0);
		p[1] = (char)(((cp >> 12) & 0x3F) | 0x80);
		p[2] = (char)(((cp >>  6) & 0x3F) | 0x80);
		p[3] = (char)(((cp >>  0) & 0x3F) | 0x80);
		return 4;

	default:
		return 0;
	}
}
