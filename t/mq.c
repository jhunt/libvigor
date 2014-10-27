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

#include "test.h"

TESTS {
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
		is_string(pdu_string(p, 0), "ERROR",     "frame 0");
		is_string(pdu_string(p, 1), "404",       "frame 1");
		is_string(pdu_string(p, 2), "Not Found", "frame 2");

		is_null(pdu_string(p, 3), "frame 4 should not exist");
		pdu_extendf(p, "%s:%i", "file.c", 231);
		is_string(pdu_string(p, 3), "file.c:231", "pdu_extendf()");

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
		is_string(pdu_string(p, 2), "", "binary segment is '\\0...'");

		size_t len = 41;
		uint8_t *res = pdu_segment(p, 2, &len);
		isnt_null(res, "pdu_segment() returns a new buffer pointer on success");
		ok(memcmp(bin, res, 64) == 0, "pdu_segment() gives the segment data");
		is_int(len, 64, "pdu_segment() gives the length of the segment");

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

		is_null(fgets(line, 8193, io), "EOF reached");
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
		rc = pdu_send(client_req, client);
		is_int(rc, 0, "pdu_send() returns 0 on success");

		pdu_t *server_req = pdu_recv(server);
		isnt_null(server_req, "pdu_recv() returns the PDU it reads");
		is_int(pdu_size(server_req), 2, "req: PDU is 2 segments long");
		is_string(pdu_type(server_req), "PING", "req: type");
		is_string(pdu_string(server_req, 0), "PING",    "req: frame 0");
		is_string(pdu_string(server_req, 1), "payload", "req: frame 1");

		pdu_t *server_rep = pdu_reply(server_req, "PONG", 2, "ping...", "pong...");
		rc = pdu_send_and_free(server_rep, server);
		is_int(rc, 0, "pdu_send_and_free() returns 0 on success");

		pdu_t *client_rep = pdu_recv(client);
		isnt_null(client_rep, "pdu_recv() returns the PDU it reads");
		is_int(pdu_size(client_rep), 3, "rep: PDU is 3 segments long");
		is_string(pdu_type(client_rep), "PONG", "rep: type");
		is_string(pdu_string(client_rep, 0), "PONG",    "rep: frame 0");
		is_string(pdu_string(client_rep, 1), "ping...", "rep: frame 1");
		is_string(pdu_string(client_rep, 2), "pong...", "rep: frame 2");
	}
}
