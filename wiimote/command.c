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
 *  03/01/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#include <stdint.h>
#include <unistd.h>
#include "wiimote_internal.h"

/* IR Sensitivity Block */
unsigned char ir_block1[] = CLIFF_IR_BLOCK_1;
unsigned char ir_block2[] = CLIFF_IR_BLOCK_2;

struct write_seq ir_enable10_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xB00030, (const void *)"\x08", 1,     WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xB00000, ir_block1, sizeof(ir_block1)-1, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xB0001A, ir_block2, sizeof(ir_block2)-1, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xB00033, (const void *)"\x01", 1,     WIIMOTE_RW_REG}
};

struct write_seq ir_enable12_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x04", 1, 0},
	{WRITE_SEQ_MEM, 0xB00030, (const void *)"\x08", 1,     WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xB00000, ir_block1, sizeof(ir_block1)-1, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xB0001A, ir_block2, sizeof(ir_block2)-1, WIIMOTE_RW_REG},
	{WRITE_SEQ_MEM, 0xB00033, (const void *)"\x03", 1,     WIIMOTE_RW_REG}
};

struct write_seq ir_disable_seq[] = {
	{WRITE_SEQ_RPT, RPT_IR_ENABLE1, (const void *)"\x00", 1, 0},
	{WRITE_SEQ_RPT, RPT_IR_ENABLE2, (const void *)"\x00", 1, 0}
};

#define CMD_BUF_LEN	21
int wiimote_command(struct wiimote *wiimote, enum wiimote_command command,
                    uint8_t flags) {
	int ret = 0;
	unsigned char buf[CMD_BUF_LEN];

	switch (command) {
	case WIIMOTE_CMD_STATUS:
		buf[0] = 0;
		if (send_report(wiimote, 0, RPT_STATUS_REQ, 1, buf)) {
			wiimote_err("Error requesting status");
			ret = -1;
		}
		break;
	case WIIMOTE_CMD_LED:
		wiimote->led_rumble_state = ((flags & 0x0F)<<4) |
		                            (wiimote->led_rumble_state & 0x01);
		buf[0]=wiimote->led_rumble_state;
		if (send_report(wiimote, SEND_RPT_NO_RUMBLE, RPT_LED_RUMBLE, 1, buf)) {
			wiimote_err("Error setting LEDs");
			ret = -1;
		}
		break;
	case WIIMOTE_CMD_RUMBLE:
		wiimote->led_rumble_state = (wiimote->led_rumble_state & 0xFE) |
		                            (flags ? 1 : 0);
		buf[0]=wiimote->led_rumble_state;
		if (send_report(wiimote, SEND_RPT_NO_RUMBLE, RPT_LED_RUMBLE, 1, buf)) {
			wiimote_err("Error setting rumble");
			ret = -1;
		}
		break;
	case WIIMOTE_CMD_RPT_MODE:
		update_rpt_mode(wiimote, flags);
		break;
	default:
		wiimote_err("Unknown command");
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

	/* Lock wiimote access */
	if (pthread_mutex_lock(&wiimote->wiimote_mutex)) {
		wiimote_err("Error locking rw_mutex");
		return -1;
	}

	/* Use -1 to update the reporting mode without changing flags */
	if (flags == -1) {
		flags = wiimote->rpt_mode_flags;
	}

	if ((flags & WIIMOTE_RPT_EXT) &&
	  ((wiimote->extension == WIIMOTE_EXT_NUNCHUK) ||
	   (wiimote->extension == WIIMOTE_EXT_CLASSIC))) {
		if ((flags & WIIMOTE_RPT_IR) &&
		  (flags & WIIMOTE_RPT_ACC)) {
			rpt_mode = RPT_BTN_ACC_IR10_EXT6;
			ir_enable_seq = ir_enable10_seq;
			seq_len = SEQ_LEN(ir_enable10_seq);
		}
		else if (flags & WIIMOTE_RPT_IR) {
			rpt_mode = RPT_BTN_IR10_EXT9;
			ir_enable_seq = ir_enable10_seq;
			seq_len = SEQ_LEN(ir_enable10_seq);
		}
		else if (flags & WIIMOTE_RPT_ACC) {
			rpt_mode = RPT_BTN_ACC_EXT16;
		}
		else if (flags & WIIMOTE_RPT_BTN) {
			rpt_mode = RPT_BTN_EXT8;
		}
		else {
			rpt_mode = RPT_EXT21;
		}	
	}
	else {
		if (flags & WIIMOTE_RPT_IR) {
			rpt_mode = RPT_BTN_ACC_IR12;
			ir_enable_seq = ir_enable12_seq;
			seq_len = SEQ_LEN(ir_enable12_seq);
		}
		else if (flags & WIIMOTE_RPT_ACC) {
			rpt_mode = RPT_BTN_ACC;
		}
		else {
			rpt_mode = RPT_BTN;
		}
	}

	/* Enable IR */
	/* TODO: only do this when necessary (record old IR mode) */
	if ((flags & WIIMOTE_RPT_IR)) {
		if (exec_write_seq(wiimote, seq_len, ir_enable_seq)) {
			wiimote_err("Error on IR enable");
			if (pthread_mutex_unlock(&wiimote->wiimote_mutex)) {
				wiimote_err("Error unlocking wiimote_mutex: deadlock warning");
			}
			return -1;
		}
	}
	/* Disable IR */
	else if ((wiimote->rpt_mode_flags & WIIMOTE_RPT_IR) &
	  !(flags & WIIMOTE_RPT_IR)) {
		if (exec_write_seq(wiimote, SEQ_LEN(ir_disable_seq),
		                   ir_disable_seq)) {
			wiimote_err("Error on IR enable");
			if (pthread_mutex_unlock(&wiimote->wiimote_mutex)) {
				wiimote_err("Error unlocking wiimote_mutex: deadlock warning");
			}
			return -1;
		}
	}

	/* Send SET_REPORT */
	buf[0]=0;
	buf[1]=rpt_mode;
	if (send_report(wiimote, 0, RPT_RPT_MODE, RPT_MODE_BUF_LEN, buf)) {
		wiimote_err("Error setting report state");
		if (pthread_mutex_unlock(&wiimote->wiimote_mutex)) {
			wiimote_err("Error unlocking wiimote_mutex: deadlock warning");
		}
		return -1;
	}

	wiimote->rpt_mode_flags = flags;

	/* Unlock wiimote_mutex */
	if (pthread_mutex_unlock(&wiimote->wiimote_mutex)) {
		wiimote_err("Error unlocking wiimote_mutex: deadlock warning");
	}

	return 0;
}

