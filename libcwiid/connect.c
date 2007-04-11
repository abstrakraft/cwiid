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
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * cancel rw operations from cwiid_disconnect
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * cwiid_connect now takes a pointer to bdaddr_t
 *  * changed cwiid_findfirst to cwiid_find_wiimote
 *
 *  2007-03-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed memcpy to bacmp
 *  * audited error checking (coda and error handler sections)
 *  * updated comments
 *
 *  2007-03-06 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote parameter to cwiid_err calls
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include "cwiid_internal.h"
#include "queue.h"

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static int wiimote_id = 0;

cwiid_wiimote_t *cwiid_connect(bdaddr_t *bdaddr,
                               cwiid_mesg_callback_t *mesg_callback, int *id)
{
	struct wiimote *wiimote = NULL;
	struct sockaddr_l2 ctl_remote_addr, int_remote_addr;

	/* Allocate wiimote */
	if ((wiimote = malloc(sizeof *wiimote)) == NULL) {
		cwiid_err(NULL, "Error allocating cwiid_wiimote");
		goto ERR_HND;
	}

	/* Set wiimote members for proper error detection */
	wiimote->ctl_socket = -1;
	wiimote->int_socket = -1;
	wiimote->dispatch_queue = NULL;

	/* Global Lock, Store and Increment wiimote_id */
	if (pthread_mutex_lock(&global_mutex)) {
		cwiid_err(NULL, "Error locking global lock");
		goto ERR_HND;
	}
	wiimote->id = wiimote_id++;
	if (pthread_mutex_unlock(&global_mutex)) {
		cwiid_err(wiimote, "Error unlocking global lock");
		goto ERR_HND;
	}
	/* Return the id in a pointer, if desired */
	if (id) {
		*id = wiimote->id;
	}

	/* Store mesg callback */
	wiimote->mesg_callback = mesg_callback;

	/* If BDADDR_ANY is given, find available wiimote */
	if (bacmp(bdaddr, BDADDR_ANY) == 0) {
		if (cwiid_find_wiimote(bdaddr, 2)) {
			goto ERR_HND;
		}
	}

	/* Clear address structs, fill address family, address, and ports */
	memset(&ctl_remote_addr, 0, sizeof(ctl_remote_addr));
	ctl_remote_addr.l2_family = AF_BLUETOOTH;
	ctl_remote_addr.l2_bdaddr = *bdaddr;
	ctl_remote_addr.l2_psm = htobs(CTL_PSM);

	memset(&int_remote_addr, 0, sizeof(int_remote_addr));
	int_remote_addr.l2_family = AF_BLUETOOTH;
	int_remote_addr.l2_bdaddr = *bdaddr;
	int_remote_addr.l2_psm = htobs(INT_PSM);

	/* Get Bluetooth Sockets */
	if ((wiimote->ctl_socket =
	  socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) == -1) {
		cwiid_err(wiimote, "Error opening control socket");
		goto ERR_HND;
	}
	if ((wiimote->int_socket =
	  socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) == -1) {
		cwiid_err(wiimote, "Error opening interrupt socket");
		goto ERR_HND;
	}

	/* Connect to Wiimote */
	if (connect(wiimote->ctl_socket, (struct sockaddr *)&ctl_remote_addr,
		        sizeof(ctl_remote_addr))) {
		cwiid_err(wiimote, "Error opening control channel");
		goto ERR_HND;
	}
	if (connect(wiimote->int_socket, (struct sockaddr *)&int_remote_addr,
		        sizeof(int_remote_addr))) {
		cwiid_err(wiimote, "Error opening interrupt channel");
		goto ERR_HND;
	}

	/* Create Dispatch Queue */
	if ((wiimote->dispatch_queue = queue_new()) == NULL) {
		cwiid_err(wiimote, "Error creating dispatch queue");
		goto ERR_HND;
	}

	/* TODO: backout logic (pthread_*_destroy) */
	/* Mutex and cond init */
	if (pthread_mutex_init(&wiimote->wiimote_mutex, NULL) ||
	  pthread_mutex_init(&wiimote->rw_mutex, NULL) ||
	  pthread_cond_init(&wiimote->rw_cond, NULL) ||
	  pthread_mutex_init(&wiimote->rw_cond_mutex, NULL)) {
		cwiid_err(wiimote, "Error initializing synchronization variables");
		goto ERR_HND;
	}

	/* Set rw_status before interrupt thread */
	wiimote->rw_status = RW_NONE;
	wiimote->rw_error = 0;

	/* Launch interrupt channel listener and dispatch threads */
	if (pthread_create(&wiimote->int_listen_thread, NULL,
	                   (void *(*)(void *))&int_listen, wiimote)) {
		cwiid_err(wiimote, "Error creating interrupt channel listener thread");
		goto ERR_HND;
	}
	if (pthread_create(&wiimote->dispatch_thread, NULL,
	                   (void *(*)(void *))&dispatch, wiimote)) {
		pthread_cancel(wiimote->int_listen_thread);
		pthread_join(wiimote->int_listen_thread, NULL);
		cwiid_err(wiimote, "Error creating dispatch thread");
		goto ERR_HND;
	}

	/* Success!  Update state */
	wiimote->buttons = 0;
	wiimote->rpt_mode_flags = 0;
	wiimote->extension = CWIID_EXT_NONE;
	wiimote->led_rumble_state = 0;
	cwiid_command(wiimote, CWIID_CMD_LED, 0);
	cwiid_command(wiimote, CWIID_CMD_STATUS, 0);

	return wiimote;

ERR_HND:
	if (wiimote) {
		if (wiimote->dispatch_queue) {
			queue_free(wiimote->dispatch_queue,
			           (free_func_t *)free_mesg_array);
		}
		if (wiimote->int_socket != -1) {
			if (close(wiimote->int_socket)) {
				cwiid_err(wiimote, "Error closing interrupt channel");
			}
		}
		if (wiimote->ctl_socket != -1) {
			if (close(wiimote->ctl_socket)) {
				cwiid_err(wiimote, "Error closing control channel");
			}
		}
		free(wiimote);
	}
	return NULL;
}

int cwiid_disconnect(struct wiimote *wiimote)
{
	void *pthread_ret;

	/* Cancel and join int_thread */
	if (pthread_cancel(wiimote->int_listen_thread)) {
		/* int could exit on it's own, so we don't care */
		/* cwiid_err(wiimote, "Error canceling int_listen_thread"); */
	}
	else {
		if (pthread_join(wiimote->int_listen_thread, &pthread_ret)) {
			cwiid_err(wiimote, "Error joining int_listen_thread");
		}
		else if (pthread_ret != PTHREAD_CANCELED) {
			cwiid_err(wiimote, "Invalid return value from int_listen_thread");
		}
	}

	/* Cancel any RW operations in progress */
	wiimote->rw_error = 1;
	if (pthread_mutex_lock(&wiimote->rw_cond_mutex)) {
		cwiid_err(wiimote, "Error locking rw_cond_mutex: deadlock warning");
	}
	else {
		if (pthread_cond_signal(&wiimote->rw_cond)) {
			cwiid_err(wiimote, "Error signaling rw_cond: deadlock warning");
		}
		if (pthread_mutex_unlock(
		  &wiimote->rw_cond_mutex)) {
			cwiid_err(wiimote, "Error unlocking rw_cond_mutex");
		}
	}

	/* Cancel and detach dispatch_thread */
	/* We detach to decouple dispatch (which runs the callback) from callback
	 * code - specifically, a race condition exists for gtk apps */
	if (pthread_cancel(wiimote->dispatch_thread)) {
		cwiid_err(wiimote, "Error canceling dispatch_thread");
	}
	if (pthread_detach(wiimote->dispatch_thread)) {
		cwiid_err(wiimote, "Error detaching dispatch_thread");
	}

	/* Close sockets */
	if (close(wiimote->int_socket)) {
		cwiid_err(wiimote, "Error closing interrupt channel");
	}
	if (close(wiimote->ctl_socket)) {
		cwiid_err(wiimote, "Error closing control channel");
	}

	/* TODO: We have no way of telling if all in flight rw operations
	 * have exited yet, so for now, user must verify that all have returned
	 * before calling disconnect */

	/* Destroy sync variables */
	if (pthread_mutex_destroy(&wiimote->wiimote_mutex)) {
		cwiid_err(wiimote, "Error destroying cwiid_mutex");
	}
	if (pthread_mutex_destroy(&wiimote->rw_mutex)) {
		cwiid_err(wiimote, "Error destroying rw_mutex");
	}
	if (pthread_cond_destroy(&wiimote->rw_cond)) {
		cwiid_err(wiimote, "Error destroying rw_cond");
	}
	if (pthread_mutex_destroy(&wiimote->rw_cond_mutex)) {
		cwiid_err(wiimote, "Error destroying rw_cond_mutex");
	}

	free(wiimote);

	return 0;
}

