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

#ifndef WMPLUGIN_H
#define WMPLUGIN_H

#include <wiimote.h>

#define WMPLUGIN_MAX_BUTTON_COUNT	16
#define WMPLUGIN_MAX_AXIS_COUNT		6

#define WMPLUGIN_ABS	1
#define WMPLUGIN_REL	2

struct wmplugin_button_info {
	char *name;
};

struct wmplugin_axis_info {
	char *name;
	int type;
	int max;
	int min;
	int fuzz;
	int flat;
};

struct wmplugin_info {
	unsigned int button_count;
	struct wmplugin_button_info button_info[WMPLUGIN_MAX_BUTTON_COUNT];
	unsigned int axis_count;
	struct wmplugin_axis_info axis_info[WMPLUGIN_MAX_AXIS_COUNT];
};

struct wmplugin_axis {
	int valid;
	int value;
};

struct wmplugin_data {
	unsigned short buttons;
	struct wmplugin_axis axes[WMPLUGIN_MAX_AXIS_COUNT];
};

typedef struct wmplugin_info *wmplugin_info_t(void);
typedef int wmplugin_init_t(int id, wiimote_t *wiimote);
typedef struct wmplugin_data *wmplugin_exec_t(int, union wiimote_mesg * []);

int wmplugin_set_report_mode(int id, unsigned int flags);
void wmplugin_err(int id, char *str, ...);

#endif
