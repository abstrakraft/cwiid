/* Copyright (C) 2010 Stefan Tomanek <stefan@pico.ruhr.de>
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
 */

#include <math.h>

#include "wmplugin.h"

#define SCREWDRIVER_RIGHT 0x0001
#define SCREWDRIVER_LEFT  0x0002
#define SCREWDRIVER_UP    0x0004
#define SCREWDRIVER_DOWN  0x0008

#define PI	3.14159265358979323

static unsigned char info_init = 0;
static struct wmplugin_info info;
static struct wmplugin_data data;

static struct acc_cal acc_cal;

static int plugin_id;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;
static void process_btn(struct cwiid_btn_mesg *mesg);
static void process_acc(struct cwiid_acc_mesg *mesg);

static int btn_active = 0;
static uint8_t btn_id = CWIID_BTN_B;

static double last_roll = 0.0;
static double last_pitch = 0.0;
static float roll_threshold = 0.25;
static float pitch_threshold = 0.25;

struct wmplugin_info *wmplugin_info() {
	if (!info_init) {
		info.button_count = 4;
		info.button_info[0].name = "Right";
		info.button_info[1].name = "Left";
		info.button_info[2].name = "Up";
		info.button_info[3].name = "Down";
		info.param_count = 3;
		info.param_info[0].name = "Button";
		info.param_info[0].type = WMPLUGIN_PARAM_INT;
		info.param_info[0].ptr = &btn_id;
		info.param_info[1].name = "RollThreshold";
		info.param_info[1].type = WMPLUGIN_PARAM_FLOAT;
		info.param_info[1].ptr = &roll_threshold;
		info.param_info[2].name = "PitchThreshold";
		info.param_info[2].type = WMPLUGIN_PARAM_FLOAT;
		info.param_info[2].ptr = &pitch_threshold;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *wiimote)
{
	plugin_id = id;

	data.buttons = 0;
	if (wmplugin_set_rpt_mode(id, CWIID_RPT_ACC | CWIID_RPT_BTN)) {
		return -1;
	}

	if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NONE, &acc_cal)) {
		wmplugin_err(id, "calibration error");
		return -1;
	}

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[])
{
	int i;
	struct wmplugin_data *ret = NULL;

	for (i=0; i < mesg_count; i++) {
		switch (mesg[i].type) {
		case CWIID_MESG_BTN:
			process_btn(&mesg[i].btn_mesg);
			break;
		case CWIID_MESG_ACC:
			process_acc(&mesg[i].acc_mesg);
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

static void process_btn(struct cwiid_btn_mesg *mesg)
{
	btn_active = (mesg->buttons & btn_id);
}

static void process_acc(struct cwiid_acc_mesg *mesg)
{
	double a;
	double roll, pitch;
	double d_roll, d_pitch;

	data.buttons = 0;

	a_x = (((double)mesg->acc[CWIID_X] - acc_cal.zero[CWIID_X]) /
	      (acc_cal.one[CWIID_X] - acc_cal.zero[CWIID_X]))*NEW_AMOUNT +
	      a_x*OLD_AMOUNT;
	a_y = (((double)mesg->acc[CWIID_Y] - acc_cal.zero[CWIID_Y]) /
	      (acc_cal.one[CWIID_Y] - acc_cal.zero[CWIID_Y]))*NEW_AMOUNT +
	      a_y*OLD_AMOUNT;
	a_z = (((double)mesg->acc[CWIID_Z] - acc_cal.zero[CWIID_Z]) /
	      (acc_cal.one[CWIID_Z] - acc_cal.zero[CWIID_Z]))*NEW_AMOUNT +
	      a_z*OLD_AMOUNT;

	a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
	roll = atan(a_x/a_z);
	pitch = atan(a_y/a_z*cos(roll));

	if (a_z <= 0.0) {
		roll += PI * ((a_x > 0.0) ? 1 : -1);
	}

	if (!btn_active) {
		last_roll = roll;
		last_pitch = pitch;
	} else {
		d_roll = roll - last_roll;
		if ( fabs(d_roll) > roll_threshold ) {
			last_roll = roll;
			data.buttons |= ( (d_roll>0)? SCREWDRIVER_RIGHT : SCREWDRIVER_LEFT );
		}

		d_pitch = pitch - last_pitch;
		if ( fabs(d_pitch) > pitch_threshold ) {
			last_pitch = pitch;
			data.buttons |= ( (d_pitch>0)? SCREWDRIVER_UP : SCREWDRIVER_DOWN );
		}
	}
}

