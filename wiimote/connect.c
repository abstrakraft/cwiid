/* Copyright (C) 2007 L. Donnie Smith <wiimote@abstrakraft.org>
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
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include "wiimote_internal.h"
#include "queue.h"

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static int wiimote_id = 0;

wiimote_t *wiimote_connect(bdaddr_t bdaddr,
                           wiimote_mesg_callback_t *mesg_callback, int *id)
{
	struct wiimote *wiimote;
	struct sockaddr_l2 ctl_remote_addr, int_remote_addr;

	/* TODO: clean up error checking and backout code */

	/* Verify valid wiimote state */
	if ((wiimote = malloc(sizeof *wiimote)) == NULL) {
		wiimote_err("Error allocate wiimote)");
		return NULL;
	}

	/* Global Lock, Store and Increment wiimote_id */
	if (pthread_mutex_lock(&global_mutex)) {
		wiimote_err("Error locking global lock");
		free(wiimote);
		return NULL;
	}
	wiimote->id = wiimote_id++;
	if (pthread_mutex_unlock(&global_mutex)) {
		wiimote_err("Error unlocking global lock");
		free(wiimote);
		return NULL;
	}
	if (id) {
		*id = wiimote->id;
	}

	/* Store mesg callback */
	wiimote->mesg_callback = mesg_callback;

	/* Get bdaddr */
	/* TODO: better way to compare ? */
	if (memcmp(&bdaddr, BDADDR_ANY, sizeof(bdaddr_t)) == 0) {
		if (wiimote_findfirst(&bdaddr)) {
			free(wiimote);
			wiimote_err("Unable to find wiimote");
			return NULL;
		}
	}

	/* Clear address structs, fill address family, address, and ports */
	memset(&ctl_remote_addr, 0, sizeof(ctl_remote_addr));
	ctl_remote_addr.l2_family = AF_BLUETOOTH;
	ctl_remote_addr.l2_bdaddr = bdaddr;
	ctl_remote_addr.l2_psm = htobs(CTL_PSM);

	memset(&int_remote_addr, 0, sizeof(int_remote_addr));
	int_remote_addr.l2_family = AF_BLUETOOTH;
	int_remote_addr.l2_bdaddr = bdaddr;
	int_remote_addr.l2_psm = htobs(INT_PSM);

	/* Get sockets */
	wiimote->ctl_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	wiimote->int_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

	/* Connect */
	if (connect(wiimote->ctl_socket, (struct sockaddr *)&ctl_remote_addr,
		        sizeof(ctl_remote_addr))) {
		free(wiimote);
		wiimote_err("Error establishing control channel connection");
		return NULL;
	}
	else if (connect(wiimote->int_socket, (struct sockaddr *)&int_remote_addr,
		             sizeof(int_remote_addr))) {
		close(wiimote->ctl_socket);
		free(wiimote);
		wiimote_err("Error establishing interrupt channel connection");
		return NULL;
	}

	if ((wiimote->dispatch_queue = queue_new()) == NULL) {
		close(wiimote->int_socket);
		close(wiimote->ctl_socket);
		free(wiimote);
		wiimote_err("Error creating dispatch queue");
	}
		
	/* TODO: this should have finer grained error checking and
	 * backout logic (pthread_*_destroy) */
	/* Mutex and cond init */
	if (pthread_mutex_init(&wiimote->wiimote_mutex, NULL) ||
	  pthread_mutex_init(&wiimote->rw_mutex, NULL) ||
	  pthread_cond_init(&wiimote->rw_cond, NULL) ||
	  pthread_mutex_init(&wiimote->rw_cond_mutex, NULL)) {
		close(wiimote->int_socket);
		close(wiimote->ctl_socket);
		queue_free(wiimote->dispatch_queue, (free_func_t *)free_mesg_array);
		free(wiimote);
		wiimote_err("Error initializing synchronization variables");
		return NULL;
	}
	wiimote->rw_status = RW_NONE;

	/* Launch interrupt channel listener and dispatch threads */
	if (pthread_create(&wiimote->int_listen_thread, NULL,
	                   (void *(*)(void *))&int_listen, wiimote)) {
		close(wiimote->int_socket);
		close(wiimote->ctl_socket);
		queue_free(wiimote->dispatch_queue, (free_func_t *)free_mesg_array);
		free(wiimote);
		wiimote_err("Error creating interrupt channel listener thread");
		return NULL;
	}
	if (pthread_create(&wiimote->dispatch_thread, NULL,
	                   (void *(*)(void *))&dispatch, wiimote)) {
		close(wiimote->int_socket);
		close(wiimote->ctl_socket);
		pthread_cancel(wiimote->int_listen_thread);
		pthread_join(wiimote->int_listen_thread, NULL);
		queue_free(wiimote->dispatch_queue, (free_func_t *)free_mesg_array);
		free(wiimote);
		wiimote_err("Error creating dispatch thread");
		return NULL;
	}

	/* Success!  Update state */
	wiimote->buttons = 0;
	wiimote->rpt_mode_flags = 0;
	wiimote->extension = WIIMOTE_EXT_NONE;
	wiimote->led_rumble_state = 0;
	wiimote_command(wiimote, WIIMOTE_CMD_LED, 0);
	wiimote_command(wiimote, WIIMOTE_CMD_STATUS, 0);

	return wiimote;
}

int wiimote_disconnect(struct wiimote *wiimote)
{
	void *pthread_ret;

	/* Cancel and join int_thread */
	if (pthread_cancel(wiimote->int_listen_thread)) {
		wiimote_err("Error canceling int_listen_thread");
	}
	else {
		if (pthread_join(wiimote->int_listen_thread, &pthread_ret)) {
			wiimote_err("Error joining int_listen_thread");
		}
		else if (pthread_ret != PTHREAD_CANCELED) {
			wiimote_err("Invalid return value from int_listen_thread");
		}
	}
	/* TODO: cancel RW operations if they are in progress */
	/* Cancel and detach dispatch_thread */
	/* We detach to decouple dispatch (which runs the callback) from wiimote
	 * code - specifically, a race condition exists for gtk apps */
	if (pthread_cancel(wiimote->dispatch_thread)) {
		wiimote_err("Error canceling dispatch_thread");
	}
	if (pthread_detach(wiimote->dispatch_thread)) {
		wiimote_err("Error detaching dispatch_thread");
	}

	/* Close sockets */
	if (close(wiimote->int_socket)) {
		wiimote_err("Error closing interrupt channel");
	}
	if (close(wiimote->ctl_socket)) {
		wiimote_err("Error closing control channel");
	}

	/* Destroy sync variables */
	if (pthread_mutex_destroy(&wiimote->wiimote_mutex)) {
		wiimote_err("Error destroying wiimote_mutex");
	}
	if (pthread_mutex_destroy(&wiimote->rw_mutex)) {
		wiimote_err("Error destroying rw_mutex");
	}
	if (pthread_cond_destroy(&wiimote->rw_cond)) {
		wiimote_err("Error destroying rw_cond");
	}
	if (pthread_mutex_destroy(&wiimote->rw_cond_mutex)) {
		wiimote_err("Error destroying rw_cond_mutex");
	}

	free(wiimote);

	return 0;
}

