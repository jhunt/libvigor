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

static int64_t EXPIRY = 0;

#define fsm_ok(initial, event, final) do { \
	ha_t *ha = ha_new(HA_STATE_ ## initial); \
	if (EXPIRY) ha->expiry = EXPIRY; \
	is_int(ha->state, HA_STATE_ ## initial,  #initial " -> " #event " -> " #final ": initial state is "      #initial); \
	is_int(ha_check(ha, HA_ ## event), 0,    #initial " -> " #event " -> " #final ": transitions via event " #event); \
	is_int(ha->state, HA_STATE_ ## final,    #initial " -> " #event " -> " #final ": final state is "        #final); \
	ha_free(ha); \
} while (0)

#define fsm_noop(initial, event) do { \
	ha_t *ha = ha_new(HA_STATE_ ## initial); \
	if (EXPIRY) ha->expiry = EXPIRY; \
	is_int(ha->state, HA_STATE_ ## initial,  #initial " -> " #event " NOOP: initial state is "      #initial); \
	is_int(ha_check(ha, HA_ ## event), 0,    #initial " -> " #event " NOOP: transitions via event " #event); \
	is_int(ha->state, HA_STATE_ ## initial,  #initial " -> " #event " NOOP: final state is still "  #initial); \
	ha_free(ha); \
} while (0)

#define fsm_notok(initial, event) do { \
	ha_t *ha = ha_new(HA_STATE_ ## initial); \
	if (EXPIRY) ha->expiry = EXPIRY; \
	is_int(ha->state, HA_STATE_ ## initial,  #initial " -> " #event " INVALID: initial state is "      #initial); \
	isnt_int(ha_check(ha, HA_ ## event), 0,  #initial " -> " #event " INVALID: transition via event "  #event " fails"); \
	ha_free(ha); \
} while (0)

TESTS {
	alarm(5);
	void *ZMQ = zmq_ctx_new();

	subtest { /* basic HA FSM instantiation */
		ha_t *ha = ha_new(HA_STATE_STANDBY);
		isnt_null(ha, "ha_new() returned a valid pointer");

		is_int(ha->state, HA_STATE_STANDBY, "initial state set properly");
		is_int(ha->heartbeat, 1000, "default heartbeat set");

		ha_free(ha);
	}

	subtest { /* FSM transitions */
		fsm_noop  (PRIMARY, PEER_PRIMARY);
		fsm_ok    (PRIMARY, PEER_STANDBY, ACTIVE);
		fsm_ok    (PRIMARY, PEER_ACTIVE,  PASSIVE);
		fsm_noop  (PRIMARY, PEER_PASSIVE);
		fsm_noop  (PRIMARY, CLIENT_REQUEST);

		fsm_noop  (STANDBY, PEER_PRIMARY);
		fsm_noop  (STANDBY, PEER_STANDBY);
		fsm_ok    (STANDBY, PEER_ACTIVE,    PASSIVE);
		fsm_noop  (STANDBY, PEER_PASSIVE);
		fsm_notok (STANDBY, CLIENT_REQUEST);

		fsm_ok    (PASSIVE, PEER_PRIMARY, ACTIVE);
		fsm_ok    (PASSIVE, PEER_STANDBY, ACTIVE);
		fsm_noop  (PASSIVE, PEER_ACTIVE);
		fsm_notok (PASSIVE, PEER_PASSIVE);
	EXPIRY = time_ms() + 8000;
		fsm_notok (PASSIVE, CLIENT_REQUEST);
	EXPIRY = time_ms() - 8000;
		fsm_ok    (PASSIVE, CLIENT_REQUEST, ACTIVE);
	EXPIRY = 0;

		fsm_noop  (ACTIVE, PEER_PRIMARY);
		fsm_noop  (ACTIVE, PEER_STANDBY);
		fsm_notok (ACTIVE, PEER_ACTIVE);
		fsm_noop  (ACTIVE, PEER_PASSIVE);
		fsm_noop  (ACTIVE, CLIENT_REQUEST);
	}

	subtest { /* STATE and EVENT names */
		ha_t *ha = ha_new(HA_STATE_STANDBY);

		ha->state = HA_STATE_STANDBY; is(ha_state(ha), "standby", "HA_STATE_STANDBY == standby");
		ha->state = HA_STATE_PRIMARY; is(ha_state(ha), "primary", "HA_STATE_PRIMARY == primary");
		ha->state = HA_STATE_ACTIVE;  is(ha_state(ha), "active",  "HA_STATE_ACTIVE == active");
		ha->state = HA_STATE_PASSIVE; is(ha_state(ha), "passive", "HA_STATE_PASSIVE == passive");
		ha->state += 4; is(ha_state(ha), "invalid", "out-of-range (too high) == invalid");
		ha->state  = 0; is(ha_state(ha), "invalid", "out-of-range (too low) == invalid");

		ha->event = HA_PEER_STANDBY; is(ha_event(ha), "standby", "HA_PEER_STANDBY == standby");
		ha->event = HA_PEER_PRIMARY; is(ha_event(ha), "primary", "HA_PEER_PRIMARY == primary");
		ha->event = HA_PEER_ACTIVE;  is(ha_event(ha), "active",  "HA_PEER_ACTIVE == active");
		ha->event = HA_PEER_PASSIVE; is(ha_event(ha), "passive", "HA_PEER_PASSIVE == passive");
		ha->event = HA_CLIENT_REQUEST; is(ha_event(ha), "client-request", "HA_CLIENT_REQUEST == client-request");
		ha->event += 4; is(ha_event(ha), "invalid", "out-of-range (too high) == invalid");
		ha->event  = 0; is(ha_event(ha), "invalid", "out-of-range (too low) == invalid");

		ha_free(ha);
	}

	subtest { /* ha_is* helpers */
		ha_t *ha = ha_new(HA_STATE_STANDBY);

		ha->state = HA_STATE_STANDBY;
		ok( ha_isstandby(ha), "HA_STATE_STANDBY  ha_isstandby");
		ok(!ha_isprimary(ha), "HA_STATE_STANDBY !ha_isprimary");
		ok(!ha_isactive(ha),  "HA_STATE_STANDBY !ha_isactive");
		ok(!ha_ispassive(ha), "HA_STATE_STANDBY !ha_ispassive");

		ha->state = HA_STATE_PRIMARY;
		ok(!ha_isstandby(ha), "HA_STATE_PRIMARY !ha_isstandby");
		ok( ha_isprimary(ha), "HA_STATE_PRIMARY  ha_isprimary");
		ok(!ha_isactive(ha),  "HA_STATE_PRIMARY !ha_isactive");
		ok(!ha_ispassive(ha), "HA_STATE_PRIMARY !ha_ispassive");

		ha->state = HA_STATE_ACTIVE;
		ok(!ha_isstandby(ha), "HA_STATE_ACTIVE !ha_isstandby");
		ok(!ha_isprimary(ha), "HA_STATE_ACTIVE !ha_isprimary");
		ok( ha_isactive(ha),  "HA_STATE_ACTIVE  ha_isactive");
		ok(!ha_ispassive(ha), "HA_STATE_ACTIVE !ha_ispassive");

		ha->state = HA_STATE_PASSIVE;
		ok(!ha_isstandby(ha), "HA_STATE_PASSIVE !ha_isstandby");
		ok(!ha_isprimary(ha), "HA_STATE_PASSIVE !ha_isprimary");
		ok(!ha_isactive(ha),  "HA_STATE_PASSIVE !ha_isactive");
		ok( ha_ispassive(ha), "HA_STATE_PASSIVE  ha_ispassive");

		ha_free(ha);
	}

	subtest { /* full-stack, double-threaded fight to the death! */
		ha_t *fred   = ha_new(HA_STATE_PRIMARY); fred->heartbeat   = 100;
		ha_t *barney = ha_new(HA_STATE_STANDBY); barney->heartbeat = 100;
		int rc;

		fred->pub   = zmq_socket(ZMQ, ZMQ_PUB); if (!fred->pub)   BAIL_OUT("failed to create PUB socket");
		fred->sub   = zmq_socket(ZMQ, ZMQ_SUB); if (!fred->sub)   BAIL_OUT("failed to create SUB socket");
		barney->pub = zmq_socket(ZMQ, ZMQ_PUB); if (!barney->pub) BAIL_OUT("failed to create PUB socket");
		barney->sub = zmq_socket(ZMQ, ZMQ_SUB); if (!barney->sub) BAIL_OUT("failed to create SUB socket");

		rc = ha_bind(fred,   "inproc://fred.pub");   if (rc != 0) BAIL_OUT("failed to bind PUB socket");
		rc = ha_bind(barney, "inproc://barney.pub"); if (rc != 0) BAIL_OUT("failed to bind PUB socket");

		rc = ha_connect(fred, "inproc://barney.pub"); if (rc != 0) BAIL_OUT("failed to connect SUB socket");
		rc = ha_connect(barney, "inproc://fred.pub"); if (rc != 0) BAIL_OUT("failed to connect SUB socket");

		ok(ha_startup(fred)   == 0, "started up fred HA peer");
		ok(ha_startup(barney) == 0, "started up barney HA peer");

		sleep_ms(500);
		ok(ha_isactive(fred),    "FRED is now active");
		ok(ha_ispassive(barney), "BARNEY is now passive");

		ha_shutdown(fred);
		sleep_ms(500);
		is_int(ha_check(barney, HA_CLIENT_REQUEST), 0, "BARNEY takes over for FRED");
		ok(ha_isactive(barney), "BARNEY is now active");

		ha_shutdown(barney);
		diag("full-stack test complete");

		ha_free(fred);
		ha_free(barney);
	}

	zmq_ctx_destroy(ZMQ);
	alarm(0);
	done_testing();
}
