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

/*

    ##     ##    ###
    ##     ##   ## ##
    ##     ##  ##   ##
    ######### ##     ##
    ##     ## #########
    ##     ## ##     ##
    ##     ## ##     ##

 */

static const char* HA_STATES[] = {
	"invalid",
	"primary",
	"standby",
	"active",
	"passive",
	NULL
};

static const char* HA_EVENTS[] = {
	"invalid",
	"primary",
	"standby",
	"active",
	"passive",
	"client-request",
	NULL
};

ha_t* ha_new(int state)
{
	ha_t *m = vcalloc(1, sizeof(ha_t));
	pthread_mutex_init(&m->lock, NULL);
	pthread_mutex_init(&m->exit, NULL);
	m->state = state;
	m->heartbeat = 1000;
	m->expiry = time_ms() + 2 * m->heartbeat;
	return m;
}

void ha_free(ha_t *m)
{
	pthread_mutex_destroy(&m->exit);
	pthread_mutex_destroy(&m->lock);
	vzmq_shutdown(m->sub, 0);
	vzmq_shutdown(m->pub, 500);
	free(m);
}

int ha_bind(ha_t *m, const char *endpoint)
{
	return zmq_bind(m->pub, endpoint);
}

int ha_connect(ha_t *m, const char *endpoint)
{
	if (zmq_connect(m->sub, endpoint) != 0) return -1;
	return zmq_setsockopt(m->sub, ZMQ_SUBSCRIBE, "", 0);
}

int ha_check(ha_t *m, int event)
{
	pthread_mutex_lock(&m->lock);
	m->event = event;

	// These are the PRIMARY and STANDBY states; we're waiting to become
	// ACTIVE or PASSIVE depending on events we get from our peer:
	switch (m->state) {
	case HA_STATE_PRIMARY:
		switch (m->event) {
		case HA_PEER_STANDBY:
			logger(LOG_DEBUG, "I: connected to backup (passive), ready active\n");
			m->state = HA_STATE_ACTIVE;
			break;

		case HA_PEER_ACTIVE:
			logger(LOG_DEBUG, "I: connected to backup (active), ready passive\n");
			m->state = HA_STATE_PASSIVE;
			break;
		}
		break;

	case HA_STATE_STANDBY:
		switch (m->event) {
		case HA_PEER_ACTIVE:
			logger(LOG_DEBUG, "I: connected to primary (active), ready passive\n");
			m->state = HA_STATE_PASSIVE;
			break;

		case HA_CLIENT_REQUEST:
			goto fail;
		}
		break;

	case HA_STATE_ACTIVE:
		switch (m->event) {
		case HA_PEER_ACTIVE:
			logger(LOG_DEBUG, "E: fatal error - dual actives, aborting\n");
			goto fail;
		}
		break;

	case HA_STATE_PASSIVE:
		switch (m->event) {
		case HA_PEER_PRIMARY:
			// Peer is restarting - become active, peer will go passive
			logger(LOG_DEBUG, "I: primary (passive) is restarting, ready active\n");
			m->state = HA_STATE_ACTIVE;
			break;

		case HA_PEER_STANDBY:
			// Peer is restarting - become active, peer will go passive
			logger(LOG_DEBUG, "I: backup (passive) is restarting, ready active\n");
			m->state = HA_STATE_ACTIVE;
			break;

		case HA_PEER_PASSIVE:
			// Two passives would mean cluster would be non-responsive
			logger(LOG_DEBUG, "E: fatal error - dual passives, aborting\n");
			goto fail;

		case HA_CLIENT_REQUEST:
			// Peer becomes active if timeout has passed
			// It's the client request that triggers the failover
			assert(m->expiry > 0);
			fprintf(stdout, "expires: %li\n", m->expiry);
			if (time_ms() >= m->expiry) {
				// If peer is dead, switch to the active state
				logger(LOG_DEBUG, "I: failover successful, ready active\n");
				m->state = HA_STATE_ACTIVE;
			} else {
				// If peer is alive, reject connections
				goto fail;
			}
			break;
		}
		break;
	}
	pthread_mutex_unlock(&m->lock);
	return 0;

fail:
	pthread_mutex_unlock(&m->lock);
	return 1;
}

#define WITH_LOCK(m,x) do { \
	pthread_mutex_lock(&m->lock); \
	(x); \
	pthread_mutex_unlock(&m->lock); \
} while (0);

const char* ha_state(ha_t *m)
{
	const char *r;
	int i = m->state; if (i < 0 || i > HA_MAX_STATE) i = 0;
	WITH_LOCK(m, r = HA_STATES[i]);
	return r;
}

const char *ha_event(ha_t *m)
{
	const char *r;
	int i = m->event; if (i < 0 || i > HA_MAX_EVENT) i = 0;
	WITH_LOCK(m, r = HA_EVENTS[i]);
	return r;
}

#define ha_is(m,s) do { \
	int r; \
	WITH_LOCK((m), r = ((m)->state == (s))); \
	return r; \
} while (0)
int ha_isprimary(ha_t *m) { ha_is(m, HA_STATE_PRIMARY); }
int ha_isstandby(ha_t *m) { ha_is(m, HA_STATE_STANDBY); }
int ha_isactive(ha_t *m)  { ha_is(m, HA_STATE_ACTIVE);  }
int ha_ispassive(ha_t *m) { ha_is(m, HA_STATE_PASSIVE); }
#undef ha_is

static void* ha_thread(void *_)
{
	ha_t *m = (ha_t*)_;

	int64_t next = time_ms() + m->heartbeat;
	while (!signalled()) {
		zmq_pollitem_t socks[] = {{ m->sub, 0, ZMQ_POLLIN, 0 }};
		int left = (int)((next - time_ms()));
		if (left < 0) left = 0;

		errno = 0;
		int rc = zmq_poll(socks, 1, left);
		if (rc == -1)
			break;

		rc = pthread_mutex_trylock(&m->exit);
		if (rc != 0)
			break; // someone locked 'exit'; we should shutdown
		pthread_mutex_unlock(&m->exit);

		if (socks[0].revents & ZMQ_POLLIN) {
			pdu_t *pdu = pdu_recv(m->sub);
			assert(pdu);

			if (strcmp(pdu_type(pdu), "HA") != 0)
				break; // Error, so exit

			char *s = pdu_string(pdu, 1);
			int state = atoi(s); free(s);
			if (ha_check(m, state) != 0)
				break; // Error, so exit

			m->expiry = time_ms() + 2 * m->heartbeat;
			pdu_free(pdu);
		}

		if (time_ms() >= next) {
			pdu_t *pdu = pdu_make("HA", 0);
			assert(pdu);

			pdu_extendf(pdu, "%i", m->state);
			int rc = pdu_send_and_free(pdu, m->pub);
			assert(rc == 0);

			next = time_ms() + m->heartbeat;
		}
	}

	return m;
}
int ha_startup(ha_t *m)
{
	return pthread_create(&m->tid, NULL, ha_thread, m);
}

void ha_shutdown(ha_t *m)
{
	void *_;
	pthread_mutex_lock(&m->exit);
	pthread_join(m->tid, &_);
	pthread_mutex_unlock(&m->exit);
}
