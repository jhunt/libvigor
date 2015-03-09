/*
  Copyright 2015 James Hunt <james@jameshunt.us>

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

static void s_transform(uint32_t state[5], const uint8_t buffer[64]);

#define SHA1_FD_BUFSIZE 8192

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* blk0() and blk() perform the initial expand. */
/* I got the idea of expanding during the round function from SSLeay */
#ifdef WORDS_BIGENDIAN
#define blk0(i) block->l[i]
#else
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#endif
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */
static void s_transform(uint32_t state[5], const uint8_t buffer[64])
{
	uint32_t a, b, c, d, e;
	typedef union {
		uint8_t c[64];
		uint32_t l[16];
	} CHAR64LONG16;
	CHAR64LONG16* block;

	block = (CHAR64LONG16*)buffer;

	/* Copy sha1->state[] to working vars */
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	/* 4 rounds of 20 operations each. Loop unrolled. */
	R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
	R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
	R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
	R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
	R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
	R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
	R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
	R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
	R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
	R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
	R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
	R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
	R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
	R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
	R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
	R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
	R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
	R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
	R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
	R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

	/* Add the working vars back into sha1.state[] */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	/* Wipe variables */
	a = b = c = d = e = 0;
}


/* Initialize new sha1 */
void sha1_init(sha1_t* sha1)
{
	/* SHA1 initialization constants */
	sha1->state[0] = 0x67452301;
	sha1->state[1] = 0xEFCDAB89;
	sha1->state[2] = 0x98BADCFE;
	sha1->state[3] = 0x10325476;
	sha1->state[4] = 0xC3D2E1F0;
	sha1->count[0] = sha1->count[1] = 0;

	memset(sha1->raw, 0, SHA1_RAWLEN);
	memset(sha1->hex, 0, SHA1_HEXLEN);
}


/* Run your data through this. */
void sha1_update(sha1_t* sha1, const uint8_t* data, const size_t len)
{
	size_t i, j;

	j = (sha1->count[0] >> 3) & 63;
	if ((sha1->count[0] += len << 3) < (len << 3)) sha1->count[1]++;
	sha1->count[1] += (len >> 29);
	if ((j + len) > 63) {
		memcpy(&sha1->buffer[j], data, (i = 64-j));
		s_transform(sha1->state, sha1->buffer);
		for ( ; i + 63 < len; i += 64) {
			memcpy(sha1->buffer, data + i, 64);
			s_transform(sha1->state, sha1->buffer);
		}
		j = 0;
	}
	else i = 0;
	memcpy(&sha1->buffer[j], &data[i], len - i);
}


/* Add padding and calculate the message digest . */
void sha1_finish(sha1_t* sha1)
{
	uint32_t i;
	uint8_t  finalcount[8];
	char    *hex;

	for (i = 0; i < 8; i++)
		finalcount[i] = (unsigned char)((sha1->count[(i >= 4 ? 0 : 1)] >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */

	sha1_update(sha1, (uint8_t *)"\200", 1);
	while ((sha1->count[0] & 504) != 448)
		sha1_update(sha1, (uint8_t *)"\0", 1);

	sha1_update(sha1, finalcount, 8);  /* Should cause a s_transform() */

	for (i = 0; i < SHA1_RAWLEN; i++)
		sha1->raw[i] = (uint8_t)((sha1->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);

	/* calculate hex digest */
	hex = sha1->hex;
	for (i = 0; i < SHA1_RAWLEN; i++) {
		sprintf(hex,"%02x", sha1->raw[i]);
		hex += 2;
	}
	*hex = '\0';

	/* Wipe variables */
	i = 0;
	memset(sha1->buffer, 0, 64);
	memset(sha1->state, 0, 20);
	memset(sha1->count, 0, 8);
	memset(finalcount, 0, 8);
}

/**
  Initialize a SHA1 $checksum.

  This function ensures that the $sha1->$raw and $sha1->$hex
  are initialized to appropriate starting values.  Callers using
  helper methods like @sha1_file and @sha1_data do not need to
  call this function explicitly.

  If $hex is specified, then $checksum will be initialized with
  a starting checksum.
 */
void sha1_set(sha1_t *sha1, const char *hex)
{
	if (hex && strlen(hex) == SHA1_HEXLEN - 1) {
		memcpy(sha1->hex, hex, SHA1_HEXLEN - 1);
		sha1->hex[SHA1_HEXLEN - 1] = '\0';

		unsigned int i;
		char digit[3] = {0};
		char *endptr = NULL; /* for strtol */
		for (i = 0; i < SHA1_RAWLEN; i++) {
			digit[0] = *hex++; digit[1] = *hex++;
			sha1->raw[i] = strtol(digit, &endptr, 16);
			if (endptr && *endptr != '\0') {
				goto blank_init;
			}
		}
		return;
	}

blank_init:
	memset(sha1->raw, 0, SHA1_RAWLEN);
	memset(sha1->hex, 0, SHA1_HEXLEN);
}

/**
  Compare $a and $b for equivalence.

  This function uses `memcmp(3)`, so it can be used for
  sorting comparisons.  It does *not* handle NULL parameters.

  If $a and $b are the same checksum, returns 0.  Otherwise,
  returns non-zero.
 */
int sha1_cmp(const sha1_t *a, const sha1_t *b)
{
	return memcmp(a->raw, b->raw, SHA1_RAWLEN);
}

/**
  Calulate checksum of data read from $fd.

  Data will be read until $fd reaches EOF.

  The $sha1 parameter must be a user-supplied `sha1`
  structure.  This function will call @sha1_init on $sha1.

  On success, returns 0 and updates $sha1 accordingly.
  On failure, returns non-zero, and the contents of $sha1
  are undefined.
 */
int sha1_fd(sha1_t *sha1, int fd)
{
	char buf[SHA1_FD_BUFSIZE];
	ssize_t nread;

	sha1_init(sha1);
	while ((nread = read(fd, buf, SHA1_FD_BUFSIZE)) != 0) {
		sha1_update(sha1, (uint8_t*)buf, nread);
	}
	sha1_finish(sha1);

	return 0;
}

/**
  Calculate the SHA1 checksum of $path.

  $path will be opened in read-only mode and read in chunks
  to calculate the checksum.  If $path cannot be opened
  (permission denied, ENOENT, etc.) non-zero will be returned
  to indicate failure.

  The $sha1 parameter must be a user-supplied `sha1`
  structure.  This function will call @sha1_init on $sha1.

  On success, returns 0 and updates $sha1 accordingly.
  On failure, returns non-zero, and the contents of $sha1
  are undefined.
 */
int sha1_file(sha1_t *sha1, const char *path) {
	int fd;
	struct stat st;

	sha1_init(sha1);
	fd = open(path, O_RDONLY);

	if (fd < 0 || fstat(fd, &st) == -1) {
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		errno = EISDIR;
		return -1;
	}

	sha1_fd(sha1, fd);

	close(fd);
	return 0;
}

/**
  Calculate the SHA1 checksum of $data.

  $data will be treated as an array of $len 8-bit unsigned
  integers.  Character strings will be converted according to
  standard C locale rules.

  The $sha1 parameter must be a user-supplied `sha1`
  structure.  This function will call @sha1_init on $sha1.

  On success, returns 0 and updates $sha1 accordingly.
  On failure, returns non-zero, and the contents of $sha1
  are undefined.
 */
int sha1_data(sha1_t *sha1, const void *data, size_t len)
{
	sha1_init(sha1);
	sha1_update(sha1, (const uint8_t *)data, len);
	sha1_finish(sha1);

	return 0;
}

