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

    ##     ## ########
    ##     ## ##     ##
    ##     ## ##     ##
    ######### ########
    ##     ## ##     ##
    ##     ## ##     ##
    ##     ## ########

 */

int hb_init(hb_t *hb, int ival, int fresh, double backoff, int max_delay)
{
	assert(hb);
	assert(ival > 0);
	assert(fresh > 0);
	assert(backoff >= 1.0);
	assert(max_delay >= ival);

	hb->interval  = ival;
	hb->freshness = fresh;
	hb->backoff   = backoff;
	hb->max_delay = max_delay;
	hb->expiry    = time_ms() + (hb->interval * hb->freshness);

	return hb_ping(hb);
}

int hb_ping(hb_t *hb)
{
	assert(hb);

	hb->missed = 0;
	hb->delay  = hb->interval;
	return 1;
}

int hb_miss(hb_t *hb)
{
	assert(hb);
	if (hb->missed > hb->freshness)
		return 0;
	hb->missed++;

	hb->delay *= hb->backoff;
	if (hb->delay > hb->max_delay)
		hb->delay = hb->max_delay;

	return hb_alive(hb);
}

int hb_alive(hb_t *hb)
{
	assert(hb);
	return hb->missed > hb->freshness
		? 0   /* missed too many pings */
		: 1;
}

void hb_sleep(hb_t *hb)
{
	assert(hb);
	sleep_ms(hb->delay);
}
