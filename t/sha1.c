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

#include "test.h"

#define FIPS1_IN "abc"
#define FIPS2_IN "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"

#define FIPS1_OUT "a9993e36" "4706816a" "ba3e2571" "7850c26c" "9cd0d89d"
#define FIPS2_OUT "84983e44" "1c3bd26e" "baae4aa1" "f95129e5" "e54670f1"
#define FIPS3_OUT "34aa973c" "d4c4daa4" "f61eeb2b" "dbad2731" "6534016f"

TESTS {
	sha1_t cksum, calc, init, sha1;
	int i;

	sha1_init(&cksum);
	sha1_data(&cksum, FIPS1_IN, strlen(FIPS1_IN));
	is_string(cksum.hex, FIPS1_OUT, "FIPS Pub 180-1 test vector #1");

	sha1_init(&cksum);
	sha1_data(&cksum, FIPS2_IN, strlen(FIPS2_IN));
	is_string(cksum.hex, FIPS2_OUT, "FIPS Pub 180-1 test vector #2");

	/* it's hard to define a string constant for 1 mil 'a's... */
	sha1_init(&cksum);
	for (i = 0; i < 1000000; ++i) {
		sha1_update(&cksum, (unsigned char *)"a", 1);
	}
	sha1_finish(&cksum);
	is_string(cksum.hex, FIPS3_OUT, "FIPS Pub 180-1 test vector #3");


	sha1_init(&calc);
	is_string(calc.hex, "", "initial blank checksum");

	/* Borrow from FIPS checks */
	sha1_data(&calc, "this is a test", strlen("this is a test"));
	sha1_set(&init, calc.hex);
	is_string(calc.hex, init.hex, "init.hex == calc.hex");

	ok(memcmp(init.raw, calc.raw, SHA1_RAWLEN) == 0, "raw digests are equiv.");

	sha1_set(&calc, "0123456789abcd0123456789abcd0123456789ab");
	isnt_string(calc.hex, "", "valid checksum is not empty string");

	sha1_set(&calc, "0123456789abcd0123456789");
	is_string(calc.hex, "", "invalid initial checksum (too short)");

	sha1_set(&calc, "0123456789abcd0123456789abcdefghijklmnop");
	is_string(calc.hex, "", "invalid initial checksum (non-hex digits)");


	sha1_t a, b;
	const char *s1 = "This is the FIRST string";
	const char *s2 = "This is the SECOND string";

	sha1_init(&a);
	sha1_init(&b);

	sha1_data(&a, s1, strlen(s1));
	sha1_data(&b, s2, strlen(s2));

	ok(sha1_cmp(&a, &a) == 0, "identical checksums are equal");
	ok(sha1_cmp(&a, &b) != 0, "different checksums are not equal");


	ok(sha1_file(&cksum, "/tmp")  < 0, "sha1_file on directory fails");
	ok(sha1_file(&cksum, "/nope") < 0, "sha1_file on non-existent file fails");

	FILE *io = fopen("t/tmp/out.file", "w");
	if (!io) BAIL_OUT("failed to open t/tmp/out.file for writing");

	fprintf(io, "Robot Haiku\n"
	            "-----------\n"
	            "Seven hundred ten\n"
	            "Seven hundred eleven\n"
	            "Seven hundred twelve\n");
	fclose(io);
	ok(sha1_file(&cksum, "t/tmp/out.file") == 0, "sha1_file succeeds");
	is_string(cksum.hex, "e958bd59716ff22c0541497284849faae94acbda",
			"calculated sha1 of test file");
	unlink("t/tmp/out.file");


	/* make sure the original buffer is intact */
	char buf[128] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
	                "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\0";
	sha1_data(&sha1, buf, 128);
	is_string(buf,  "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
	                "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde\0",
		"sha1_data leaves the original buffer intact");

	done_testing();
}
