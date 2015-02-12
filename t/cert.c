/*
  Copyright 2011-2014 James Hunt <james@jameshunt.us>

  This file is part of Clockwork.

  Clockwork is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Clockwork is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Clockwork.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test.h"

#define EMPTY_BIN_KEY "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
#define EMPTY_B16_KEY "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"

TESTS {
	alarm(5);
	subtest { /* failure on instantiation */
		is_null(cert_new(42), "Fail to create a non-ENCRPYTION, non-SIGNING cert");
	}

	subtest { /* basic key generation */
		cert_t *key = cert_new(VIGOR_CERT_ENCRYPTION);
		ok(key->type == VIGOR_CERT_ENCRYPTION, "newly-created key is an encryption key");
		ok(!key->pubkey, "newly-created key has no public key");
		ok(!key->seckey, "newly-created key has no secret key");
		ok(memcmp(key->pubkey_b16, EMPTY_B16_KEY, 64) == 0,
			"public key (base16) is all zeros on creation");
		ok(memcmp(key->pubkey_bin, EMPTY_BIN_KEY, 32) == 0,
			"public key (binary) is all zeros on creation");
		ok(memcmp(key->seckey_b16, EMPTY_B16_KEY, 64) == 0,
			"secret key (base16) is all zeros on creation");
		ok(memcmp(key->seckey_bin, EMPTY_BIN_KEY, 32) == 0,
			"secret key (binary) is all zeros on creation");

		cert_free(key);
		key = cert_generate(VIGOR_CERT_ENCRYPTION);
		isnt_null(key, "cert_generate() returned a pointer");

		ok(key->type == VIGOR_CERT_ENCRYPTION, "newly-generated key is an encryption key");
		ok(key->pubkey, "newly-generated key has a public key");
		ok(key->seckey, "newly-generated key has a secret key");
		ok(memcmp(key->pubkey_b16, EMPTY_B16_KEY, 64) != 0,
			"public key (base16) is not all zeros");
		ok(memcmp(key->pubkey_bin, EMPTY_BIN_KEY, 32) != 0,
			"public key (binary) is not all zeros");
		ok(memcmp(key->seckey_b16, EMPTY_B16_KEY, 64) != 0,
			"secret key (base16) is not all zeros");
		ok(memcmp(key->seckey_bin, EMPTY_BIN_KEY, 32) != 0,
			"secret key (binary) is not all zeros");

		cert_t *other = cert_generate(VIGOR_CERT_ENCRYPTION);
		ok(key->type == VIGOR_CERT_ENCRYPTION, "newly-generated key is an encryption key");
		ok(memcmp(key->pubkey_bin, other->pubkey_bin, 32) != 0,
			"cert_generate() generates unique public keys");
		ok(memcmp(key->seckey_bin, other->pubkey_bin, 32) != 0,
			"cert_generate() generates unique secret keys");

		cert_free(key);
		cert_free(other);

		key = cert_make(VIGOR_CERT_SIGNING,
				"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
				NULL);
		isnt_null(key, "cert_make() returned a key");
		ok(key->type == VIGOR_CERT_SIGNING, "cert_make() gave us a signing key");
		ok(key->pubkey, "cert_make() set the public key");
		ok(memcmp(key->pubkey_bin,
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"cert_make() populated the public key (binary) properly");
		is_string(key->pubkey_b16,
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"cert_make() populated the public key (base16) properly");
		ok(!key->seckey, "no secret key given (NULL arg)");

		cert_free(key);
		key = cert_make(VIGOR_CERT_SIGNING,
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516"
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516");
		isnt_null(key, "cert_make() returned a key");
		ok(key->type == VIGOR_CERT_SIGNING, "cert_make() gave us a signing key");
		ok(key->pubkey, "cert_make() set the public key");
		ok(memcmp(key->pubkey_bin,
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"cert_make() populated the public key (binary) properly");
		is_string(key->pubkey_b16,
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"cert_make() populated the public key (base16) properly");
		ok(key->seckey, "cert_make() set the secret key");
		ok(memcmp(key->seckey_bin,
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
			"\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16"
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
			"\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16", 64) == 0,
			"cert_make() populated the secret key (binary) properly");
		is_string(key->seckey_b16,
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516"
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516",
			"cert_make() populated the secret key (base16) properly");
		cert_free(key);

		key = cert_make(VIGOR_CERT_SIGNING, NULL, NULL);
		is_null(key, "cert_make() requires a pubkey");
		key = cert_make(VIGOR_CERT_SIGNING, "pubkey", NULL);
		is_null(key, "cert_make() requires pubkey to be 64 characters long");
		key = cert_make(VIGOR_CERT_ENCRYPTION, EMPTY_B16_KEY, "test");
		is_null(key, "cert_make(ENC) requires seckey to be 64 characters long");
		key = cert_make(VIGOR_CERT_SIGNING, EMPTY_B16_KEY, EMPTY_BIN_KEY);
		is_null(key, "cert_make(SIG) requires seckey to be 128 characters long");
	}

	subtest { /* read in (combined pub+sec) */
		cert_t *key;

		key = cert_read("/dev/null");
		is_null(key, "Can't read a key from /dev/null");

		key = cert_read("/path/to/enoent");
		is_null(key, "Can't read a key from a nonexistent file");

		key = cert_read(TEST_DATA "/certs/combined.crt");
		isnt_null(key, "Read combined certificate from certs/combined.crt");
		ok(key->type == VIGOR_CERT_ENCRYPTION, "combined.crt is an encryption key");
		ok(key->pubkey, "combined.crt contains a public key");
		ok(key->seckey, "combined.crt contains a secret key");

		is_string(key->pubkey_b16,
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"Read the correct public key from combined.crt");
		ok(memcmp(key->pubkey_bin,
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"Read the correct public key (binary) from combined.crt");

		char *s;
		is_string(s = cert_public_s(key),
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"Read the correct public key from combined.crt (via cert_public_s)");
		free(s);
		ok(memcmp(cert_public(key),
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"Read the correct public key (binary) from combined.crt (via cw_curvkey_public)");

		is_string(key->seckey_b16,
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516",
			"Read the correct secret key from combined.crt");
		ok(memcmp(key->seckey_bin,
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
			"\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16", 32) == 0,
			"Read the correct secret key (binary) from combined.crt");

		is_string(s = cert_secret_s(key),
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516",
			"Read the correct secret key from combined.crt (via cert_secret_s)");
		free(s);
		ok(memcmp(cert_secret(key),
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
			"\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16", 32) == 0,
			"Read the correct secret key (binary) from combined.crt (via cert_secret)");

		is_string(key->ident, "some.host.some.where", "Read identity from combined.crt");

		cert_free(key);
	}

	subtest { /* read in (pub) */
		cert_t *key = cert_read(TEST_DATA "/certs/public.crt");
		isnt_null(key, "Read public certificate from certs/public.crt");
		ok(key->type == VIGOR_CERT_ENCRYPTION, "public.crt is an encryption key");
		ok( key->pubkey, "public.crt contains a public key");
		ok(!key->seckey, "public.crt does not contain a secret key");

		is_string(key->pubkey_b16, "0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"Read the correct public key from public.crt");
		ok(memcmp(key->pubkey_bin,
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"Read the correct public key (binary) from public.crt");
		is_string(key->ident, "some.host.some.where", "Read identity from public.crt");

		cert_free(key);
	}

	subtest { /* write out (full) */
		cert_t *key = cert_generate(VIGOR_CERT_ENCRYPTION);
		key->ident = strdup("a.new.host.to.test");

		isnt_int(cert_write(key, "/root/permdenied", 1), 0,
			"cert_write() fails on EPERM");
		isnt_int(cert_write(key, "/path/to/enoent", 1), 0,
			"cert_write() fails on bad parent path");

		is_int(cert_write(key, TEST_TMP "/newfull.crt", 1), 0,
			"wrote certificate (pub+sec) to newfull.crt");

		cert_t *other = cert_read(TEST_TMP "/newfull.crt");
		isnt_null(other, "read newly-written certificate (pub+sub)");
		is_string(other->pubkey_b16, key->pubkey_b16, "pubkey matches (write/reread)");
		is_string(other->seckey_b16, key->seckey_b16, "seckey matches (write/reread)");

		cert_free(key);
		cert_free(other);
	}

	subtest { /* write out (!full) */
		cert_t *key = cert_generate(VIGOR_CERT_ENCRYPTION);
		key->ident = strdup("a.new.host.to.test");

		isnt_int(cert_write(key, "/root/permdenied", 1), 0,
			"cert_write() fails on EPERM");
		isnt_int(cert_write(key, "/path/to/enoent", 1), 0,
			"cert_write() fails on bad parent path");

		is_int(cert_write(key, TEST_TMP "/newpub.crt", 0), 0,
			"wrote certificate (pub) to newpub.crt");

		cert_t *other = cert_read(TEST_TMP "/newpub.crt");
		isnt_null(other, "read newly-written certificate (pub)");
		is_string(other->pubkey_b16, key->pubkey_b16, "pubkey matches (write/reread)");
		isnt_string(other->seckey_b16, key->seckey_b16, "seckey does not match (write/reread)");

		cert_free(key);
		cert_free(other);
	}

	subtest { /* change keys * rescan */
		cert_t *key = cert_generate(VIGOR_CERT_ENCRYPTION);
		uint8_t oldpub[32], oldsec[32];

		memcpy(oldpub, key->pubkey_bin, 32);
		memcpy(oldsec, key->seckey_bin, 32);

		memset(key->pubkey_b16, '8', 64);
		memset(key->seckey_b16, 'e', 64);

		ok(memcmp(oldpub, key->pubkey_bin, 32) == 0, "old public key still in force (pre-rescan)");
		ok(memcmp(oldsec, key->seckey_bin, 32) == 0, "old secret key still in force (pre-rescan)");

		is_int(cert_rescan(key), 0, "recanned changed keypair");
		ok(memcmp(oldpub, key->pubkey_bin, 32) != 0, "old public key still no longer valid");
		ok(memcmp(oldsec, key->seckey_bin, 32) != 0, "old secret key still no longer valid");

		cert_free(key);
	}

	/************************************************************************/

	subtest { /* CA basics */
		trustdb_t *ca = trustdb_new();
		isnt_null(ca, "trustdb_new() returns a new CA pointer");
		ok(ca->verify, "By default, CA will verify certs");
		trustdb_free(ca);
	}

	subtest { /* verify modes */
		trustdb_t *ca = trustdb_new();
		assert(ca);

		cert_t *key = cert_generate(VIGOR_CERT_ENCRYPTION);
		assert(key);
		const char *fqdn = "original.host.name";
		const char *fail = "some.other.host.name";
		key->ident = strdup(fqdn);

		ca->verify = 0;
		ok(trustdb_verify(ca, key, NULL) == 0, "[verify=no] new key is verified (untrusted)");
		ok(trustdb_verify(ca, key, fqdn) == 0, "[verify=no] new key is verified, with ident check (untrusted)");
		ok(trustdb_verify(ca, key, fail) == 0, "[verify=no] new key is verified, failed ident check (untrusted)");
		ca->verify = 1;
		ok(trustdb_verify(ca, key, NULL) != 0, "[verify=yes] new key is not yet verified (untrusted)");
		ok(trustdb_verify(ca, key, fqdn) != 0, "[verify=yes] new key is not yet verified, with ident check (untrusted)");
		ok(trustdb_verify(ca, key, fail) != 0, "[verify=yes] new key is not yet verified, failed ident check (untrusted)");

		is_int(trustdb_trust(ca, key), 0, "trusted new key/cert");

		ca->verify = 0;
		ok(trustdb_verify(ca, key, NULL) == 0, "[verify=no] new key is verified (trusted)");
		ok(trustdb_verify(ca, key, fqdn) == 0, "[verify=no] new key is verified, with ident check (trusted)");
		ok(trustdb_verify(ca, key, fail) == 0, "[verify=no] new key is verified, failed ident check (trusted)");
		ca->verify = 1;
		ok(trustdb_verify(ca, key, NULL) == 0, "[verify=yes] new key is verified (trusted)");
		ok(trustdb_verify(ca, key, fqdn) == 0, "[verify=yes] new key is verified, with ident check (trusted)");
		ok(trustdb_verify(ca, key, fail) != 0, "[verify=yes] new key is not verified, failed ident check (trusted)");

		is_int(trustdb_revoke(ca, key), 0, "revoked key/cert");

		ca->verify = 0;
		ok(trustdb_verify(ca, key, NULL) == 0, "[verify=no] new key is verified (revoked)");
		ok(trustdb_verify(ca, key, fqdn) == 0, "[verify=no] new key is verified, with ident check (revoked)");
		ok(trustdb_verify(ca, key, fail) == 0, "[verify=no] new key is verified, failed ident check (revoked)");
		ca->verify = 1;
		ok(trustdb_verify(ca, key, NULL) != 0, "[verify=yes] new key is not verified (revoked)");
		ok(trustdb_verify(ca, key, fqdn) != 0, "[verify=yes] new key is not verified, with ident check (revoked)");
		ok(trustdb_verify(ca, key, fail) != 0, "[verify=yes] new key is not verified, failed ident check (revoked)");

		cert_free(key);
		trustdb_free(ca);
	}

	subtest { /* read CA list */
		trustdb_t *ca = trustdb_read(TEST_DATA "/certs/trusted");
		isnt_null(ca, "read certificate authority from file");

		cert_t *other = cert_generate(VIGOR_CERT_ENCRYPTION);
		isnt_null(other, "generated new (untrusted) key");

		cert_t *key = cert_read(TEST_DATA "/certs/combined.crt");
		isnt_null(key, "read key from combined.crt");

		is_int(trustdb_verify(ca, key, NULL), 0, "key for some.host.some.where is verified");
		isnt_int(trustdb_verify(ca, other, NULL), 0, "new key not trusted");

		trustdb_free(ca);
		cert_free(other);
		cert_free(key);
	}

	subtest { /* write CA list */
		trustdb_t *ca = trustdb_read(TEST_DATA "/certs/trusted");
		isnt_null(ca, "read certificate authority from file");

		cert_t *other = cert_generate(VIGOR_CERT_ENCRYPTION);
		isnt_null(other, "generated new (untrusted) key");
		other->ident = strdup("xyz.host");

		cert_t *key = cert_read(TEST_DATA "/certs/combined.crt");
		isnt_null(key, "read key from combined.crt");

		trustdb_revoke(ca, key);
		trustdb_trust(ca, other);

		is_int(trustdb_write(ca, TEST_TMP "/trusted"), 0,
			"wrote certificate authority to " TEST_TMP "/trusted");

		trustdb_free(ca);
		ca = trustdb_read(TEST_TMP "/trusted");
		isnt_null(ca, "re-read certificate authority");

		isnt_int(trustdb_verify(ca, key, NULL), 0, "some.host.some.where not trusted by on-disk CA");
		is_int(trustdb_verify(ca, other, NULL), 0, "xyz.host trusted by on-disk CA");

		trustdb_free(ca);
		cert_free(key);
		cert_free(other);
	}

	/************************************************************************/

	subtest { /* basic key generation */
		cert_t *key = cert_new(VIGOR_CERT_SIGNING);
		ok(!key->pubkey, "newly-created key has no public key");
		ok(!key->seckey, "newly-created key has no secret key");
		ok(memcmp(key->pubkey_b16, EMPTY_B16_KEY, 64) == 0,
			"public key (base16) is all zeros on creation");
		ok(memcmp(key->pubkey_bin, EMPTY_BIN_KEY, 32) == 0,
			"public key (binary) is all zeros on creation");
		ok(memcmp(key->seckey_b16, EMPTY_B16_KEY EMPTY_B16_KEY, 128) == 0,
			"secret key (base16) is all zeros on creation");
		ok(memcmp(key->seckey_bin, EMPTY_BIN_KEY EMPTY_BIN_KEY, 64) == 0,
			"secret key (binary) is all zeros on creation");

		cert_free(key);
		key = cert_generate(VIGOR_CERT_SIGNING);
		isnt_null(key, "cert_generate() returned a pointer");

		ok(key->pubkey, "newly-generated key has a public key");
		ok(key->seckey, "newly-generated key has a secret key");
		ok(memcmp(key->pubkey_b16, EMPTY_B16_KEY, 64) != 0,
			"public key (base16) is not all zeros");
		ok(memcmp(key->pubkey_bin, EMPTY_BIN_KEY, 32) != 0,
			"public key (binary) is not all zeros");
		ok(memcmp(key->seckey_b16, EMPTY_B16_KEY EMPTY_B16_KEY, 128) != 0,
			"secret key (base16) is not all zeros");
		ok(memcmp(key->seckey_bin, EMPTY_BIN_KEY EMPTY_BIN_KEY, 64) != 0,
			"secret key (binary) is not all zeros");

		cert_t *other = cert_generate(VIGOR_CERT_SIGNING);
		ok(memcmp(key->pubkey_bin, other->pubkey_bin, 32) != 0,
			"cert_generate() generates unique public keys");
		ok(memcmp(key->seckey_bin, other->pubkey_bin, 32) != 0,
			"cert_generate() generates unique secret keys");

		cert_free(key);
		cert_free(other);
	}

	subtest { /* read in (combined pub+sec) */
		cert_t *key;

		key = cert_read("/dev/null");
		is_null(key, "Can't read a key from /dev/null");

		key = cert_read("/path/to/enoent");
		is_null(key, "Can't read a key from a nonexistent file");

		key = cert_read(TEST_DATA "/keys/combined.key");
		isnt_null(key, "Read combined key from keys/combined.key");
		ok(key->pubkey, "combined.key contains a public key");
		ok(key->seckey, "combined.key contains a secret key");

		is_string(key->pubkey_b16,
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"Read the correct public key from combined.key");
		ok(memcmp(key->pubkey_bin,
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"Read the correct public key (binary) from combined.key");

		char *s;
		is_string(s = cert_public_s(key),
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"Read the correct public key from combined.key (via cert_public_s)");
		free(s);
		ok(memcmp(cert_public(key),
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"Read the correct public key (binary) from combined.key (via cw_curvkey_public)");

		is_string(key->seckey_b16,
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516"
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516",
			"Read the correct secret key from combined.key");
		ok(memcmp(key->seckey_bin,
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
		    "\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16"
		    "\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
		    "\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16", 64) == 0,
			"Read the correct secret key (binary) from combined.key");

		is_string(s = cert_secret_s(key),
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516"
			"feb546ed444dddb23c9a3c7a72236cf32c90148649d4563d8264665f248db516",
			"Read the correct secret key from combined.key (via cert_secret_s)");
		free(s);
		ok(memcmp(cert_secret(key),
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
			"\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16"
			"\xfe\xb5\x46\xed\x44\x4d\xdd\xb2\x3c\x9a\x3c\x7a\x72\x23\x6c\xf3"
			"\x2c\x90\x14\x86\x49\xd4\x56\x3d\x82\x64\x66\x5f\x24\x8d\xb5\x16", 64) == 0,
			"Read the correct secret key (binary) from combined.key (via cert_secret)");

		is_string(key->ident, "some.host.some.where", "Read identity from combined.key");

		cert_free(key);
	}

	subtest { /* read in (pub) */
		cert_t *key = cert_read(TEST_DATA "/keys/public.key");
		isnt_null(key, "Read public key from keys/public.key");
		ok( key->pubkey, "public.key contains a public key");
		ok(!key->seckey, "public.key does not contain a secret key");

		is_string(key->pubkey_b16,
			"0394ba6c5dda02a38ad80ff80560179e8c52a9cb178d18d86490d04149ab3546",
			"Read the correct public key from public.key");
		ok(memcmp(key->pubkey_bin,
			"\x03\x94\xba\x6c\x5d\xda\x02\xa3\x8a\xd8\x0f\xf8\x05\x60\x17\x9e"
			"\x8c\x52\xa9\xcb\x17\x8d\x18\xd8\x64\x90\xd0\x41\x49\xab\x35\x46", 32) == 0,
			"Read the correct public key (binary) from public.key");
		is_string(key->ident, "some.host.some.where", "Read identity from public.key");

		cert_free(key);
	}

	subtest { /* write out (full) */
		cert_t *key = cert_generate(VIGOR_CERT_SIGNING);
		key->ident = strdup("jhacker");

		isnt_int(cert_write(key, "/root/permdenied", 1), 0,
			"cert_write() fails on EPERM");
		isnt_int(cert_write(key, "/path/to/enoent", 1), 0,
			"cert_write() fails on bad parent path");

		is_int(cert_write(key, TEST_TMP "/newfull.key", 1), 0,
			"wrote key (pub+sec) to newfull.key");

		cert_t *other = cert_read(TEST_TMP "/newfull.key");
		isnt_null(other, "read newly-written key (pub+sub)");
		is_string(other->pubkey_b16, key->pubkey_b16, "pubkey matches (write/reread)");
		is_string(other->seckey_b16, key->seckey_b16, "seckey matches (write/reread)");

		cert_free(key);
		cert_free(other);
	}

	subtest { /* write out (!full) */
		cert_t *key = cert_generate(VIGOR_CERT_SIGNING);
		key->ident = strdup("a.new.host.to.test");

		isnt_int(cert_write(key, "/root/permdenied", 1), 0,
			"cert_write() fails on EPERM");
		isnt_int(cert_write(key, "/path/to/enoent", 1), 0,
			"cert_write() fails on bad parent path");

		is_int(cert_write(key, TEST_TMP "/newpub.key", 0), 0,
			"wrote key (pub) to newpub.key");

		cert_t *other = cert_read(TEST_TMP "/newpub.key");
		isnt_null(other, "read newly-written key (pub)");
		is_string(other->pubkey_b16, key->pubkey_b16, "pubkey matches (write/reread)");
		isnt_string(other->seckey_b16, key->seckey_b16, "seckey does not match (write/reread)");

		cert_free(key);
		cert_free(other);
	}

	subtest { /* change keys * rescan */
		cert_t *key = cert_generate(VIGOR_CERT_SIGNING);
		uint8_t oldpub[32], oldsec[32];

		memcpy(oldpub, key->pubkey_bin, 32);
		memcpy(oldsec, key->seckey_bin, 32);

		memset(key->pubkey_b16, '8', 64);
		memset(key->seckey_b16, 'e', 64);

		ok(memcmp(oldpub, key->pubkey_bin, 32) == 0, "old public key still in force (pre-rescan)");
		ok(memcmp(oldsec, key->seckey_bin, 32) == 0, "old secret key still in force (pre-rescan)");

		is_int(cert_rescan(key), 0, "recanned changed keypair");
		ok(memcmp(oldpub, key->pubkey_bin, 32) != 0, "old public key still no longer valid");
		ok(memcmp(oldsec, key->seckey_bin, 32) != 0, "old secret key still no longer valid");

		cert_free(key);
	}

	subtest { /* message sealing - basic cases */
		cert_t *key = cert_generate(VIGOR_CERT_SIGNING);
		isnt_null(key, "Generated a new keypair for sealing messages");

		char *unsealed = strdup("this is my message;"
		                        "it is over 64 characters long, "
		                        "which is the minimum length of a sealed msg");
		unsigned long long ulen = 93;
		ok(!cert_sealed(key, unsealed, ulen), "raw message is not verifiable");

		uint8_t *sealed = NULL;
		unsigned long long slen = cert_seal(key, unsealed, ulen, &sealed);
		ok(slen > 0, "cert_seal returned a positive, non-zero buffer length");
		ok(slen > 64U, "sealed buffer is at least 64 bytes long");
		isnt_null(sealed, "cert_seal updated our pointer");
		ok(cert_sealed(key, sealed, slen), "Sealed message can be verified");

		free(unsealed);
		free(sealed);
		cert_free(key);
	}

	alarm(0);
	done_testing();
}
