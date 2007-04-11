/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-07 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_info.class to btclass
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created file
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <cwiid.h>

#define OPTSTRING	"ahlq"

#define USAGE \
	"%s [OPTIONS]\n" \
	"  -a  list all bluetooth devices (not just wiimotes)\n" \
	"  -h  print this help message\n" \
	"  -l  long format (device details)\n" \
	"  -q  quiet mode\n"

int main(int argc, char *argv[])
{
	int c;
	uint8_t flags = 0;
	char long_format = 0;
	char quiet = 0;
	struct cwiid_bdinfo *bdinfo;
	int bdinfo_count;
	int i;
	char ba_str[18];

	/* Parse options */
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (c) {
		case 'h':
			printf(USAGE, argv[0]);
			return 0;
			break;
		case 'a':
			flags |= BT_NO_WIIMOTE_FILTER;
			break;
		case 'l':
			long_format = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case '?':
		default:
			fprintf(stderr, "Try '%s -h' for more information\n", argv[0]);
			return -1;
			break;
		}
	}

	/* TODO: check for other stuff on the command-line? */

	/* Handle quiet mode */
	if (quiet) {
		cwiid_set_err(NULL);
	}
	/* Print discoverable mode message */
	else if (flags & BT_NO_WIIMOTE_FILTER) {
		fprintf(stderr, "Put Bluetooth devices in discoverable mode now...\n");
	}
	else {
		fprintf(stderr, "Put Wiimotes in discoverable mode now "
		        "(press 1+2)...\n");
	}

	/* Get device info */
	if ((bdinfo_count = cwiid_get_bdinfo_array(-1, 2, -1, &bdinfo, flags))
	  == -1) {
		return -1;
	}

	/* Print info */
	for (i=0; i < bdinfo_count; i++) {
		ba2str(&bdinfo[i].bdaddr, ba_str);
		if (long_format) {
			printf("%s 0x%.2X%.2X%.2X %s\n", ba_str, bdinfo[i].btclass[2],
			       bdinfo[i].btclass[1], bdinfo[i].btclass[0], bdinfo[i].name);
		}
		else {
			printf("%s\n", ba_str);
		}
	}

	return 0;
}

