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
 *
 *  ChangeLog:
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_{connect,disconnect,command} to
 *    cwiid_{open,close,request_status|set_led|set_rumble|set_rpt_mode}
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * initialized param array
 *
 *  2007-04-08 Arthur Peters <amp@singingwizard.org>
 *  * added debounce and low pass filter
 *
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * type audit (stdint, const, char booleans)
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * made global variables static
 */

#include "wmplugin.h"

cwiid_wiimote_t *wiimote;

struct cursor {
	unsigned char valid;
	uint16_t pos[2];
};
int a_debounce, b_debounce;

/* static objects are initialized to 0 by default */
static int a_index = -1, b_index = -1;
static struct cwiid_ir_src a, b, prev_a, prev_b;

static unsigned char info_init = 0;
static struct wmplugin_info info;
static struct wmplugin_data data;

static struct cursor c, prev_c;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;

struct wmplugin_info *wmplugin_info() {
	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 2;
		info.axis_info[0].name = "X";
		info.axis_info[0].type = WMPLUGIN_ABS;
		info.axis_info[0].max  = 1024;
		info.axis_info[0].min  = 0;
		info.axis_info[0].fuzz = 0;
		info.axis_info[0].flat = 0;
		info.axis_info[1].name = "Y";
		info.axis_info[1].type = WMPLUGIN_ABS;
		info.axis_info[1].max  = 768;
		info.axis_info[1].min  = 0;
		info.axis_info[1].fuzz = 0;
		info.axis_info[1].flat = 0;
		info.param_count = 0;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	data.buttons = 0;

	if (wmplugin_set_report_mode(id, CWIID_RPT_IR)) {
		return -1;
	}

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg *mesg[])
{
	int i;
	uint8_t flags;
	static uint8_t old_flags;
	struct cwiid_ir_mesg *ir_mesg;

	ir_mesg = NULL;
	for (i=0; i < mesg_count; i++) {
		if (mesg[i]->type == CWIID_MESG_IR) {
			ir_mesg = &mesg[i]->ir_mesg;
		}
	}

	if (!ir_mesg) {
		return NULL;
	}

	/* update history */
	prev_a = a;
	prev_b = b;
	prev_c = c;

	/* invalidate a & b indices if sources are no longer present */
	if ((a_index != -1) && (!ir_mesg->src[a_index].valid)) {
		a_index = -1;
	}
	if ((b_index != -1) && (!ir_mesg->src[b_index].valid)) {
		b_index = -1;
	}

	/* of not set, pick largest available source for a & b */
	if (a_index == -1) {
		for (i=0; i < CWIID_IR_SRC_COUNT; i++) {
			if ((ir_mesg->src[i].valid) && (i != b_index)) {
				if ((a_index == -1) ||
				  (ir_mesg->src[i].size > ir_mesg->src[a_index].size)) {
					a_index = i;
				}
			}
		}
	}
	/* if there is no current src_b, pick the largest valid one */
	if (b_index == -1) {
		for (i=0; i < CWIID_IR_SRC_COUNT; i++) {
			if ((ir_mesg->src[i].valid) && (i != a_index)) {
				if ((b_index == -1) ||
				  (ir_mesg->src[i].size > ir_mesg->src[b_index].size)) {
					b_index = i;
				}
			}
		}
	}

#define DEBOUNCE_THRESHOLD 50

	/* set a & b, mirror the x coordinates */
	if (a_index == -1) {
		a_debounce++;
		if( a_debounce > DEBOUNCE_THRESHOLD ) {
			a.valid = 0;
		}
		else {
			a = prev_a;
		}
	}
	else {
		a = ir_mesg->src[a_index];
		a.pos[CWIID_X] = CWIID_IR_X_MAX - a.pos[CWIID_X];
		a_debounce = 0;
	}
	if (b_index == -1) {
		b_debounce++;
		if( b_debounce > DEBOUNCE_THRESHOLD ) {
			b.valid = 0;
		}
		else {
			b = prev_b;
		}
	}
	else {
		b = ir_mesg->src[b_index];
		b.pos[CWIID_X] = CWIID_IR_X_MAX - b.pos[CWIID_X];
		b_debounce = 0;
	}

	/* if both sources are valid, calculate the center */
	if (a.valid && b.valid) {
		c.valid = 1;
		for (i=0; i < 2; i++) {
			c.pos[i] = (a.pos[i] + b.pos[i])/2;
		}
	}
	/* if either source is valid, use best guess */
	else if (a.valid) {
		/* if a isn't new, and we have a previous center,
		 * assume source-center relationship holds */
		if (prev_a.valid && prev_c.valid) {
			c.valid = 1;
			for (i=0; i < 2; i++) {
				c.pos[i] = a.pos[i] + (prev_c.pos[i] - prev_a.pos[i]);
			}
		}
		/* if a is new or we don't have a previous center,
		 * use a as the center */
		else {
			c.valid = 1;
			for (i=0; i < 2; i++) {
				c.pos[i] = a.pos[i];
			}
		}
	}
	else if (b.valid) {
		/* if b isn't new, and we have a previous center,
		 * assume source-center relationship holds */
		if (prev_b.valid && prev_c.valid) {
			c.valid = 1;
			for (i=0; i < 2; i++) {
				c.pos[i] = b.pos[i] + (prev_c.pos[i] - prev_b.pos[i]);
			}
		}
		/* if b is new or we don't have a previous center,
		 * use b as the center */
		else {
			c.valid = 1;
			for (i=0; i < 2; i++) {
				c.pos[i] = b.pos[i];
			}
		}
	}
	/* no sources, no guesses */
	else {
		c.valid = 0;
	}

	/* LEDs */
	flags = 0;
	if ((a_index == 1) || (b_index == 1)) {
		flags |= CWIID_LED1_ON;
	}
	else if ((a_index == 2) || (b_index == 2)) {
		flags |= CWIID_LED2_ON;
	}
	else if ((a_index == 3) || (b_index == 3)) {
		flags |= CWIID_LED3_ON;
	}
	else if ((a_index == 4) || (b_index == 4)) {
		flags |= CWIID_LED4_ON;
	}
	if (flags != old_flags) {
		cwiid_set_led(wiimote, flags);
	}
	old_flags = flags;

	data.axes[0].valid = data.axes[1].valid = c.valid;

#define NEW_AMOUNT 0.6
#define OLD_AMOUNT (1.0-NEW_AMOUNT)

	data.axes[0].value = c.pos[CWIID_X]*NEW_AMOUNT +
	                     data.axes[0].value*OLD_AMOUNT;
	data.axes[1].value = c.pos[CWIID_Y]*NEW_AMOUNT +
	                     data.axes[1].value*OLD_AMOUNT;

	return &data;
}
