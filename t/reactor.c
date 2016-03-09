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

int reactor_echo(void *sock, pdu_t *pdu, void *data)
{
	pdu_t *reply;
	int rc = VIGOR_REACTOR_CONTINUE;

	if (strcmp(pdu_type(pdu), "ECHO") == 0) {
		char *payload = pdu_string(pdu, 1);
		reply = pdu_reply(pdu, "ECHO", 1, payload);
		free(payload);

	} else if (strcmp(pdu_type(pdu), "EXIT") == 0) {
		reply = pdu_reply(pdu, "BYE", 0);
		rc = VIGOR_REACTOR_HALT;

	} else {
		reply = pdu_reply(pdu, "ERROR", 1, "bad PDU type");
	}

	pdu_send_and_free(reply, sock);
	return rc;
}

int reactor_reverse(void *sock, pdu_t *pdu, void *data)
{
	pdu_t *reply;
	int rc = VIGOR_REACTOR_CONTINUE;

	if (strcmp(pdu_type(pdu), "EXIT") == 0) {
		reply = pdu_reply(pdu, "BYE", 0);
		rc = VIGOR_REACTOR_HALT;

	} else {
		char *reversed = pdu_string(pdu, 0);
		if (*reversed != '\0') {
			int i, len = strlen(reversed);
			for (i = 0; i < len / 2; i++) {
				char t = reversed[i];
				reversed[i] = reversed[len - i - 1];
				reversed[len - i - 1] = t;
			}
		}
		reply = pdu_reply(pdu, reversed, 0);
		free(reversed);
	}

	pdu_send_and_free(reply, sock);
	return rc;
}

void* server1_thread(void *Z)
{
	int rc;
	void *sock = zmq_socket(Z, ZMQ_ROUTER);
	rc = zmq_bind(sock, "inproc://reactor.16.server1");
	if (rc != 0) return string("failed to bind: %s", zmq_strerror(errno));

	reactor_t *r = reactor_new();
	if (!r) return strdup("failed to create a reactor");

	rc = reactor_set(r, sock, reactor_echo, NULL);
	if (rc != 0) return string("failed to add handler to reactor (rc=%i)", rc);

	rc = reactor_go(r);
	if (rc != 0) return string("reactor_go returned non-zero (rc=%i)", rc);

	reactor_free(r);
	vzmq_shutdown(sock, 0);
	return strdup("OK");
}

void *server2_thread(void *Z)
{
	int rc;
	void *echoer = zmq_socket(Z, ZMQ_ROUTER);
	rc = zmq_bind(echoer, "inproc://reactor.16.server2.echo");
	if (rc != 0) return string("failed to bind echoer: %s", zmq_strerror(errno));

	void *reverser = zmq_socket(Z, ZMQ_ROUTER);
	rc = zmq_bind(reverser, "inproc://reactor.16.server2.reverse");
	if (rc != 0) return string("failed to bind reverser: %s", zmq_strerror(errno));

	reactor_t *r = reactor_new();
	if (!r) return strdup("failed to create a reactor");

	rc = reactor_set(r, echoer, reactor_echo, NULL);
	if (rc != 0) return string("failed to add echo handler (rc=%i)", rc);
	rc = reactor_set(r, reverser, reactor_reverse, NULL);
	if (rc != 0) return string("failed to add reverse handler (rc=%i)", rc);

	rc = reactor_go(r);
	if (rc != 0) return string("reactor_go returned non-zero (rc=%i)", rc);

	reactor_free(r);
	vzmq_shutdown(echoer,   0);
	vzmq_shutdown(reverser, 0);
	return strdup("OK");
}

TESTS {
	alarm(5);
	void *Z = zmq_ctx_new();
	char *s;

	subtest { /* whitebox testing */
		reactor_t *r = reactor_new();
		isnt_null(r, "reactor_new() gave us a new reactor handle");
		reactor_free(r);
	}

	subtest { /* full-stack client */
		pthread_t tid;
		int rc = pthread_create(&tid, NULL, server1_thread, Z);
		is_int(rc, 0, "created server1 thread");
		sleep_ms(250);

		void *sock = zmq_socket(Z, ZMQ_DEALER);
		rc = zmq_connect(sock, "inproc://reactor.16.server1");
		is_int(rc, 0, "connected to inproc:// endpoint");

		pdu_t *pdu, *reply;
		pdu = pdu_make("ECHO", 1, "Are you there, Reactor? It's me, Tester");
		rc = pdu_send_and_free(pdu, sock);
		is_int(rc, 0, "sent ECHO PDU to server1_thread");

		reply = pdu_recv(sock);
		isnt_null(reply, "Got a reply from server1_thread");
		is_string(s = pdu_string(reply, 0), "ECHO", "ECHO yields ECHO"); free(s);
		is_string(s = pdu_string(reply, 1), "Are you there, Reactor? It's me, Tester",
			"server1_thread echoed back our PDU"); free(s);
		pdu_free(reply);

		pdu = pdu_make("EXIT", 0);
		rc = pdu_send_and_free(pdu, sock);
		is_int(rc, 0, "sent EXIT PDU to server1_thread");

		reply = pdu_recv(sock);
		isnt_null(reply, "Got a reply from server1_thread");
		is_string(s = pdu_string(reply, 0), "BYE", "EXIT yields BYE"); free(s);
		pdu_free(reply);

		char *result = NULL;
		pthread_join(tid, (void**)(&result));
		is_string(result, "OK", "server1_thread returned OK");
		free(result);

		vzmq_shutdown(sock, 0);
	}

	subtest { /* multiple listening sockets */
		pthread_t tid;
		int rc = pthread_create(&tid, NULL, server2_thread, Z);
		is_int(rc, 0, "created server2 thread");
		sleep_ms(250);

		void *echo = zmq_socket(Z, ZMQ_DEALER);
		rc = zmq_connect(echo, "inproc://reactor.16.server2.echo");
		is_int(rc, 0, "connected to inproc://echo endpoint");

		void *reverse = zmq_socket(Z, ZMQ_DEALER);
		rc = zmq_connect(reverse, "inproc://reactor.16.server2.reverse");
		is_int(rc, 0, "connected to inproc://reverse endpoint");

		pdu_t *pdu, *reply;
		pdu = pdu_make("ECHO", 1, "XYZZY");
		rc = pdu_send_and_free(pdu, echo);
		is_int(rc, 0, "sent ECHO PDU to server2_thread:echo");

		reply = pdu_recv(echo);
		isnt_null(reply, "Got a reply from server2_thread");
		is_string(s = pdu_string(reply, 0), "ECHO", "ECHO yields ECHO"); free(s);
		is_string(s = pdu_string(reply, 1), "XYZZY", "server2_thread echoed back our PDU"); free(s);
		pdu_free(reply);

		pdu = pdu_make("Rewind and let me reverse it", 0);
		rc = pdu_send_and_free(pdu, reverse);
		is_int(rc, 0, "sent a PDU to the server2_thread:reverse");

		reply = pdu_recv(reverse);
		isnt_null(reply, "Got a reply from server2_thread");
		is_string(s = pdu_string(reply, 0), "ti esrever em tel dna dniweR",
				"Reverser socket got our PDU and did the needful"); free(s);
		pdu_free(reply);

		pdu = pdu_make("EXIT", 0);
		rc = pdu_send_and_free(pdu, echo);
		is_int(rc, 0, "sent EXIT PDU to server2_thread");

		reply = pdu_recv(echo);
		isnt_null(reply, "Got a reply from server2_thread");
		is_string(s = pdu_string(reply, 0), "BYE", "EXIT yields BYE"); free(s);
		pdu_free(reply);

		char *result = NULL;
		pthread_join(tid, (void**)(&result));
		is_string(result, "OK", "server2_thread returned OK");
		free(result);

		vzmq_shutdown(echo,    0);
		vzmq_shutdown(reverse, 0);
	}

	zmq_ctx_destroy(Z);
	alarm(0);
	done_testing();
}
