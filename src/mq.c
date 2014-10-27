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

static void s_frame_scan(frame_t *f)
{
	assert(f);

	const char *data = frame_data(f);
	size_t i;
	f->flags |= VIGOR_FRAME_PRINTABLE;
	for (i = 0; i < frame_size(f); i++) {
		if (isprint(data[i])) continue;
		f->flags &= ~VIGOR_FRAME_PRINTABLE;
		break;
	}
}

static frame_t* s_frame_recv(void *zocket)
{
	assert(zocket);

	frame_t *f = vmalloc(sizeof(frame_t));
	int rc = zmq_msg_init(&f->msg);
	assert(rc == 0);

	if (zmq_msg_recv(&f->msg, zocket, 0) == -1) {
		zmq_msg_close(&f->msg);
		free(f);
		return NULL;
	}

	int more;
	size_t len;
	rc = zmq_getsockopt(zocket, ZMQ_RCVMORE, &more, &len);
	assert(rc == 0);

	s_frame_scan(f);
	return f;
}

static void s_frame_fprint(frame_t *f, FILE *io)
{
	fprintf(io, "[%i] ", (int)frame_size(f));

	char *data = frame_data(f);
	size_t len = frame_size(f);
	size_t max = frame_isprintable(f) ? 70 : 35;
	const char *ellips = "";
	if (len > max) {
		len = max;
		ellips = "...";
	}

	size_t i;
	for (i = 0; i < len; i++)
		fprintf(io, (frame_isprintable(f) ? "%c" : "%02x"), (data[i]));
	fprintf(io, "%s%s\n", ellips, (frame_isfinal(f) ? "" : " (+)"));
}

/*

    ##     ##  #######
    ###   ### ##     ##
    #### #### ##     ##
    ## ### ## ##     ##
    ##     ## ##  ## ##
    ##     ## ##    ##
    ##     ##  ##### ##

*/

frame_t* frame_new(const void *buf, size_t len, uint8_t flags)
{
	assert(buf);
	assert(len >= 0);

	frame_t *f = vmalloc(sizeof(frame_t));
	int rc = zmq_msg_init_size(&f->msg, len);
	if (rc != 0) {
		free(f);
		return NULL;
	}
	memcpy(frame_data(f), buf, len);

	f->flags = flags;
	s_frame_scan(f);

	return f;
}

void frame_free(frame_t *f)
{
	if (!f) return;
	list_delete(&f->l);
	zmq_msg_close(&f->msg);
	free(f);
}

int frame_eq(frame_t *a, frame_t *b)
{
	if (!a || !b) return 0;

	size_t len = frame_size(a);
	if (frame_size(b) != len) return 0;

	return memcmp(frame_data(a), frame_data(b), len) == 0;
}

char* frame_hex(frame_t *f)
{
	assert(f);

	static const char hex[] = "0123456789abcdef";
	char *s = vcalloc(2 * frame_size(f) + 1, sizeof(char));

	size_t i, len = frame_size(f);
	const char *data = frame_data(f);
	for (i = 0; i < len; i++) {
		s[i * 2 + 0] = hex[data[i] & 0xf0 >> 4];
		s[i * 2 + 1] = hex[data[i] & 0x0f     ];
	}
	s[i * 2] = '\0';
	return s;
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

	p->type = strdup(type);
	assert(p->type);

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
	assert(type);
	assert(n >= 0);

	pdu_t *p = vmalloc(sizeof(pdu_t));
	p->address = frame_new(frame_data(orig->address), frame_size(orig->address), 0);

	list_init(&p->frames);

	p->type = strdup(type);
	assert(p->type);

	va_list ap;
	va_start(ap, n);
	while (n-- > 0) {
		char *arg = va_arg(ap, char *);
		pdu_extend(p, arg, strlen(arg));
	}
	va_end(ap);

	return p;
}

void pdu_free(pdu_t *p)
{
	if (!p) return;

	frame_free(p->address);
	free(p->peer);
	free(p->type);

	frame_t *f, *f_tmp;
	for_each_object_safe(f, f_tmp, &p->frames, l)
		frame_free(f);

	free(p);
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

	frame_t *f = frame_new(buf, len, 0);
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
	vsnprintf(s, n, fmt, ap2);
	va_end(ap2);

	return pdu_extend(p, s, n + 1);
}

frame_t* pdu_frame(pdu_t *p, unsigned int i)
{
	assert(p);

	frame_t *f;
	for_each_object(f, &p->frames, l) {
		if (i-- == 0) return f;
	}
	return NULL;
}

char* pdu_string(pdu_t *p, unsigned int i)
{
	assert(p);

	frame_t *f = pdu_frame(p, i);
	if (!f) return NULL;

	char *s = vcalloc(frame_size(f) + 1, sizeof(char));
	memcpy(s, frame_data(f), frame_size(f));
	return s;
}

#define s_frame_sendm(f,zocket) \
	zmq_msg_send(&(f)->msg, (zocket), ZMQ_SNDMORE)

#define s_frame_send(f,zocket) \
	zmq_msg_send(&(f)->msg, (zocket), frame_isfinal(f) ? 0 : ZMQ_SNDMORE)

int pdu_send(pdu_t *p, void *zocket)
{
	assert(p);
	assert(zocket);

	int rc;

	if (p->address) {
		rc = s_frame_sendm(p->address, zocket);
		if (rc < 0) return rc;

		rc = s_frame_sendm(p->address, zocket);
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

	int body = 0;
	frame_t *f;

	for (;;) {
		f = s_frame_recv(zocket);
		if (!f) {
			pdu_free(p);
			return NULL;
		}

		if (!body) {
			if (frame_size(f) == 0) {
				body = 1;
				int more = frame_isprintable(f);
				frame_free(f);
				if (more) continue;
				else      break;

			} else {
				assert(!p->address);
				p->address =f;
			}
		} else {
			s_pdu_extend(p, f);
		}

		if (frame_isprintable(f))
			break;
	}

	free(p->peer);
	p->peer = p->address ? frame_hex(p->address) : strdup("none");

	free(p->type); p->type = pdu_string(p, 0);
	if (!p->type)  p->type = strdup("NOOP");

	return p;
}

void pdu_fprint(pdu_t *p, FILE *io)
{
	fprintf(io, "type: |%s|\n", p->type);
	if (p->address) {
		s_frame_fprint(p->address, io);
		fprintf(io, "---\n");
	}
	frame_t *f;
	for_each_object(f, &p->frames, l)
		s_frame_fprint(f, io);
	fprintf(io, "END\n");
}
