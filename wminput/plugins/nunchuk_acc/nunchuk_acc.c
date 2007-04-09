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
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * type audit (stdint, const, char booleans)
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * made global variables static
 */

#include <math.h>

#include "wmplugin.h"

#define PI	3.14159265358979323

struct acc {
	uint8_t x;
	uint8_t y;
	uint8_t z;
};

static unsigned char info_init = 0;
static struct wmplugin_info info;
static struct wmplugin_data data;

static wiimote_t *wiimote;

static struct acc acc_zero, acc_one;
static int plugin_id;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;

static void process_nunchuk(struct wiimote_nunchuk_mesg *mesg);

struct wmplugin_info *wmplugin_info() {
	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 4;
		info.axis_info[0].name = "Roll";
		info.axis_info[0].type = WMPLUGIN_ABS;
		info.axis_info[0].max  =  3141;
		info.axis_info[0].min  = -3141;
		info.axis_info[0].fuzz = 0;
		info.axis_info[0].flat = 0;
		info.axis_info[1].name = "Pitch";
		info.axis_info[1].type = WMPLUGIN_ABS;
		info.axis_info[1].max  =  1570;
		info.axis_info[1].min  = -1570;
		info.axis_info[1].fuzz = 0;
		info.axis_info[1].flat = 0;
		info.axis_info[2].name = "X";
		info.axis_info[2].type = WMPLUGIN_ABS | WMPLUGIN_REL;
		info.axis_info[2].max  =  16;
		info.axis_info[2].min  = -16;
		info.axis_info[2].fuzz = 0;
		info.axis_info[2].flat = 0;
		info.axis_info[3].name = "Y";
		info.axis_info[3].type = WMPLUGIN_ABS | WMPLUGIN_REL;
		info.axis_info[3].max  =  16;
		info.axis_info[3].min  = -16;
		info.axis_info[3].fuzz = 0;
		info.axis_info[3].flat = 0;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, wiimote_t *wiimote_arg)
{
	plugin_id = id;
	wiimote = wiimote_arg;
	data.buttons = 0;
	data.axes[0].valid = 1;
	data.axes[1].valid = 1;
	if (wmplugin_set_report_mode(id,
	                             WIIMOTE_RPT_STATUS | WIIMOTE_RPT_NUNCHUK)) {
		return -1;
	}

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union wiimote_mesg *mesg[])
{
	int i;
	enum wiimote_ext_type extension = WIIMOTE_EXT_NONE;
	unsigned char buf[7];
	struct wmplugin_data *ret = NULL;

	for (i=0; i < mesg_count; i++) {
		switch (mesg[i]->type) {
		case WIIMOTE_MESG_STATUS:
			if ((mesg[i]->status_mesg.extension == WIIMOTE_EXT_NUNCHUK) &&
			  (extension != WIIMOTE_EXT_NUNCHUK)) {
				if (wiimote_read(wiimote, WIIMOTE_RW_REG | WIIMOTE_RW_DECODE,
				                 0xA40020, 7, buf)) {
					wmplugin_err(plugin_id, "unable to read wiimote info");
				}
				acc_zero.x = buf[0];
				acc_zero.y = buf[1];
				acc_zero.z = buf[2];
				acc_one.x  = buf[4];
				acc_one.y  = buf[5];
				acc_one.z  = buf[6];
			}
			extension = mesg[i]->status_mesg.extension;
			break;
		case WIIMOTE_MESG_NUNCHUK:
			process_nunchuk(&mesg[i]->nunchuk_mesg);
			ret = &data;
			break;
		default:
			break;
		}
	}

	return ret;
}

#define NEW_AMOUNT 0.1
#define OLD_AMOUNT (1.0-NEW_AMOUNT)
double a_x = 0, a_y = 0, a_z = 0;

static void process_nunchuk(struct wiimote_nunchuk_mesg *mesg)
{
	double a;
	double roll, pitch;

	a_x = (((double)mesg->acc_x - acc_zero.x) /
	      (acc_one.x - acc_zero.x))*NEW_AMOUNT + a_x*OLD_AMOUNT;
	a_y = (((double)mesg->acc_y - acc_zero.y) /
	      (acc_one.y - acc_zero.y))*NEW_AMOUNT + a_y*OLD_AMOUNT;
	a_z = (((double)mesg->acc_z - acc_zero.z) /
	      (acc_one.z - acc_zero.z))*NEW_AMOUNT + a_z*OLD_AMOUNT;

	a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
	roll = atan(a_x/a_z);
	if (a_z <= 0.0) {
		roll += PI * ((a_x > 0.0) ? 1 : -1);
	}

	pitch = atan(a_y/a_z*cos(roll));

	data.axes[0].value = roll*1000;
	data.axes[1].value = pitch*1000;

	if ((a > 0.85) && (a < 1.15)) {
		if ((fabs(roll)*(180/PI) > 10) && (fabs(pitch)*(180/PI) < 80)) {
			data.axes[2].valid = 1;
			data.axes[2].value = roll * 5;
		}
		else {
			data.axes[2].valid = 0;
		}
		if (fabs(pitch)*(180/PI) > 10) {
			data.axes[3].valid = 1;
			data.axes[3].value = pitch * 10;
		}
		else {
			data.axes[3].valid = 0;
		}
	}
	else {
		data.axes[2].valid = 0;
		data.axes[3].valid = 0;
	}
}

