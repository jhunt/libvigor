/*
  Copyright 2014 James Hunt <james@jameshunt.us>

  This file is part of libvigor.

  libvigor is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  libvigor is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with libvigor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vigor.h>
#include "impl.h"

/*

    ########     ###     ######  ########
    ##     ##   ## ##   ##    ## ##        ##   ##
    ##     ##  ##   ##  ##       ##         ## ##
    ########  ##     ##  ######  ######   #########
    ##     ## #########       ## ##         ## ##
    ##     ## ##     ## ##    ## ##        ##   ##
    ########  ##     ##  ######  ########

 */

static char    BASE16_ENCODE[16] = { "0123456789abcdef" };
static uint8_t BASE16_DECODE[256] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 000 (NUL) - 007 (BEL) */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 010 (BS)  - 017 (SI)  */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 020 (DLE) - 027 (ETB) */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 030 (CAN) - 037 (US)  */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 040 ( )   - 047 (')   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 050 (()   - 057 (/)   */

	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* '0' .. '7' */
	0x08, 0x09,                                     /* '8' .. '9' */

	            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 072 (:)   - 077 (?)   */
	0xff,                                           /* 100 (@)               */

	      0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,       /* 'A' .. 'F' */

	                                          0xff, /*           - 107 (G)   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 110 (H)   - 117 (O)   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 120 (P)   - 127 (W)   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 130 (X)   - 137 (_)   */
	0xff,                                           /* 140 (`)               */

	      0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,       /* 'a' .. 'f' */
	                                          0xff, /*           - 147 (g)   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 150 (h)   - 157 (o)   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 160 (p)   - 167 (w)   */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 170 (x)   - 177 (DEL) */

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 200 - 207 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 210 - 217 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 220 - 227 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 230 - 237 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 240 - 247 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 250 - 257 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 260 - 267 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 270 - 277 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 300 - 307 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 310 - 317 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 320 - 327 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 330 - 337 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 340 - 347 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 350 - 357 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 360 - 367 */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 370 - 377 */
};

int base16_encode(char *dst, size_t dlen, const void *src, size_t slen)
{
	/* empty string encodes as itself (no bytes) */
	if (slen == 0) return 0;

	assert(dst);
	assert(src);

	errno = EINVAL;
	if (dlen < slen * 2) return -1;

	char *d = dst;
	int i;
	for (i = 0; i < slen; i++) {
		*d++ = BASE16_ENCODE[((((uint8_t*)src)[i]) & 0xf0) >> 4];
		*d++ = BASE16_ENCODE[((((uint8_t*)src)[i]) & 0x0f)];
	}

	return d - dst;
}

int base16_decode(void *dst, size_t dlen, const char *src, size_t slen)
{
	/* empty string decodes as itself (no bytes) */
	if (slen == 0) return 0;

	assert(dst);
	assert(src);

	errno = EINVAL;
	assert(slen > 0);
	if (dlen < slen / 2) return -1;

	void *d = dst;
	int i;
	errno = EILSEQ;
	for (i = 0; i < slen; i += 2) {
		uint8_t hi, lo;
		hi = BASE16_DECODE[(uint8_t)src[i  ]];
		lo = BASE16_DECODE[(uint8_t)src[i+1]];
		if (hi == 0xff || lo == 0xff) return -1;

		*(uint8_t*)d++ = (hi << 4) + lo;
	}

	return d - dst;
}

char* base16_encodestr(const void *src, size_t len)
{
	assert(src);
	errno = EINVAL;
	if (len <= 0) return NULL;

	size_t dlen = len * 2;
	char *dst = vmalloc(sizeof(char) * (dlen + 1));

	int rc = base16_encode(dst, dlen, src, len);
	if (rc < 0) {
		free(dst);
		return NULL;
	}

	dst[rc] = '\0';
	return dst;
}

char* base16_decodestr(const char *src, size_t len)
{
	assert(src);
	errno = EINVAL;
	if (len <= 0) return NULL;

	size_t dlen = len / 2;
	char *dst = vmalloc(sizeof(char) * (dlen + 1));

	int rc = base16_decode(dst, dlen, src, len);
	if (rc < 0) {
		free(dst);
		return NULL;
	}

	dst[rc] = '\0';
	return dst;
}
