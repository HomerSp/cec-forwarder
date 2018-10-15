/*

  irsend -  application for sending IR-codes via lirc

  Copyright (C) 1998 Christoph Bartelmus (lirc@bartelmus.de)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <getopt.h>
#include <errno.h>
#include <limits.h>

#include <lirc/lirc_client.h>

static int lircFd = -1;
const char* prog;

int send_packet(lirc_cmd_ctx* ctx, int fd)
{
	int r;

	do {
		r = lirc_command_run(ctx, fd);
		if (r != 0 && r != EAGAIN)
			fprintf(stderr,
				"Error running command: %s\n", strerror(r));
	} while (r == EAGAIN);
	return r == 0 ? 0 : -1;
}

void reformat_simarg(char* code, char buffer[])
{
	unsigned int scancode;
	unsigned int repeat;
	char keysym[32];
	char remote[64];
	char trash[32];
	int r;

	r = sscanf(code, "%x %x %32s %64s %32s",
		   &scancode, &repeat, keysym, remote, trash);
	if (r != 4) {
		fprintf(stderr, "Bad simulate argument: %s\n", code);
		exit(EXIT_FAILURE);
	}
	snprintf(buffer, PACKET_SIZE, "%016x %02x %s %s",
		 scancode, repeat, keysym, remote);
}

bool lircSendOnce(const char* code) {
	lirc_cmd_ctx ctx;
	int r = lirc_command_init(&ctx, "SEND_ONCE %s %s\n",
						      "dilog", code);

	if (r != 0) {
		fprintf(stderr, "input too long\n");
		return false;
	}

	lirc_command_reply_to_stdout(&ctx);

	int retries = 0;
	while(retries++ < 5) {
		if(lircFd < 0) {
			lircFd = lirc_get_local_socket(NULL, 0);
			if (lircFd < 0) {
				fprintf(stderr, "Cannot open socket, retrying...");
				sleep(1);
				continue;
			}
		}

		if (send_packet(&ctx, lircFd) == -1) {
			// Try to reopen the lirc socket
			fprintf(stderr, "Could not send packet, retrying...");
			close(lircFd);
			lircFd = -1;
			continue;
		}

		break;
	}

	return (retries < 5);
}
