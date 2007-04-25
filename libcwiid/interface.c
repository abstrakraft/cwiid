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
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created for API overhaul
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "cwiid_internal.h"

int cwiid_get_id(struct wiimote *wiimote)
{
	return wiimote->id;
}

int cwiid_set_data(struct wiimote *wiimote, const void *data)
{
	wiimote->data = data;
	return 0;
}

const void *cwiid_get_data(struct wiimote *wiimote)
{
	return wiimote->data;
}

int cwiid_enable(struct wiimote *wiimote, int flags)
{
	if ((flags & CWIID_FLAG_NONBLOCK) &&
	  !(wiimote->flags & CWIID_FLAG_NONBLOCK)) {
		if (fcntl(wiimote->mesg_pipe[0], F_SETFL, O_NONBLOCK)) {
			cwiid_err(wiimote, "File control error (mesg pipe)");
			return -1;
		}
	}
	wiimote->flags |= flags;
	return 0;
}

int cwiid_disable(struct wiimote *wiimote, int flags)
{
	if ((flags & CWIID_FLAG_NONBLOCK) &&
	  (wiimote->flags & CWIID_FLAG_NONBLOCK)) {
		if (fcntl(wiimote->mesg_pipe[0], F_SETFL, 0)) {
			cwiid_err(wiimote, "File control error (mesg pipe)");
			return -1;
		}
	}
	wiimote->flags &= ~flags;
	return 0;
}

int cwiid_set_mesg_callback(struct wiimote *wiimote,
                            cwiid_mesg_callback_t *callback)
{
	if (wiimote->mesg_callback) {
		if (cancel_mesg_callback(wiimote)) {
			/* prints it's own errors */
			return -1;
		}
	}

	wiimote->mesg_callback = callback;

	if (wiimote->mesg_callback) {
		if (pthread_create(&wiimote->mesg_callback_thread, NULL,
		                  (void *(*)(void *))&mesg_callback_thread, wiimote)) {
			cwiid_err(wiimote, "Thread creation error (callback thread)");
			return -1;
		}
	}

	return 0;
}

int cwiid_get_mesg(struct wiimote *wiimote, int *mesg_count,
                   union cwiid_mesg *mesg[])
{
	struct mesg_array ma;

	if (read_mesg_array(wiimote->mesg_pipe[0], &ma)) {
		if (errno == EAGAIN) {
			return -1;
		}
		else {
			cwiid_err(wiimote, "Pipe read error (mesg_pipe)");
			return -1;
		}
	}

	*mesg_count = ma.count;

	if ((*mesg = malloc(ma.count * sizeof ma.array[0])) == NULL) {
		cwiid_err(wiimote, "Memory allocation error (mesg array)");
		return -1;
	}

	memcpy(*mesg, &ma.array, ma.count * sizeof (*mesg)[0]);

	return 0;
}

int cwiid_get_state(struct wiimote *wiimote, struct cwiid_state *state)
{
	if (pthread_mutex_lock(&wiimote->state_mutex)) {
		cwiid_err(wiimote, "Mutex lock error (state mutex)");
		return -1;
	}

	memcpy(state, &wiimote->state, sizeof *state);

	if (pthread_mutex_unlock(&wiimote->state_mutex)) {
		cwiid_err(wiimote, "Mutex unlock error (state mutex) - "
		                   "deadlock warning");
		return -1;
	}

	return 0;
}
