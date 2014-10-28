#include <vigor.h>
#include "impl.h"

/*

     ######  ######## ########  ########
    ##    ## ##       ##     ##    ##
    ##       ##       ##     ##    ##
    ##       ######   ########     ##
    ##       ##       ##   ##      ##
    ##    ## ##       ##    ##     ##
     ######  ######## ##     ##    ##

 */

static int VIGOR_CERT_KEYSIZE[2] = { 32, 64 };

cert_t* cert_new(int type)
{
	if (type != VIGOR_CERT_ENCRYPTION
	 && type != VIGOR_CERT_SIGNING)
		return NULL;

	cert_t *key = vmalloc(sizeof(cert_t));
	key->type = type;
	return key;
}

cert_t* cert_make(int type, const char *pub, const char *sec)
{
	cert_t *key = cert_new(type);
	assert(key);

	if (pub) {
		if (strlen(pub) != 64) goto bail_out;

		strncpy(key->pubkey_b16, pub, 64);
		key->pubkey = 1;
	}

	if (sec) {
		int ksz = VIGOR_CERT_KEYSIZE[key->type];
		if (strlen(sec) != ksz * 2) goto bail_out;

		strncpy(key->seckey_b16, sec, ksz * 2);
		key->seckey = 1;
	}

	if (!key->pubkey) goto bail_out;
	if (cert_rescan(key) != 0) goto bail_out;
	return key;

bail_out:
	errno = EINVAL;
	cert_free(key);
	return NULL;
}

cert_t* cert_generate(int type)
{
	cert_t *key = cert_new(type);
	assert(key);

	int rc;

	if (type == VIGOR_CERT_ENCRYPTION) {
		rc = crypto_box_keypair(key->pubkey_bin, key->seckey_bin);
		assert(rc == 0);
	} else {
		rc = crypto_sign_keypair(key->pubkey_bin, key->seckey_bin);
		assert(rc == 0);
	}

	rc = base16_encode(key->pubkey_b16, 64, key->pubkey_bin, 32);
	assert(rc == 64);
	key->pubkey = 1;

	int ksz = VIGOR_CERT_KEYSIZE[type];
	rc = base16_encode(key->seckey_b16, ksz * 2, key->seckey_bin, ksz);
	assert(rc == ksz * 2);
	key->seckey = 1;

	return key;
}

cert_t* cert_read(const char *path)
{
	assert(path);

	FILE *io = fopen(path, "r");
	if (!io) return NULL;

	cert_t *key = cert_readio(io);
	fclose(io);
	return key;
}

cert_t* cert_readio(FILE *io)
{
	assert(io);

	/* format:
	   -----------------------------------------------
	   [%(encryption|signing) v1]
	   id  fqdn
	   pub DECAFBAD...
	   sec DEADBEEF...
	   -----------------------------------------------
	 */

	list_t cfg;
	list_init(&cfg);

	int rc = config_read(&cfg, io);
	if (rc != 0) return NULL;

	cert_t *key;
	if (config_isset(&cfg, "\%signing")
	 && strcmp(config_get(&cfg, "\%signing"), "v1") == 0) {
		key = cert_new(VIGOR_CERT_SIGNING);

	} else if (config_isset(&cfg, "\%encryption")
	 && strcmp(config_get(&cfg, "\%encryption"), "v1") == 0) {
		key = cert_new(VIGOR_CERT_ENCRYPTION);

	} else {
		// legacy behavior
		key = cert_new(VIGOR_CERT_ENCRYPTION);
	}
	assert(key);

	if (config_isset(&cfg, "id")) {
		free(key->ident);
		key->ident = strdup(config_get(&cfg, "id"));
	}

	char *v;
	if (config_isset(&cfg, "pub")) {
		v = config_get(&cfg, "pub");
		if (strlen(v) != 64) goto bail_out;

		strncpy(key->pubkey_b16, v, 64);
		key->pubkey = 1;
	}

	if (config_isset(&cfg, "sec")) {
		int ksz = VIGOR_CERT_KEYSIZE[key->type];
		v = config_get(&cfg, "sec");
		if (strlen(v) != ksz * 2) goto bail_out;

		strncpy(key->seckey_b16, v, ksz * 2);
		key->seckey = 1;
	}

	if (!key->pubkey) goto bail_out;
	if (cert_rescan(key) != 0) goto bail_out;

	config_done(&cfg);
	return key;

bail_out:
	errno = EINVAL;
	config_done(&cfg);
	cert_free(key);
	return NULL;
}

int cert_write(cert_t *key, const char *path, int full)
{
	assert(key);
	assert(path);

	FILE *io = fopen(path, "w");
	if (!io) return -1;

	int rc = cert_writeio(key, io, full);
	fclose(io);
	return rc;
}

int cert_writeio(cert_t *key, FILE *io, int full)
{
	assert(key);
	assert(io);

	fprintf(io, "%%%s v1\n", key->type == VIGOR_CERT_ENCRYPTION
		                                ? "encryption" : "signing");

	if (key->ident)          fprintf(io, "id  %s\n", key->ident);
	if (key->pubkey)         fprintf(io, "pub %s\n", key->pubkey_b16);
	if (key->seckey && full) fprintf(io, "sec %s\n", key->seckey_b16);

	return 0;
}

void cert_free(cert_t *key)
{
	if (!key) return;
	free(key->ident);
	free(key);
}

uint8_t *cert_public(cert_t *key)
{
	assert(key);
	return key->pubkey_bin;
}

uint8_t *cert_secret(cert_t *key)
{
	assert(key);
	return key->seckey_bin;
}

char *cert_public_s(cert_t *key)
{
	assert(key);
	if (!key->pubkey) return NULL;
	return strdup(key->pubkey_b16);
}

char *cert_secret_s(cert_t *key)
{
	assert(key);
	if (!key->seckey) return NULL;
	return strdup(key->seckey_b16);
}

int cert_rescan(cert_t *key)
{
	assert(key);

	int rc;

	if (key->pubkey) {
		rc = base16_decode(key->pubkey_bin, 32, key->pubkey_b16, 64);
		if (rc != 32) return -1;
	}

	if (key->seckey) {
		int ksz = VIGOR_CERT_KEYSIZE[key->type];
		rc = base16_decode(key->seckey_bin, ksz, key->seckey_b16, ksz * 2);
		if (rc != ksz) return -1;
	}

	return 0;
}

int cert_encode(cert_t *key)
{
	assert(key);

	int rc;

	if (key->pubkey) {
		rc = base16_encode(key->pubkey_b16, 64, key->pubkey_bin, 32);
		if (rc != 64) return -1;
	}

	if (key->seckey) {
		int ksz = VIGOR_CERT_KEYSIZE[key->type];
		rc = base16_encode(key->seckey_b16, ksz * 2, key->seckey_bin, ksz);
		if (rc != ksz * 2) return -1;
	}

	return 0;
}

unsigned long long cert_seal(cert_t *k, const void *_u, unsigned long long ulen, uint8_t **s)
{
	const uint8_t *u = (const uint8_t *)_u;
	assert(k && k->seckey);
	assert(u);
	assert(ulen > 0);
	assert(s);

	long long unsigned slen = ulen + crypto_sign_BYTES;
	*s = vmalloc(slen);

	if (crypto_sign(*s, &slen, u, ulen, k->seckey_bin) == 0)
		return slen;
	free(*s); *s = NULL;
	return 0;
}

unsigned long long cert_unseal(cert_t *k, const void *_s, unsigned long long slen, uint8_t **u)
{
	const uint8_t *s = (const uint8_t *)_s;
	assert(k && k->pubkey);
	assert(s);
	assert(slen > crypto_sign_BYTES);
	assert(u);

	unsigned long long ulen = slen - crypto_sign_BYTES;
	*u = vmalloc(ulen);

	if (crypto_sign_open(*u, &ulen, s, slen, k->pubkey_bin) == 0)
		return ulen;
	free(*u); *u = NULL;
	return 0;
}

int cert_sealed(cert_t *k, const void *_s, unsigned long long slen)
{
	const uint8_t *s = (const uint8_t *)_s;
	assert(k && k->pubkey);
	assert(s);
	assert(slen > crypto_sign_BYTES);

	uint8_t *u;
	if (cert_unseal(k, s, slen, &u) > 0) {
		free(u);
		return 1;
	}
	return 0;
}

trustdb_t* trustdb_new(void)
{
	trustdb_t *ca = vmalloc(sizeof(trustdb_t));
	ca->verify = 1;
	list_init(&ca->certs);
	return ca;
}

trustdb_t* trustdb_read(const char *path)
{
	assert(path);

	FILE *io = fopen(path, "r");
	if (!io) return NULL;

	trustdb_t *ca = trustdb_readio(io);
	fclose(io);
	return ca;
}

trustdb_t* trustdb_readio(FILE *io)
{
	assert(io);

	trustdb_t *ca = trustdb_new();
	int rc = config_read(&ca->certs, io);
	if (rc != 0) {
		trustdb_free(ca);
		return NULL;
	}

	return ca;
}

int trustdb_write(trustdb_t *ca, const char *path)
{
	assert(ca);
	assert(path);

	FILE *io = fopen(path, "w");
	if (!io) return -1;

	int rc = trustdb_writeio(ca, io);
	fclose(io);
	return rc;
}

int trustdb_writeio(trustdb_t *ca, FILE *io)
{
	assert(ca);
	assert(io);

	keyval_t *kv;
	for_each_object(kv, &ca->certs, l)
		if (kv->val)
			fprintf(io, "%s %s\n", kv->key, kv->val);

	return 0;
}

void trustdb_free(trustdb_t *ca)
{
	if (!ca) return;
	config_done(&ca->certs);
	free(ca);
}


int trustdb_trust(trustdb_t *ca, cert_t *key)
{
	assert(ca);
	assert(key);
	assert(key->pubkey);

	config_set(&ca->certs, key->pubkey_b16, key->ident ? key->ident : "~");
	return 0;
}

int trustdb_revoke(trustdb_t *ca, cert_t *key)
{
	assert(ca);
	assert(key);
	assert(key->pubkey);

	config_unset(&ca->certs, key->pubkey_b16);
	return 0;
}

int trustdb_verify(trustdb_t *ca, cert_t *key, const char *ident)
{
	assert(ca);
	assert(key);
	assert(key->pubkey);

	if (!ca->verify) return 0;

	char *id = config_get(&ca->certs, key->pubkey_b16);
	if (!id) return 1;
	if (ident && strcmp(ident, id) != 0) return 1;
	return 0;
}
