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
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * type audit (stdint, const, char booleans)
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * made global variables static
 */

#include "wmplugin.h"

wiimote_t *wiimote;

struct cursor {
	unsigned char valid;
	uint16_t x;
	uint16_t y;
};

/* static objects are initialized to 0 by default */
static int a_index = -1, b_index = -1;
static struct wiimote_ir_src a, b, prev_a, prev_b;

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
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	data.buttons = 0;

	if (wmplugin_set_report_mode(id, WIIMOTE_RPT_IR)) {
		return -1;
	}

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union wiimote_mesg *mesg[])
{
	int i;
	uint8_t flags;
	static uint8_t old_flags;
	struct wiimote_ir_mesg *ir_mesg;

	ir_mesg = NULL;
	for (i=0; i < mesg_count; i++) {
		if (mesg[i]->type == WIIMOTE_MESG_IR) {
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
		for (i=0; i < WIIMOTE_IR_SRC_COUNT; i++) {
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
		for (i=0; i < WIIMOTE_IR_SRC_COUNT; i++) {
			if ((ir_mesg->src[i].valid) && (i != a_index)) {
				if ((b_index == -1) ||
				  (ir_mesg->src[i].size > ir_mesg->src[b_index].size)) {
					b_index = i;
				}
			}
		}
	}
	/* set a & b, mirror the x coordinates */
	if (a_index == -1) {
		a.valid = 0;
	}
	else {
		a = ir_mesg->src[a_index];
		a.x = WIIMOTE_IR_X_MAX - a.x;
	}
	if (b_index == -1) {
		b.valid = 0;
	}
	else {
		b = ir_mesg->src[b_index];
		b.x = WIIMOTE_IR_X_MAX - b.x;
	}

	/* if both sources are valid, calculate the center */
	if (a.valid && b.valid) {
		c.valid = 1;
		c.x = (a.x + b.x)/2;
		c.y = (a.y + b.y)/2;
	}
	/* if either source is valid, use best guess */
	else if (a.valid) {
		/* if a isn't new, and we have a previous center,
		 * assume source-center relationship holds */
		if (prev_a.valid && prev_c.valid) {
			c.valid = 1;
			c.x = a.x + (prev_c.x - prev_a.x);
			c.y = a.y + (prev_c.y - prev_a.y);
		}
		/* if a is new or we don't have a previous center,
		 * use a as the center */
		else {
			c.valid = 1;
			c.x = a.x;
			c.y = a.y;
		}
	}
	else if (b.valid) {
		/* if b isn't new, and we have a previous center,
		 * assume source-center relationship holds */
		if (prev_b.valid && prev_c.valid) {
			c.valid = 1;
			c.x = b.x + (prev_c.x - prev_b.x);
			c.y = b.y + (prev_c.y - prev_b.y);
		}
		/* if b is new or we don't have a previous center,
		 * use b as the center */
		else {
			c.valid = 1;
			c.x = b.x;
			c.y = b.y;
		}
	}
	/* no sources, no guesses */
	else {
		c.valid = 0;
	}

	/* LEDs */
	flags = 0;
	if ((a_index == 1) || (b_index == 1)) {
		flags |= WIIMOTE_LED1_ON;
	}
	else if ((a_index == 2) || (b_index == 2)) {
		flags |= WIIMOTE_LED2_ON;
	}
	else if ((a_index == 3) || (b_index == 3)) {
		flags |= WIIMOTE_LED3_ON;
	}
	else if ((a_index == 4) || (b_index == 4)) {
		flags |= WIIMOTE_LED4_ON;
	}
	if (flags != old_flags) {
		/* TODO: if this message is sent every time, we get a battery meter of
		 * blinking lights - why? */
		wiimote_command(wiimote, WIIMOTE_CMD_LED, flags);
	}
	old_flags = flags;

	data.axes[0].valid = data.axes[1].valid = c.valid;
	data.axes[0].value = c.x;
	data.axes[1].value = c.y;

	return &data;
}

