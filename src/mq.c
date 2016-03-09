/*
  Copyright 2016 James Hunt <james@jameshunt.us>

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

#define VIGOR_FRAME_FINAL      1
#define VIGOR_FRAME_PRINTABLE  2

typedef struct {
	list_t     l;
	zmq_msg_t  msg;
	uint8_t    flags;
} frame_t;

#define FRAME(f) ((frame_t*)(f))

#define s_frame_data(f) zmq_msg_data(&(FRAME(f))->msg)
#define s_frame_size(f) zmq_msg_size(&(FRAME(f))->msg)
#define s_frame_isfinal(f)     ((FRAME(f))->flags & VIGOR_FRAME_FINAL)
#define s_frame_isprintable(f) ((FRAME(f))->flags & VIGOR_FRAME_PRINTABLE)

static void s_frame_scan(frame_t *f)
{
	assert(f);

	const char *data = s_frame_data(f);
	size_t i;
	f->flags |= VIGOR_FRAME_PRINTABLE;
	for (i = 0; i < s_frame_size(f); i++) {
		if (isprint(data[i])) continue;
		f->flags &= ~VIGOR_FRAME_PRINTABLE;
		break;
	}
}

frame_t* s_frame_new(const void *buf, size_t len, uint8_t flags)
{
	assert(buf);
	assert(len >= 0);

	frame_t *f = vmalloc(sizeof(frame_t));
	list_init(&f->l);

	int rc = zmq_msg_init_size(&f->msg, len);
	if (rc != 0) {
		free(f);
		return NULL;
	}
	memcpy(s_frame_data(f), buf, len);

	f->flags = flags;
	s_frame_scan(f);

	return f;
}

void s_frame_free(frame_t *f)
{
	if (!f) return;
	list_delete(&f->l);
	zmq_msg_close(&f->msg);
	free(f);
}

static frame_t* s_frame_recv(void *zocket)
{
	assert(zocket);

	frame_t *f = vmalloc(sizeof(frame_t));
	list_init(&f->l);

	int rc = zmq_msg_init(&f->msg);
	assert(rc == 0);

	if (zmq_msg_recv(&f->msg, zocket, 0) == -1) {
		zmq_msg_close(&f->msg);
		free(f);
		return NULL;
	}

	int more;
	size_t len = sizeof(int);
	rc = zmq_getsockopt(zocket, ZMQ_RCVMORE, &more, &len);
	assert(rc == 0);

	if (more) f->flags &= ~VIGOR_FRAME_FINAL;
	else      f->flags |=  VIGOR_FRAME_FINAL;

	s_frame_scan(f);
	return f;
}

static void s_frame_fprint(frame_t *f, FILE *io)
{
	fprintf(io, "[% 5i] ", (int)s_frame_size(f));

	char *data = s_frame_data(f);
	size_t len = s_frame_size(f);
	size_t max = s_frame_isprintable(f) ? 70 : 35;
	const char *ellips = "";
	if (len > max) {
		len = max;
		ellips = "...";
	}

	size_t i;
	for (i = 0; i < len; i++)
		fprintf(io, (s_frame_isprintable(f) ? "%c" : "%02x"), (data[i]));
	fprintf(io, "%s%s\n", ellips, (s_frame_isfinal(f) ? "" : " (+)"));
}

static frame_t* s_pdu_frame(pdu_t *p, unsigned int i)
{
	assert(p);

	frame_t *f;
	for_each_object(f, &p->frames, l)
		if (i-- == 0) return f;
	return NULL;
}

typedef struct {
	pthread_t  tid;
	void      *socket;
	trustdb_t *tdb;
} zap_t;

/*

    ##     ##  #######
    ###   ### ##     ##
    #### #### ##     ##
    ## ### ## ##     ##
    ##     ## ##  ## ##
    ##     ## ##    ##
    ##     ##  ##### ##

*/

#define rnd(num) ((int)((float)(num) * random() / (RAND_MAX + 1.0)))
void* vzmq_ident(void *zocket, void *buf)
{
	char *id = (char*)buf;
	if (!id) {
		id = vmalloc(8);
		seed_randomness();
		id[0] = rnd(256); id[1] = rnd(256);
		id[2] = rnd(256); id[3] = rnd(256);
		id[4] = rnd(256); id[5] = rnd(256);
		id[6] = getpid() & 0xff;
		id[7] = getpid() >> 8;
	}
	zmq_setsockopt(zocket, ZMQ_IDENTITY, id, 8);
	return id;
}

void vzmq_shutdown(void *zocket, int linger)
{
	if (linger < 0) linger = 500;
	int rc = zmq_setsockopt(zocket, ZMQ_LINGER, &linger, sizeof(linger));
	if (rc != 0)
		logger(LOG_ERR, "failed to set ZMQ_LINGER to %i on socket %p: %s",
			linger, zocket, zmq_strerror(errno));

	zmq_close(zocket);
}

strings_t* vzmq_resolve(const char *endpoint, int af)
{
	strings_t *results = strings_new(NULL);

	if (strncmp(endpoint, "tcp://", 6) != 0) {
		strings_add(results, endpoint);
		return results;
	}

	char *copy = strdup(endpoint);
	char *a = copy + 6;
	char *b = strrchr(a, ':');
	if (!b) {
		strings_add(results, endpoint);
		return results;
	}
	*b++ = '\0';

	struct addrinfo hints, *info, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = af;
	hints.ai_socktype = SOCK_STREAM;

	int rc = getaddrinfo(a, NULL, &hints, &info);
	if (rc != 0) {
		logger(LOG_DEBUG, "Failed to lookup %s: %s", a, gai_strerror(rc));
		strings_add(results, endpoint);
		return results;
		free(copy);
	}

	for (p = info; p != NULL; p = p->ai_next) {
		int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd < 0) continue;
		close(fd);

		char addr[INET6_ADDRSTRLEN];
		if (p->ai_family == AF_INET
		 && !inet_ntop(AF_INET, &((struct sockaddr_in*)p->ai_addr)->sin_addr,
		               addr, sizeof(addr))) continue;

		if (p->ai_family == AF_INET6
		 && !inet_ntop(AF_INET6, &((struct sockaddr_in6*)p->ai_addr)->sin6_addr,
		               addr, sizeof(addr))) continue;

		char *s = string("tcp://%s:%s", addr, b);
		strings_add(results, s);
		free(s);
	}
	freeaddrinfo(info);
	free(copy);
	return results;
}

int vzmq_connect_af(void *z, const char *endpoint, int af)
{
	strings_t *names = vzmq_resolve(endpoint, af);
	if (names->num == 0) {
		errno = ENOENT;
		return 1;
	}

	int rc;
	unsigned int i;
	for (i = 0; i < names->num; i++) {
		logger(LOG_DEBUG, "trying endpoint %s (from %s)", names->strings[i], endpoint);
		rc = zmq_connect(z, names->strings[i]);
		if (rc == 0)
			break;
	}

	strings_free(names);
	return rc;
}

pdu_t* pdu_new(void)
{
	pdu_t *p = vmalloc(sizeof(pdu_t));
	list_init(&p->frames);

	return p;
}

pdu_t* pdu_make(const char *type, size_t n, ...)
{
	assert(type);
	assert(n >= 0);

	pdu_t *p = pdu_new();
	pdu_extend(p, type, strlen(type));

	va_list ap;
	va_start(ap, n);
	while (n-- > 0) {
		char *arg = va_arg(ap, char *);
		pdu_extend(p, arg, strlen(arg));
	}
	va_end(ap);

	return p;
}

pdu_t* pdu_reply(pdu_t *orig, const char *type, size_t n, ...)
{
	assert(orig);
	assert(type);
	assert(n >= 0);

	pdu_t *p = pdu_new();
	p->address = s_frame_new(s_frame_data(orig->address), s_frame_size(orig->address), 0);
	pdu_extend(p, type, strlen(type));

	va_list ap;
	va_start(ap, n);
	while (n-- > 0) {
		char *arg = va_arg(ap, char *);
		pdu_extend(p, arg, strlen(arg));
	}
	va_end(ap);

	return p;
}

pdu_t *pdu_dup(pdu_t *from, const char *type)
{
	assert(from);

	pdu_t *to = pdu_make(type ? type : pdu_type(from), 0);
	if (pdu_copy(to, from, 1, 0) != 0) {
		pdu_free(to);
		return NULL;
	}
	return to;
}

char* pdu_peer(pdu_t *p)
{
	if (!p->peer && p->address) {
		const char *data = s_frame_data(p->address);
		size_t len = s_frame_size(p->address);

		p->peer = vmalloc(2 * len + 1);
		if (base16_encode(p->peer, 2 * len, data, len) != 2 * len) {
			free(p->peer);
			p->peer = NULL;
		}
	}
	return p->peer;
}

char* pdu_type(pdu_t *p)
{
	if (!p->type)
		p->type = pdu_string(p, 0);
	return p->type;
}

int pdu_attn(pdu_t *p, const char *peer)
{
	if (p->peer)
		free(p->peer);

	p->peer = strdup(peer);
	size_t len = strlen(peer) / 2;

	char *addr = vmalloc(len);
	if (base16_decode(addr, len, peer, len * 2) != len) {
		free(addr);
		return 1;
	}

	s_frame_free(p->address);
	p->address = s_frame_new(addr, len, 0);
	free(addr);
	return 0;
}

void pdu_free(pdu_t *p)
{
	if (!p) return;

	s_frame_free(p->address);
	free(p->peer);
	free(p->type);

	frame_t *f, *f_tmp;
	for_each_object_safe(f, f_tmp, &p->frames, l)
		s_frame_free(f);

	free(p);
}

int pdu_copy(pdu_t *to, pdu_t *from, int start, int n)
{
	assert(from);
	assert(start >= 0);
	assert(n >= 0);

	int i, end = n ? start + n : pdu_size(from);
	for (i = start; i < pdu_size(from) && i < end; i++) {
		size_t len;
		uint8_t *buf = pdu_segment(from, i, &len);
		pdu_extend(to, buf, len);
		free(buf);
	}
	return 0;
}

static void s_pdu_extend(pdu_t *p, frame_t *f)
{
	assert(p);
	assert(f);

	frame_t *last;
	if (p->len > 0) {
		last = list_tail(&p->frames, frame_t, l);
		last->flags &= ~VIGOR_FRAME_FINAL;
	}

	f->flags |= VIGOR_FRAME_FINAL;
	list_push(&p->frames, &f->l);
	p->len++;
}

int pdu_extend (pdu_t *p, const void *buf, size_t len)
{
	assert(p);
	assert(buf);
	assert(len >= 0);

	frame_t *f = s_frame_new(buf, len, 0);
	s_pdu_extend(p, f);

	return 0;
}

int pdu_extendf(pdu_t *p, const char *fmt, ...)
{
	va_list ap1, ap2;
	va_start(ap1, fmt);
	va_copy(ap2, ap1);
	size_t n = vsnprintf(NULL, 0, fmt, ap1);
	va_end(ap1);

	char *s = vcalloc(n + 1, sizeof(char));
	vsnprintf(s, n + 1, fmt, ap2);
	va_end(ap2);

	int rc = pdu_extend(p, s, n + 1);
	free(s);
	return rc;
}

uint8_t* pdu_segment(pdu_t *p, unsigned int i, size_t *len)
{
	assert(len);

	frame_t *f = s_pdu_frame(p, i);
	if (!f) return NULL;

	*len = s_frame_size(f);
	uint8_t *dst = vmalloc(*len);
	memcpy(dst, s_frame_data(f), *len);
	return dst;
}

char* pdu_string(pdu_t *p, unsigned int i)
{
	assert(p);

	frame_t *f = s_pdu_frame(p, i);
	if (!f) return NULL;

	char *s = vcalloc(s_frame_size(f) + 1, sizeof(char));
	memcpy(s, s_frame_data(f), s_frame_size(f));
	return s;
}

#define s_frame_sendm(f,zocket) \
	zmq_msg_send(&(f)->msg, (zocket), ZMQ_SNDMORE)

#define s_frame_send(f,zocket) \
	zmq_msg_send(&(f)->msg, (zocket), s_frame_isfinal(f) ? 0 : ZMQ_SNDMORE)

static int _zmq_socktype(void *z)
{
	int v, rc;
	size_t len = sizeof(v);
	rc = zmq_getsockopt(z, ZMQ_TYPE, &v, &len);
	return rc ? rc : v;
}

int pdu_send(pdu_t *p, void *zocket)
{
	assert(p);
	assert(zocket);

	int rc;

	if (_zmq_socktype(zocket) != ZMQ_PUB) {
		if (p->address) {
			rc = s_frame_sendm(FRAME(p->address), zocket);
			if (rc < 0) return rc;
		}

		frame_t *blank = s_frame_new("", 0, 0);
		rc = s_frame_sendm(blank, zocket);
		s_frame_free(blank);
		if (rc < 0) return rc;
	}

	frame_t *f;
	for_each_object(f, &p->frames, l) {
		rc = s_frame_send(f, zocket);
		if (rc < 0) return rc;
	}

	return 0;
}

int pdu_send_and_free(pdu_t *p, void *zocket)
{
	assert(p);
	assert(zocket);

	int rc = pdu_send(p, zocket);
	pdu_free(p);
	return rc;
}

pdu_t* pdu_recv(void *zocket)
{
	pdu_t *p = pdu_new();

	int body = 0, fin = 0;
	frame_t *f;

	if (_zmq_socktype(zocket) == ZMQ_SUB)
		body = 1; /* skip the empty addressing frame */

	for (;;) {
		f = s_frame_recv(zocket);
		if (!f) {
			pdu_free(p);
			return NULL;
		}
		fin = s_frame_isfinal(f);

		if (!body) {
			if (s_frame_size(f) == 0) {
				body = 1;
				s_frame_free(f);

			} else {
				assert(!p->address);
				p->address = f;
			}
		} else {
			s_pdu_extend(p, f);
		}

		if (fin) break;
	}

	return p;
}

void pdu_fprint(pdu_t *p, FILE *io)
{
	if (p->address) {
		s_frame_fprint(FRAME(p->address), io);
		fprintf(io, "---\n");
	}
	frame_t *f;
	for_each_object(f, &p->frames, l)
		s_frame_fprint(f, io);
	fprintf(io, "END\n");
}

int pdu_to_hash(pdu_t *p, hash_t *h, int offset)
{
	assert(p);
	assert(h);

	if ((pdu_size(p) - offset) % 2 != 0)
		return 1;

	int i;
	for (i = offset; i < pdu_size(p); i += 2) {
		char *k = pdu_string(p, i);
		char *v = pdu_string(p, i + 1);

		hash_set(h, k, v);
		free(k);
	}
	return 0;
}

int pdu_from_hash(pdu_t *p, hash_t *h)
{
	assert(p);
	assert(h);

	char *k, *v;
	for_each_key_value(h, k, v) {
		pdu_extend(p, k, strlen(k));
		pdu_extend(p, v, strlen(v));
	}
	return 0;
}

static char *s_zap_recv(void *socket)
{
	char buf[256];
	int n = zmq_recv(socket, buf, 255, 0);
	if (n < 0) return NULL;
	if (n > 255) n = 255;
	buf[n] = '\0';
	return strdup(buf);
}
static int s_zap_send(void *socket, const char *s) {
	return zmq_send(socket, s, strlen(s), 0);
}
static int s_zap_sendmore(void *socket, const char *s) {
	return zmq_send(socket, s, strlen(s), ZMQ_SNDMORE);
}
static void* s_zap_thread(void *u)
{
	assert(u);
	zap_t *zap = (zap_t*)u;

	logger(LOG_INFO, "zap: authentication thread starting up");

	for (;;) {
		logger(LOG_DEBUG, "zap: awaiting auth packet");
		char *version = s_zap_recv(zap->socket);
		logger(LOG_DEBUG, "zap: inbound auth packet!");
		if (!version) break;

		char *sequence  = s_zap_recv(zap->socket);
		char *domain    = s_zap_recv(zap->socket);
		char *address   = s_zap_recv(zap->socket);
		char *identity  = s_zap_recv(zap->socket);
		char *mechanism = s_zap_recv(zap->socket);

		logger(LOG_DEBUG, "zap: received frame:   version  = %s", version);
		logger(LOG_DEBUG, "zap: received frame:   sequence = %s", sequence);
		logger(LOG_DEBUG, "zap: received frame:   domain   = %s", domain);
		logger(LOG_DEBUG, "zap: received frame:   address  = %s", address);
		logger(LOG_DEBUG, "zap: received frame:   identity = %s", identity);

		cert_t *key = cert_new(VIGOR_CERT_ENCRYPTION);
		assert(key);
		key->pubkey = 1;
		int n = zmq_recv(zap->socket, key->pubkey_bin, 32, 0);
		if (n != 32) goto bail_out;
		if (strcmp(version,   "1.0")   != 0) goto bail_out;
		if (strcmp(mechanism, "CURVE") != 0) goto bail_out;

		logger(LOG_DEBUG, "zap: verified message structure");

		cert_encode(key);
		logger(LOG_DEBUG, "zap: checking public key [%s]", key->pubkey_b16);

		s_zap_sendmore(zap->socket, version);
		s_zap_sendmore(zap->socket, sequence);

		if (!zap->tdb || trustdb_verify(zap->tdb, key, NULL) == 0) {
			logger(LOG_DEBUG, "zap: granting authentication request - 200 OK");
			s_zap_sendmore(zap->socket, "200");
			s_zap_sendmore(zap->socket, "OK");
			s_zap_sendmore(zap->socket, "anonymous");
			s_zap_send    (zap->socket, "");
		} else {
			logger(LOG_DEBUG, "zap: rejecting authentication request - 400 Untrusted");
			s_zap_sendmore(zap->socket, "400");
			s_zap_sendmore(zap->socket, "Untrusted client public key");
			s_zap_sendmore(zap->socket, "");
			s_zap_send    (zap->socket, "");
		}

		free(version);
		free(sequence);
		free(domain);
		free(address);
		free(identity);
		free(mechanism);
		cert_free(key);

		continue;
bail_out:
		logger(LOG_WARNING, "zap: denying curve authentication");
		free(version);
		free(sequence);
		free(domain);
		free(address);
		free(identity);
		free(mechanism);
		cert_free(key);
		break;
	}
	zmq_close(zap->socket);
	return zap;
}
void* zap_startup(void *zctx, trustdb_t *tdb)
{
	assert(zctx);

	zap_t *handle = vmalloc(sizeof(zap_t));
	handle->tdb = tdb;

	handle->socket = zmq_socket(zctx, ZMQ_REP);
	assert(handle->socket);

	int rc = zmq_bind(handle->socket, "inproc://zeromq.zap.01");
	assert(rc == 0);

	rc = pthread_create(&handle->tid, NULL, s_zap_thread, handle);
	assert(rc == 0);

	return handle;
}

void zap_shutdown(void *handle)
{
	if (!handle) return;

	zap_t *z = (zap_t*)handle;
	pthread_cancel(z->tid);

	void *_;
	pthread_join(z->tid, &_);

	vzmq_shutdown(z->socket, 400);
	free(z);
}
