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
 *  2007-03-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * audited error checking (coda and error handler sections)
 *  * updated comments
 *
 *  2007-03-06 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote parameter to cwiid_err calls
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#include <stdint.h>
#include <unistd.h>
#include "cwiid_internal.h"

/* IR Sensitivity Block */
unsigned char ir_block1[] = CLIFF_IR_BLOCK_1;
unsigned char ir_block2[] = CLIFF_IR_BLOCK_2;

struct write_seq ir_enable10_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xB00030, (const void *)"\x08", 1,     CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00000, ir_block1, sizeof(ir_block1)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB0001A, ir_block2, sizeof(ir_block2)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00033, (const void *)"\x01", 1,     CWIID_RW_REG}
};

struct write_seq ir_enable12_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xB00030, (const void *)"\x08", 1,     CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00000, ir_block1, sizeof(ir_block1)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB0001A, ir_block2, sizeof(ir_block2)-1, CWIID_RW_REG},
	{WRITE_SEQ_MEM, 0xB00033, (const void *)"\x03", 1,     CWIID_RW_REG}
};

struct write_seq ir_disable_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x00", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x00", 1, 0}
};

#define CMD_BUF_LEN	21
int cwiid_command(struct wiimote *wiimote, enum cwiid_command command,
                  uint8_t flags) {
	int ret = 0;
	unsigned char buf[CMD_BUF_LEN];

	switch (command) {
	case CWIID_CMD_STATUS:
		buf[0] = 0;
		if (send_report(wiimote, 0, RPT_STATUS_REQ, 1, buf)) {
			cwiid_err(wiimote, "Error requesting status");
			ret = -1;
		}
		break;
	case CWIID_CMD_LED:
		wiimote->led_rumble_state = ((flags & 0x0F)<<4) |
		                            (wiimote->led_rumble_state & 0x01);
		buf[0]=wiimote->led_rumble_state;
		if (send_report(wiimote, SEND_RPT_NO_RUMBLE, RPT_LED_RUMBLE, 1, buf)) {
			cwiid_err(wiimote, "Error setting LEDs");
			ret = -1;
		}
		break;
	case CWIID_CMD_RUMBLE:
		wiimote->led_rumble_state = (wiimote->led_rumble_state & 0xFE) |
		                            (flags ? 1 : 0);
		buf[0]=wiimote->led_rumble_state;
		if (send_report(wiimote, SEND_RPT_NO_RUMBLE, RPT_LED_RUMBLE, 1, buf)) {
			cwiid_err(wiimote, "Error setting rumble");
			ret = -1;
		}
		break;
	case CWIID_CMD_RPT_MODE:
		update_rpt_mode(wiimote, flags);
		break;
	default:
		cwiid_err(wiimote, "Unknown command");
		ret = -1;
		break;
	}

	return ret;
}

#define RPT_MODE_BUF_LEN 2
int update_rpt_mode(struct wiimote *wiimote, int8_t flags)
{
	unsigned char buf[RPT_MODE_BUF_LEN];
	uint8_t rpt_mode;
	struct write_seq *ir_enable_seq;
	int seq_len;
	int ret = 0;

	/* Lock wiimote access */
	if (pthread_mutex_lock(&wiimote->wiimote_mutex)) {
		cwiid_err(wiimote, "Error locking wiimote_mutex");
		ret = -1;
		goto CODA;
	}

	/* Use -1 to update the reporting mode without changing flags */
	if (flags == -1) {
		flags = wiimote->rpt_mode_flags;
	}

	/* Pick a report mode based on report flags */
	if ((flags & CWIID_RPT_EXT) &&
	  ((wiimote->extension == CWIID_EXT_NUNCHUK) ||
	   (wiimote->extension == CWIID_EXT_CLASSIC))) {
		if ((flags & CWIID_RPT_IR) &&
		  (flags & CWIID_RPT_ACC)) {
			rpt_mode = RPT_BTN_ACC_IR10_EXT6;
			ir_enable_seq = ir_enable10_seq;
			seq_len = SEQ_LEN(ir_enable10_seq);
		}
		else if (flags & CWIID_RPT_IR) {
			rpt_mode = RPT_BTN_IR10_EXT9;
			ir_enable_seq = ir_enable10_seq;
			seq_len = SEQ_LEN(ir_enable10_seq);
		}
		else if (flags & CWIID_RPT_ACC) {
			rpt_mode = RPT_BTN_ACC_EXT16;
		}
		else if (flags & CWIID_RPT_BTN) {
			rpt_mode = RPT_BTN_EXT8;
		}
		else {
			rpt_mode = RPT_EXT21;
		}	
	}
	else {
		if (flags & CWIID_RPT_IR) {
			rpt_mode = RPT_BTN_ACC_IR12;
			ir_enable_seq = ir_enable12_seq;
			seq_len = SEQ_LEN(ir_enable12_seq);
		}
		else if (flags & CWIID_RPT_ACC) {
			rpt_mode = RPT_BTN_ACC;
		}
		else {
			rpt_mode = RPT_BTN;
		}
	}

	/* Enable IR */
	/* TODO: only do this when necessary (record old IR mode) */
	if ((flags & CWIID_RPT_IR)) {
		if (exec_write_seq(wiimote, seq_len, ir_enable_seq)) {
			cwiid_err(wiimote, "Error on IR enable");
			ret = -1;
			goto CODA;
		}
	}
	/* Disable IR */
	else if ((wiimote->rpt_mode_flags & CWIID_RPT_IR) &
	         !(flags & CWIID_RPT_IR)) {
		if (exec_write_seq(wiimote, SEQ_LEN(ir_disable_seq),
		                   ir_disable_seq)) {
			cwiid_err(wiimote, "Error on IR enable");
			ret = -1;
			goto CODA;
		}
	}

	/* Send SET_REPORT */
	buf[0]=0;
	buf[1]=rpt_mode;
	if (send_report(wiimote, 0, RPT_RPT_MODE, RPT_MODE_BUF_LEN, buf)) {
		cwiid_err(wiimote, "Error setting report state");
		ret = -1;
		goto CODA;
	}

	wiimote->rpt_mode_flags = flags;

CODA:
	/* Unlock cwiid_mutex */
	if (pthread_mutex_unlock(&wiimote->wiimote_mutex)) {
		cwiid_err(wiimote, "Error unlocking wiimote_mutex: deadlock warning");
	}

	return ret;
}

