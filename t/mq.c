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

#include "test.h"

#define LOOPBACK "tcp://127.0.0.1:1026"

TESTS {
	alarm(5);
	log_open("t/mq", "file:/dev/null");
	log_level(LOG_EMERG, NULL);

	char *s;

	subtest { /* pdu instantiation */
		pdu_t *p = pdu_new();
		isnt_null(p, "pdu_new() returns a valid pdu handle");
		is_null(p->address, "pdu_new() doesn't set p->address");
		is_int(pdu_size(p), 0, "pdu_new() doesn't create any frames");
		pdu_free(p);
	}

	subtest { /* pdu construction */
		pdu_t *p = pdu_make("ERROR", 2, "404", "Not Found");
		isnt_null(p, "pdu_make() returns a valid pdu handle");
		is_null(p->address, "pdu_make() doesn't set p->address");
		is_null(pdu_peer(p), "pdu_make() doesn't set peer");
		is_string(pdu_type(p), "ERROR", "First frame is always the type");
		is_int(pdu_size(p), 3, "PDU consists of three frames");
		is_string(s = pdu_string(p, 0), "ERROR",     "frame 0"); free(s);
		is_string(s = pdu_string(p, 1), "404",       "frame 1"); free(s);
		is_string(s = pdu_string(p, 2), "Not Found", "frame 2"); free(s);

		is_null(s = pdu_string(p, 3), "frame 4 should not exist"); free(s);
		pdu_extendf(p, "%s:%i", "file.c", 231);
		is_string(s = pdu_string(p, 3), "file.c:231", "pdu_extendf()"); free(s);

		pdu_free(p);
	}

	subtest { /* binary segments */
		uint8_t i, bin[64];
		for (i = 0; i < 64; i++)
			bin[i] = i;

		pdu_t *p = pdu_make("BINARY", 1, "data");
		is_int(pdu_size(p), 2, "PDU initially only contains 2 segments");
		pdu_extend(p, bin, 64);
		is_int(pdu_size(p), 3, "pdu_extend() expands PDU to 3 segments");
		is_string(s = pdu_string(p, 2), "", "binary segment is '\\0...'"); free(s);

		size_t len = 41;
		uint8_t *res = pdu_segment(p, 2, &len);
		isnt_null(res, "pdu_segment() returns a new buffer pointer on success");
		ok(memcmp(bin, res, 64) == 0, "pdu_segment() gives the segment data");
		is_int(len, 64, "pdu_segment() gives the length of the segment");

		free(res);
		pdu_free(p);
	}

	subtest { /* diagnostics */
		pdu_t *p = pdu_make("DIAGNOSTIC", 3,

			"xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.XYZZY"
			"xyzzy.xyzzy.xyzzy.so.long.and.thanks.for.all.the.fish.xyzzy.xyzzy.xyzzy",

			"\xf\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf"
			"\xf\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf"
			"\xf\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf"
			"\xf\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf",

			"q.e.d.");

		FILE *io = tmpfile();
		isnt_null(io, "created tempfile");
		pdu_fprint(p, io);
		rewind(io);

		char line[8192];
		isnt_null(fgets(line, 8192, io), "line 1 read from file");
		is_string(line, "[   10] DIAGNOSTIC (+)\n", "line 1");

		isnt_null(fgets(line, 8192, io), "line 2 read from file");
		is_string(line, "[  142] xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.xyzzy.XYZZ... (+)\n", "line 2");

		isnt_null(fgets(line, 8192, io), "line 3 read from file");
		is_string(line, "[   64] 0f0102030405060708090a0b0c0d0e0f0f0102030405060708090a0b0c0d0e0f0f0102... (+)\n", "line 3");

		isnt_null(fgets(line, 8192, io), "line 4 read from file");
		is_string(line, "[    6] q.e.d.\n", "line 4");

		isnt_null(fgets(line, 8192, io), "line 5 read from file");
		is_string(line, "END\n", "line 5");

		is_null(fgets(line, 8192, io), "EOF reached");

		pdu_free(p);
	}

	subtest { /* send + receive */
		void *z = zmq_ctx_new(); assert(z);
		void *server = zmq_socket(z, ZMQ_ROUTER); assert(server);
		void *client = zmq_socket(z, ZMQ_DEALER); assert(client);

		int rc = zmq_bind(server, "inproc://server");
		assert(rc == 0);

		rc = zmq_connect(client, "inproc://server");
		assert(rc == 0);

		pdu_t *client_req = pdu_make("PING", 1, "payload");
		rc = pdu_send_and_free(client_req, client);
		is_int(rc, 0, "pdu_send() returns 0 on success");

		pdu_t *server_req = pdu_recv(server);
		isnt_null(server_req, "pdu_recv() returns the PDU it reads");
		is_int(pdu_size(server_req), 2, "req: PDU is 2 segments long");
		is_string(pdu_type(server_req), "PING", "req: type");
		is_string(s = pdu_string(server_req, 0), "PING",    "req: frame 0"); free(s);
		is_string(s = pdu_string(server_req, 1), "payload", "req: frame 1"); free(s);

		pdu_t *server_rep = pdu_reply(server_req, "PONG", 2, "ping...", "pong...");
		rc = pdu_send_and_free(server_rep, server);
		is_int(rc, 0, "pdu_send_and_free() returns 0 on success");

		pdu_t *client_rep = pdu_recv(client);
		isnt_null(client_rep, "pdu_recv() returns the PDU it reads");
		is_int(pdu_size(client_rep), 3, "rep: PDU is 3 segments long");
		is_string(pdu_type(client_rep), "PONG", "rep: type");
		is_string(s = pdu_string(client_rep, 0), "PONG",    "rep: frame 0"); free(s);
		is_string(s = pdu_string(client_rep, 1), "ping...", "rep: frame 1"); free(s);
		is_string(s = pdu_string(client_rep, 2), "pong...", "rep: frame 2"); free(s);

		vzmq_shutdown(client, 0);
		vzmq_shutdown(server, 0);
		zmq_ctx_destroy(z);

		pdu_free(server_req);
		pdu_free(client_rep);
	}

	subtest { /* pdu_to_hash */
		pdu_t *pdu;
		hash_t hash;
		memset(&hash, 0, sizeof(hash));

		pdu = pdu_make("SIMPLE", 6,
			"key1", "value1",
			"key2", "second value",
			"KeY3", "value III");

		ok(pdu_to_hash(pdu, &hash, 2) != 0,
			"failed to create hash from bad PDU (offset)");

		is_null(hash_get(&hash, "key1"), "key1 not set");
		is_null(hash_get(&hash, "key2"), "key2 not set");
		is_null(hash_get(&hash, "KeY3"), "KeY3 not set");

		ok(pdu_to_hash(pdu, &hash, 1) == 0, "created hash from PDU");
		is(hash_get(&hash, "key1"), "value1",       "key1 set");
		is(hash_get(&hash, "key2"), "second value", "key2 set");
		is(hash_get(&hash, "KeY3"), "value III",    "KeY3 set");

		pdu_free(pdu);
		hash_done(&hash, 1);
	}

	subtest { /* hash_to_pdu */
		pdu_t *pdu;
		hash_t hash;
		memset(&hash, 0, sizeof(hash));

		pdu = pdu_make("SIMPLE", 3, "extra", "header", "fields");
		hash_set(&hash, "uuid", "DEADBEEF");
		hash_set(&hash, "type", "worker");

		ok(pdu_from_hash(pdu, &hash) == 0, "extended PDU from hash");
		char *s;
		is(s = pdu_string(pdu, 1), "extra",    "PDU[1] == 'extra'");    free(s);
		is(s = pdu_string(pdu, 2), "header",   "PDU[2] == 'header'");   free(s);
		is(s = pdu_string(pdu, 3), "fields",   "PDU[3] == 'fields'");   free(s);
		is(s = pdu_string(pdu, 4), "uuid",     "PDU[4] == 'uuid'");     free(s);
		is(s = pdu_string(pdu, 5), "DEADBEEF", "PDU[5] == 'DEADBEEF'"); free(s);
		is(s = pdu_string(pdu, 6), "type",     "PDU[6] == 'type'");     free(s);
		is(s = pdu_string(pdu, 7), "worker",   "PDU[7] == 'worker'");   free(s);

		pdu_free(pdu);
		hash_done(&hash, 0);
	}

	subtest { /* vzmq_ident */
		uint8_t client_id[8] = { 0xde, 0xad, 0xbe, 0xef,     0xde, 0xca, 0xfb, 0xad };
		void *z = zmq_ctx_new(); assert(z);
		void *server = zmq_socket(z, ZMQ_ROUTER); assert(server); free(vzmq_ident(server, NULL));
		void *client = zmq_socket(z, ZMQ_DEALER); assert(client); vzmq_ident(client, client_id);

		int rc = zmq_bind(server, "inproc://server");
		assert(rc == 0);

		rc = zmq_connect(client, "inproc://server");
		assert(rc == 0);

		pdu_t *c1 = pdu_make("PING1", 0);
		ok(pdu_send_and_free(c1, client) == 0, "sent C1 PDU");
		pdu_t *s1 = pdu_recv(server);
		isnt_null(s1, "received S1 PDU");

		pdu_t *c2 = pdu_make("PING2", 0);
		ok(pdu_send_and_free(c2, client) == 0, "sent C2 PDU");
		pdu_t *s2 = pdu_recv(server);
		isnt_null(s2, "received S2 PDU");

		is_string(pdu_peer(s1), "deadbeefdecafbad", "PDU peer address is a hex string");
		is_string(pdu_peer(s1), pdu_peer(s2), "S1 and S2 PDUs are from the same peer");

		pdu_free(s1);
		pdu_free(s2);

		vzmq_shutdown(client, 0);
		vzmq_shutdown(server, 0);
		zmq_ctx_destroy(z);
	}

	subtest { /* zap */
		cert_t *server_cert = cert_generate(VIGOR_CERT_ENCRYPTION); assert(server_cert);
		cert_t *client_cert = cert_generate(VIGOR_CERT_ENCRYPTION); assert(client_cert);

		void *z = zmq_ctx_new(); assert(z);
		void *zap = zap_startup(z, NULL); assert(zap);

		void *server = zmq_socket(z, ZMQ_ROUTER); assert(server);
		void *client = zmq_socket(z, ZMQ_DEALER); assert(client);

		int optval = 1;
		ok(zmq_setsockopt(server, ZMQ_CURVE_SERVER, &optval, sizeof(optval)) == 0,
			"Set ZMQ_CURVE_SERVER on the server socket");
		ok(zmq_setsockopt(server, ZMQ_CURVE_SECRETKEY, cert_secret(server_cert), 32) == 0,
			"Set ZMQ_CURVE_SECRETKEY on the server socket");

		ok(zmq_setsockopt(client, ZMQ_CURVE_SECRETKEY, cert_secret(client_cert), 32) == 0,
			"Set ZMQ_CURVE_SECRETKEY on the client socket");
		ok(zmq_setsockopt(client, ZMQ_CURVE_PUBLICKEY, cert_public(client_cert), 32) == 0,
			"Set ZMQ_CURVE_PUBLICKEY on the client socket");
		ok(zmq_setsockopt(client, ZMQ_CURVE_SERVERKEY, cert_public(server_cert), 32) == 0,
			"Set ZMQ_CURVE_SERVERKEY on the client socket");

		cert_free(server_cert);
		cert_free(client_cert);

		ok(zmq_bind(   server, LOOPBACK) == 0, "Bound the server socket");
		ok(zmq_connect(client, LOOPBACK) == 0, "Connected the client socket");

		pdu_t *pdu = pdu_make("PING", 0);
		ok(pdu_send_and_free(pdu, client) == 0, "sent PDU");
		pdu = pdu_recv(server);
		isnt_null(pdu, "received PDU");
		pdu_free(pdu);

		vzmq_shutdown(client, 0);
		vzmq_shutdown(server, 0);
		zap_shutdown(zap);
		zmq_ctx_destroy(z);
	}

	subtest { /* zap - bad certs */
		char *badcert_x[32],
		     *badcert_y[32],
		     *badcert_z[32];
		memset(badcert_x, 'x', 32);
		memset(badcert_y, 'y', 32);
		memset(badcert_z, 'z', 32);

		void *z = zmq_ctx_new(); assert(z);
		void *zap = zap_startup(z, NULL); assert(zap);

		void *server = zmq_socket(z, ZMQ_ROUTER); assert(server);
		void *client = zmq_socket(z, ZMQ_DEALER); assert(client);

		int optval = 1;
		ok(zmq_setsockopt(server, ZMQ_CURVE_SERVER, &optval, sizeof(optval)) == 0,
			"Set ZMQ_CURVE_SERVER on the server socket");
		ok(zmq_setsockopt(server, ZMQ_CURVE_SECRETKEY, badcert_z, 32) == 0,
			"Set ZMQ_CURVE_SECRETKEY on the server socket");

		ok(zmq_setsockopt(client, ZMQ_CURVE_SECRETKEY, badcert_x, 32) == 0,
			"Set ZMQ_CURVE_SECRETKEY on the client socket");
		ok(zmq_setsockopt(client, ZMQ_CURVE_PUBLICKEY, badcert_y, 32) == 0,
			"Set ZMQ_CURVE_PUBLICKEY on the client socket");
		ok(zmq_setsockopt(client, ZMQ_CURVE_SERVERKEY, badcert_z, 32) == 0,
			"Set ZMQ_CURVE_SERVERKEY on the client socket");

		ok(zmq_bind(   server, LOOPBACK) == 0, "Bound the server socket");
		ok(zmq_connect(client, LOOPBACK) == 0, "Connected the client socket");

		ok(pdu_send_and_free(pdu_make("PING?", 0), client) == 0, "sent PDU");

		/* now we have to use a poller, since we should never get a response */
		diag("waiting up to 2s for a response");
		zmq_pollitem_t poll[1] = { { server, 0, ZMQ_POLLIN, 0 } };
		is_int(zmq_poll(poll, 1, 2000), 0, "no response in 2s of waiting");

		vzmq_shutdown(client, 0);
		vzmq_shutdown(server, 0);
		zap_shutdown(zap);
		zmq_ctx_destroy(z);
	}

	subtest { /* endpoint resolution */
		strings_t *res;
		const char *e;

		e = "inproc://leave.it.alone";
		res = vzmq_resolve(e, AF_UNSPEC);
		isnt_null(res, "resolved %s to a list of endpoints", e);
		is_int(res->num, 1, "%s -> 1 result", e);
		is(res->strings[0], e, "%s resolves to itself", e);
		strings_free(res);

		e = "tcp://1.2.3.4:5678";
		res = vzmq_resolve(e, AF_UNSPEC);
		isnt_null(res, "resolved %s to a list of endpoints", e);
		is_int(res->num, 1, "%s -> 1 result", e);
		is(res->strings[0], e, "%s resolves to itself", e);
		strings_free(res);

		e = "tcp://localhost:5678";
		res = vzmq_resolve(e, AF_INET); /* IPv4 */
		isnt_null(res, "resolved %s to a list of endpoints", e);
		is(res->strings[0], "tcp://127.0.0.1:5678", "localhost is 127.0.0.1");
		strings_free(res);

		e = "tcp://rr.tap.niftylogic.net:5678";
		res = vzmq_resolve(e, AF_INET); /* IPv4 */
		isnt_null(res, "resolved %s to a list of endpoints", e);
		is_int(res->num, 3, "%s -> 3 results", e);
		if (res->num == 3) {
			int i, saw[3] = { 0 };
			for (i = 0; i < 3; i++) {
				if (strcmp(res->strings[i], "tcp://192.0.2.10:5678") == 0) saw[0]++;
				if (strcmp(res->strings[i], "tcp://192.0.2.11:5678") == 0) saw[1]++;
				if (strcmp(res->strings[i], "tcp://192.0.2.12:5678") == 0) saw[2]++;
			}
			is_int(saw[0], 1, "found 'tcp://192.0.2.10:5678' correct number of times");
			is_int(saw[1], 1, "found 'tcp://192.0.2.11:5678' correct number of times");
			is_int(saw[2], 1, "found 'tcp://192.0.2.12:5678' correct number of times");
		} else {
			diag(strings_join(res, "\n"));
		}
		strings_free(res);
	}

	subtest { /* basic dup */
		char *s;
		pdu_t *orig = pdu_make("ORIGINAL", 4, "this", "is", "A", "test");
		pdu_t *copy = pdu_dup(orig, NULL);

		isnt_null(copy, "pdu_dup() created a copy PDU");
		is(pdu_type(copy), "ORIGINAL", "PDU type is copied over");
		is_int(pdu_size(copy), 5, "copy PDU is 5 frames long");
		is(s = pdu_string(copy, 1), "this", "COPY[1] == 'this'"); free(s);
		is(s = pdu_string(copy, 2), "is",   "COPY[2] == 'is'"); free(s);
		is(s = pdu_string(copy, 3), "A",    "COPY[3] == 'A'"); free(s);
		is(s = pdu_string(copy, 4), "test", "COPY[4] == 'test'"); free(s);

		pdu_free(copy);
		pdu_free(orig);
	}

	subtest { /* dup with new TYPE */
		char *s;
		pdu_t *orig = pdu_make("ORIGINAL", 4, "this", "is", "A", "test");
		pdu_t *copy = pdu_dup(orig, "A-COPY");

		isnt_null(copy, "pdu_dup() created a copy PDU");
		is(pdu_type(copy), "A-COPY", "PDU type is overridden");
		is_int(pdu_size(copy), 5, "copy PDU is 5 frames long");
		is(s = pdu_string(copy, 1), "this", "COPY[1] == 'this'"); free(s);
		is(s = pdu_string(copy, 2), "is",   "COPY[2] == 'is'"); free(s);
		is(s = pdu_string(copy, 3), "A",    "COPY[3] == 'A'"); free(s);
		is(s = pdu_string(copy, 4), "test", "COPY[4] == 'test'"); free(s);

		pdu_free(copy);
		pdu_free(orig);
	}

	subtest { /* copy */
		char *s;
		pdu_t *orig = pdu_make("ORIGINAL", 4, "this", "is", "A", "test");
		pdu_t *copy = pdu_make("COPY", 1, "it was");
		ok(pdu_copy(copy, orig, 3, 0) == 0, "copied frames to new PDU");

		isnt_null(copy, "pdu_dup() created a copy PDU");
		is(pdu_type(copy), "COPY", "PDU type is set");
		is_int(pdu_size(copy), 4, "copy PDU is 4 frames long");
		is(s = pdu_string(copy, 1), "it was", "COPY[1] == 'it was'"); free(s);
		is(s = pdu_string(copy, 2), "A",      "COPY[2] == 'A'"); free(s);
		is(s = pdu_string(copy, 3), "test",   "COPY[3] == 'test'"); free(s);

		pdu_free(copy);
		pdu_free(orig);
	}

	subtest { /* pdu_peer / pdu_attn */
		void *z = zmq_ctx_new(); assert(z);
		void *server = zmq_socket(z, ZMQ_ROUTER); assert(server);
		void *client = zmq_socket(z, ZMQ_DEALER); assert(client);

		int rc = zmq_bind(server, "inproc://peering");
		assert(rc == 0);

		rc = zmq_connect(client, "inproc://peering");
		assert(rc == 0);

		pdu_t *c1 = pdu_make("PING", 0);
		ok(pdu_send_and_free(c1, client) == 0, "sent C1 PDU");
		pdu_t *s1 = pdu_recv(server);
		isnt_null(s1, "received S1 PDU");

		isnt_null(pdu_peer(s1), "received a non-NULL peer address from client");

		pdu_t *s2 = pdu_make("PONG", 0);
		ok(pdu_attn(s2, pdu_peer(s1)) == 0, "Set the address frame via pdu_attn");
		ok(pdu_send_and_free(s2, server) == 0, "sent S2 reply PDU");
		pdu_t *c2 = pdu_recv(client);
		isnt_null(c2, "client received reply PDU");
		is_string(pdu_type(c2), "PONG", "PONG!");

		pdu_free(s1);
		pdu_free(c2);

		vzmq_shutdown(client, 0);
		vzmq_shutdown(server, 0);
		zmq_ctx_destroy(z);
	}

	subtest { /* PUB/SUB send/receive */
		void *z = zmq_ctx_new(); assert(z);
		void *server = zmq_socket(z, ZMQ_PUB); assert(server);
		void *client = zmq_socket(z, ZMQ_SUB); assert(server);

		int rc = zmq_bind(server, "inproc://broadcast");
		assert(rc == 0);

		rc = zmq_setsockopt(client, ZMQ_SUBSCRIBE, "", 0);
		assert(rc == 0);

		rc = zmq_connect(client, "inproc://broadcast");
		assert(rc == 0);

		pdu_t *s1 = pdu_make("ATTN", 2, "missive", "IMPORTANT!");
		ok(pdu_send_and_free(s1, server) == 0, "sent ATTN broadcast PDU");

		pdu_t *c1 = pdu_recv(client);
		isnt_null(c1, "received ATTN PDU from broadcaster");
		char *s;
		is_string(pdu_type(c1), "ATTN", "First frame is type of PDU");
		is_int(pdu_size(c1), 3, "PDU consists of three frames");
		is_string(s = pdu_string(c1, 0), "ATTN",       "frame 0"); free(s);
		is_string(s = pdu_string(c1, 1), "missive",    "frame 1"); free(s);
		is_string(s = pdu_string(c1, 2), "IMPORTANT!", "frame 2"); free(s);
		is_null(s = pdu_string(c1, 3), "frame 3 should not exist"); free(s);
		pdu_free(c1);

		vzmq_shutdown(client, 0);
		vzmq_shutdown(server, 0);
		zmq_ctx_destroy(z);
	}

	log_close();
	alarm(0);
	done_testing();
}
