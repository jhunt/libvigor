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

/*

     ######  ####  ######   ##    ##    ###    ##        ######
    ##    ##  ##  ##    ##  ###   ##   ## ##   ##       ##    ##
    ##        ##  ##        ####  ##  ##   ##  ##       ##
     ######   ##  ##   #### ## ## ## ##     ## ##        ######
          ##  ##  ##    ##  ##  #### ######### ##             ##
    ##    ##  ##  ##    ##  ##   ### ##     ## ##       ##    ##
     ######  ####  ######   ##    ## ##     ## ########  ######

 */

static int VIGOR_SIGNALLED = 0;
static void vigor_sig_handler(int sig)
{
	VIGOR_SIGNALLED = 1;
}

void signal_handlers(void)
{
	struct sigaction action;
	action.sa_handler = vigor_sig_handler;
	action.sa_flags = SA_SIGINFO;

	sigemptyset(&action.sa_mask);
	sigaction(SIGINT,  &action, NULL);
	sigaction(SIGTERM, &action, NULL);
}

int signalled(void)
{
	int rc = VIGOR_SIGNALLED > 0;
	VIGOR_SIGNALLED = 0;
	return rc;
}
