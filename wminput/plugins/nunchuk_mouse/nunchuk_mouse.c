/* Copyright (C) 2009 Jonas Kölker <jonaskoelker@gnu.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA
 */

#include <math.h>

#include "wmplugin.h"
#include <cwiid.h>
#include "nunchuk_mouse.h"

static void process_nunchuk(struct cwiid_nunchuk_mesg *mesg)
{
    /* st(x|y): stick value along direction */
	const uint8_t stx = mesg->stick[CWIID_X];
	const uint8_t sty = mesg->stick[CWIID_Y];
    /* d(x|y)(n|p): "delta x, direction=negative", etc. */
    const int dxn = (center_x - DEADZONE) - stx;
    const int dxp = stx - (center_x + DEADZONE);
    const int dyn = (center_y - DEADZONE) - sty;
    const int dyp = sty - (center_y + DEADZONE);

    /* TODO: with -REL_Y, we go down slower than we go up. */
    data.buttons = 0;
    /* += (vs =): to accumulate the result of multiple events. */
    data.axes[X].value +=
      (dxn >= 0)? -(dxn * RANGE / x_neg_range):
      (dxp >= 0)? +(dxp * RANGE / x_pos_range): 0;
    data.axes[Y].value +=
      (dyn >= 0)? -(dyn * RANGE / y_neg_range):
      (dyp >= 0)? +(dyp * RANGE / y_pos_range): 0;
}

static void calibrate_joystick() {
    int ret;
    uint8_t buf[6];

    /* http://abstrakraft.org/cwiid/ticket/40 */
    ret = cwiid_read(wiimote, CWIID_RW_REG | CWIID_RW_DECODE,
                     0xA40028, sizeof buf, buf);
    if (ret) wmplugin_err(plugin_id, "joystick calibration error");

    center_x = buf[OFF_X + OFF_CENTER];
    x_neg_range = (center_x - DEADZONE) - buf[OFF_X + OFF_MIN];
    x_pos_range = buf[OFF_X + OFF_MAX] - (center_x + DEADZONE);

    center_y = buf[OFF_Y + OFF_CENTER];
    y_neg_range = (center_y - DEADZONE) - buf[OFF_Y + OFF_MIN]; // this is wrong
    y_pos_range = buf[OFF_Y + OFF_MAX] - (center_y + DEADZONE);
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[])
{ 
  int i;
  enum cwiid_ext_type ext_type = CWIID_EXT_NONE;
  struct wmplugin_data *ret = NULL;
  
  data.axes[X].value = data.axes[Y].value = 0;
  for (i=0; i < mesg_count; i++) {
    switch (mesg[i].type) {
    case CWIID_MESG_STATUS:
      if ((mesg[i].status_mesg.ext_type == CWIID_EXT_NUNCHUK) &&
	  (ext_type != CWIID_EXT_NUNCHUK)) {
	calibrate_joystick();
      }
      ext_type = mesg[i].status_mesg.ext_type;
      break;
    case CWIID_MESG_NUNCHUK:
      process_nunchuk(&mesg[i].nunchuk_mesg);
      ret = &data;
      break;
    default:
      break;
    }
  }
  return ret;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	plugin_id = id;
	wiimote = arg_wiimote;
	data.buttons = 0;
	data.axes[0].valid = 1;
	data.axes[1].valid = 1;
	if (wmplugin_set_rpt_mode(id, CWIID_RPT_STATUS | CWIID_RPT_NUNCHUK)) {
		return -1;
	}

	return 0;
}

struct wmplugin_info *wmplugin_info() {
  
    if (info_init) return &info;
	info.button_count = 0;
	info.axis_count = 2;

    info.axis_info[X].name = "X";
	info.axis_info[X].type = WMPLUGIN_REL;
	info.axis_info[X].min  = -RANGE;
	info.axis_info[X].max  = +RANGE;
	info.axis_info[X].fuzz = 0;
	info.axis_info[X].flat = 0;

    info.axis_info[Y].name = "Y";
	info.axis_info[Y].type = WMPLUGIN_REL;
	info.axis_info[Y].min  = -RANGE;
	info.axis_info[Y].max  = +RANGE;
	info.axis_info[Y].fuzz = 0;
	info.axis_info[Y].flat = 0;

    info_init = 1;
	return &info;
}
