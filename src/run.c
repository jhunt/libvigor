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

    #######   ##     ## ##    ##
    ##    ##  ##     ## ###   ##
    ##    ##  ##     ## ####  ##
    #######   ##     ## ## ## ##
    ##   ##   ##     ## ##  ####
    ##    ##  ##     ## ##   ###
    ##     ##  #######  ##    ##

*/

#define MAXARGS 100

int run2(runner_t *ctx, char *cmd, ...)
{
	pid_t pid = fork();
	if (pid < 0) {
		logger(LOG_ERR, "Failed to fork(): %s", strerror(errno));
		return -1;
	}

	if (pid > 0) {
		int status = 0;
		waitpid(pid, &status, 0);

		if (ctx && ctx->out) rewind(ctx->out);
		if (ctx && ctx->err) rewind(ctx->err);

		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		else
			return -2;
	}

	if (ctx && ctx->gid) {
		logger(LOG_DEBUG, "Setting GID to %u", ctx->gid);
		if (setgid(ctx->gid) != 0) {
			logger(LOG_ERR, "Failed to set effective GID to %u: %s",
					ctx->gid, strerror(errno));
			exit(127);
		}
	}
	if (ctx && ctx->uid) {
		logger(LOG_DEBUG, "Setting UID to %u", ctx->uid);
		if (setuid(ctx->uid) != 0) {
			logger(LOG_ERR, "Failed to set effective UID to %u: %s",
					ctx->uid, strerror(errno));
			exit(127);
		}
	}

	char *args[MAXARGS] = { 0 };
	int argno = 1;

	char *p = strrchr(cmd, '/');
	args[0] = strdup(p ? ++p : cmd);

	va_list argv;
	va_start(argv, cmd);
	while ((args[argno++] = va_arg(argv, char *)) != (char *)0)
		;

	if (ctx && ctx->in) {
		int fd = fileno(ctx->in);
		if (fd >= 0) dup2(fd, 0);
	}

	if (ctx && ctx->out) {
		int fd = fileno(ctx->out);
		if (fd >= 0) dup2(fd, 1);
	}

	if (ctx && ctx->err) {
		int fd = fileno(ctx->err);
		if (fd >= 0) dup2(fd, 2);
	}

	execvp(cmd, args);
	logger(LOG_DEBUG, "run: execvp('%s') failed - %s",
			cmd, strerror(errno));
	exit(127);
}
