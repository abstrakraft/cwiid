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
 *  04/04/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated wiimote_read and wiimote_write to trigger and detect rw_error
 *
 *  03/14/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * wiimote_read - changed to obey decode flag only for register read
 *
 *  03/06/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote parameter to wiimote_err calls
 *
 *  03/01/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "wiimote_internal.h"

struct write_seq speaker_enable_seq[] = {
	{WRITE_SEQ_RPT, RPT_SPEAKER_ENABLE, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT,   RPT_SPEAKER_MUTE, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xA20009, (const void *)"\x01", 1, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xA20001, (const void *)"\x08", 1, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xA20001, (const void *)"\x00\x00\x00\x0C\x40\x00\x00",
	                          7, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xA20008, (const void *)"\x01", 1, WIIMOTE_RW_REG},
	{WRITE_SEQ_RPT,   RPT_SPEAKER_MUTE, (const void *)"\x00", 1, 0}
};

struct write_seq speaker_disable_seq[] = {
	{WRITE_SEQ_RPT,   RPT_SPEAKER_MUTE, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_SPEAKER_ENABLE, (const void *)"\x00", 1, 0}
};


#define RPT_READ_REQ_LEN 6
int wiimote_read(struct wiimote *wiimote, uint8_t flags, uint32_t offset,
                 uint16_t len, void *data)
{
	unsigned char buf[RPT_READ_REQ_LEN];
	int ret = 0;
	int i;

	/* TODO: once this code has been tested, check for rw_error before printing
	 * error messages */

	/* Exit immediately if rw_error */
	if (wiimote->rw_error) {
		return -1;
	}

	/* Compose read request packet */
	buf[0]=flags & (WIIMOTE_RW_EEPROM | WIIMOTE_RW_REG);
	buf[1]=(unsigned char)((offset>>16) & 0xFF);
	buf[2]=(unsigned char)((offset>>8) & 0xFF);
	buf[3]=(unsigned char)(offset & 0xFF);
	buf[4]=(unsigned char)((len>>8) & 0xFF);
	buf[5]=(unsigned char)(len & 0xFF);


	/* Lock wiimote rw access */
	if (pthread_mutex_lock(&wiimote->rw_mutex)) {
		wiimote_err(wiimote, "Error locking rw_mutex");
		return -1;
	}

	/* Check for rw_error after mutex wait */
	if (wiimote->rw_error) {
		ret = -1;
		goto CODA;
	}

	/* Setup read info */
	wiimote->rw_status = RW_PENDING;
	wiimote->read_buf = data;
	wiimote->read_len = len;
	wiimote->read_received = 0;

	/* Send read request packet */
	if (send_report(wiimote, 0, RPT_READ_REQ, RPT_READ_REQ_LEN, buf)) {
		wiimote_err(wiimote, "Error sending read request");
		ret = -1;
		goto CODA;
	}

	/* Lock rw_cond_mutex  */
	if (pthread_mutex_lock(&wiimote->rw_cond_mutex)) {
		wiimote_err(wiimote, "Error locking rw_cond_mutex");
		ret = -1;
		goto CODA;
	}
	/* Wait on condition, signalled by int_listen */
	while (!wiimote->rw_error && !ret && (wiimote->rw_status == RW_PENDING)) {
		if (pthread_cond_wait(&wiimote->rw_cond,
		                      &wiimote->rw_cond_mutex)) {
			wiimote_err(wiimote, "Error waiting on rw_cond");
			ret = -1;
			/* can't goto CODA from here - unlock rw_cond_mutex first */
		}
	}
	/* Unlock rw_cond_mutex */
	if (pthread_mutex_unlock(&wiimote->rw_cond_mutex)) {
		wiimote_err(wiimote, "Error unlocking rw_cond_mutex");
		wiimote->rw_error = 1;
		ret = -1;
		goto CODA;
	}

	/* Check status */
	if (wiimote->rw_status != RW_READY) {
		ret = -1;
	}

CODA:
	/* Clear rw_status */
	wiimote->rw_status = RW_NONE;

	/* Unlock rw_mutex */
	if (pthread_mutex_unlock(&wiimote->rw_mutex)) {
		wiimote_err(wiimote, "Error unlocking rw_mutex: deadlock warning");
		wiimote->rw_error = 1;
	}

	/* Decode (only for register reads) */
	if ((ret == 0) && (flags & WIIMOTE_RW_DECODE) &&
	  (flags & WIIMOTE_RW_REG)) {
		for (i=0; i < len; i++) {
			((unsigned char *)data)[i] = DECODE(((unsigned char *)data)[i]);
		}
	}

	return ret;
}

#define RPT_WRITE_LEN 21
int wiimote_write(struct wiimote *wiimote, uint8_t flags, uint32_t offset,
                  uint16_t len, const void *data)
{
	unsigned char buf[RPT_WRITE_LEN];
	uint16_t sent=0;
	int ret = 0;

	/* TODO: once this code has been tested, check for rw_error before printing
	 * error messages */

	/* Exit immediately if rw_error */
	if (wiimote->rw_error) {
		return -1;
	}

	/* Compose write packet header */
	buf[0]=flags;

	/* Lock wiimote rw access */
	if (pthread_mutex_lock(&wiimote->rw_mutex)) {
		wiimote_err(wiimote, "Error locking rw_mutex");
		return -1;
	}

	/* Send packets */
	/* Check rw_error after mutex wait */
	while (!wiimote->rw_error && (sent<len)) {
		wiimote->rw_status = RW_PENDING;

		/* Compose write packet */
		buf[1]=(unsigned char)(((offset+sent)>>16) & 0xFF);
		buf[2]=(unsigned char)(((offset+sent)>>8) & 0xFF);
		buf[3]=(unsigned char)((offset+sent) & 0xFF);
		if (len-sent >= 0x10) {
			buf[4]=(unsigned char)0x10;
		}
		else {
			buf[4]=(unsigned char)(len-sent);
		}
		memcpy(buf+5, data+sent, buf[4]);

		if (send_report(wiimote, 0, RPT_WRITE, RPT_WRITE_LEN, buf)) {
			wiimote_err(wiimote, "Error sending write");
			ret = -1;
			goto CODA;
		}
		/* Lock rw_cond_mutex  */
		else if (pthread_mutex_lock(&wiimote->rw_cond_mutex)) {
			wiimote_err(wiimote, "Error locking rw_cond_mutex");
			ret = -1;
			goto CODA;
		}
		else {
			/* Wait on condition, signalled by wiimote_int_listen */
			while (!wiimote->rw_error && !ret &&
			  (wiimote->rw_status == RW_PENDING)) {
				if (pthread_cond_wait(&wiimote->rw_cond,
				                      &wiimote->rw_cond_mutex)) {
					wiimote_err(wiimote, "Error waiting on rw_cond");
					ret = -1;
					/* can't goto CODA from here -
					 * unlock rw_cond_mutex first */
				}
			}
			/* Unlock rw_cond_mutex */
			if (pthread_mutex_unlock(&wiimote->rw_cond_mutex)) {
				wiimote_err(wiimote, "Error unlocking rw_cond_mutex");
				wiimote->rw_error = 1;
				ret = -1;
				goto CODA;
			}

			/* Check for error from cond_wait */
			if (ret == -1) {
				goto CODA;
			}

			/* Check status */
			if (wiimote->rw_status != RW_READY) {
				ret = -1;
				goto CODA;
			}
		}
		sent+=buf[4];
	}

CODA:
	/* Clear rw_status */
	wiimote->rw_status = RW_NONE;

	/* Unlock rw_mutex */
	if (pthread_mutex_unlock(&wiimote->rw_mutex)) {
		wiimote_err(wiimote, "Error unlocking rw_mutex: deadlock warning");
		wiimote->rw_error = 1;
	}

	return ret;
}

#define SOUND_BUF_LEN	21
int wiimote_beep(wiimote_t *wiimote)
{
	/* unsigned char buf[SOUND_BUF_LEN] = { 0xA0, 0xCC, 0x33, 0xCC, 0x33,
	    0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33,
	    0xCC, 0x33, 0xCC, 0x33}; */
	unsigned char buf[SOUND_BUF_LEN] = { 0xA0, 0xC3, 0xC3, 0xC3, 0xC3,
	    0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3,
	    0xC3, 0xC3, 0xC3, 0xC3};
	int i;
	int ret = 0;
	pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;
	struct timespec t;

	if (exec_write_seq(wiimote, SEQ_LEN(speaker_enable_seq),
	                   speaker_enable_seq)) {
		wiimote_err(wiimote, "Error on speaker enable");
		ret = -1;
	}

	pthread_mutex_lock(&timer_mutex);

	for (i=0; i<100; i++) {
		clock_gettime(CLOCK_REALTIME, &t);
		t.tv_nsec += 10204081;
		/* t.tv_nsec += 7000000; */
		if (send_report(wiimote, 0, RPT_SPEAKER_DATA, SOUND_BUF_LEN, buf)) {
		 	printf("%d\n", i);
			wiimote_err(wiimote, "Error on speaker data");
			ret = -1;
			break;
		}
		/* TODO: I should be shot for this, but hey, it works.
		 * longterm - find a better wait */
		pthread_cond_timedwait(&timer_cond, &timer_mutex, &t);
	}

	pthread_mutex_unlock(&timer_mutex);

	if (exec_write_seq(wiimote, SEQ_LEN(speaker_disable_seq),
	                   speaker_disable_seq)) {
		wiimote_err(wiimote, "Error on speaker disable");
		ret = -1;
	}

	return ret;
}

