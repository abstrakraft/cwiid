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
 *  04/02/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * exception handling bugs
 *
 *  04/01/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created file
 */

#include <stdlib.h>
#include <string.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "wiimote_internal.h"

/* When filtering wiimotes, in order to avoid having to store the
 * remote names before the blue_dev array is malloced (because we don't
 * yet know how many wiimotes there are, we'll assume there are no more
 * than dev_count, and realloc to the actual number afterwards, since
 * reallocing to a smaller chunk should be fast. */
#define BT_MAX_INQUIRY 256
/* timeout in 2 second units */
int wiimote_get_info_array(int dev_id, unsigned int timeout, int max_wm,
                           struct wiimote_info **wm, uint8_t flags)
{
	/* TODO: I suppose we'll have to sift through BlueZ source to properly
	 * check errors here...
	 */
	inquiry_info *dev_list = NULL;
	int max_inquiry;
	int dev_count;
	int sock = -1;
	int wm_count;
	int i, j;
	int err = 0;
	int ret;

	/* NULLify for the benefit of error handling */
	*wm = NULL;

	/* If not given (=-1), get the first available Bluetooth interface */
	if (dev_id == -1) {
		if ((dev_id = hci_get_route(NULL)) == -1) {
			wiimote_err(NULL, "No Bluetooth interface found");
			return -1;
		}
	}

	/* Get Bluetooth Device List */
	if ((flags & BT_NO_WIIMOTE_FILTER) && (max_wm != -1)) {
		max_inquiry = max_wm;
	}
	else {
		max_inquiry = BT_MAX_INQUIRY;
	}
	if ((dev_count = hci_inquiry(dev_id, timeout, max_inquiry, NULL,
	                             &dev_list, IREQ_CACHE_FLUSH)) == -1) {
		wiimote_err(NULL, "Error on bluetooth device inquiry");
		err = 1;
		goto CODA;
	}

	if (dev_count == 0) {
		wm_count = 0;
		goto CODA;
	}

	/* Open connection to Bluetooth Interface */
	if ((sock = hci_open_dev(dev_id)) == -1) {
		wiimote_err(NULL, "Error opening Bluetooth interface");
		err = 1;
		goto CODA;
	}

	/* Allocate info list */
	if (max_wm == -1) {
		max_wm = dev_count;
	}
	if ((*wm = malloc(max_wm * sizeof **wm)) == NULL) {
		wiimote_err(NULL, "Error mallocing wiimote_info array");
		err = 1;
		goto CODA;
	}

	/* Copy dev_list to info */
	for (wm_count=i=0; (i < dev_count) && (wm_count < max_wm); i++) {
		/* timeout (5000) in milliseconds */
		if (hci_remote_name(sock, &dev_list[i].bdaddr, BT_WM_NAME_LEN,
		                    (*wm)[wm_count].name, 5000)) {
			wiimote_err(NULL, "Error reading Bluetooth device name");
			err = 1;
			goto CODA;
		}
		/* Filter? */
		if (!(flags & BT_NO_WIIMOTE_FILTER) &&
		  ((dev_list[i].dev_class[0] != WIIMOTE_CLASS_0) ||
		   (dev_list[i].dev_class[1] != WIIMOTE_CLASS_1) ||
		   (dev_list[i].dev_class[2] != WIIMOTE_CLASS_2) ||
		   (strncmp((*wm)[wm_count].name, WIIMOTE_NAME, BT_WM_NAME_LEN)))) {
			continue;
		}
		bacpy(&(*wm)[wm_count].bdaddr, &dev_list[i].bdaddr);
		for (j=0; j<3; j++) {
			(*wm)[wm_count].class[j] = dev_list[i].dev_class[j];
		}
		wm_count++;
	}

	if (wm_count == 0) {
		free(*wm);
	}
	else if (wm_count < max_wm) {
		if ((*wm = realloc(*wm, wm_count * sizeof **wm)) == NULL) {
			wiimote_err(NULL, "Error reallocing wiimote_info array");
			err = 1;
			goto CODA;
		}
	}

CODA:
	if (dev_list) free(dev_list);
	if (sock != -1) hci_close_dev(sock);
	if (err) {
		if (*wm) free(*wm);
		ret = -1;
	}
	else {
		ret = wm_count;
	}
	return ret;
}

int wiimote_find_wiimote(bdaddr_t *bdaddr, int timeout)
{
	struct wiimote_info *wm;
	int ret;

	if (timeout == -1) {
		while ((ret = wiimote_get_info_array(-1, 2, 1, &wm, 0)) == 0);
		if (ret == -1) {
			return -1;
		}
	}
	else {
		if (wiimote_get_info_array(-1, timeout, 1, &wm, 0) == 1) {
			return -1;
		}
	}

	bacpy(bdaddr, &wm[0].bdaddr);
	free(wm);
	return 0;
}
