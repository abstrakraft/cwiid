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

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <wiimote.h>

#include "conf.h"
#include "util.h"
#include "wmplugin.h"

struct conf conf;

/* GetOpt */
#define OPTSTRING	"hc:"
extern char *optarg;
extern int optind, opterr, optopt;

/* Prototypes */
wiimote_mesg_callback_t wiimote_callback;
int wminput_set_report_mode();
void process_btn_mesg(struct wiimote_btn_mesg *mesg);
void process_nunchuk_mesg(struct wiimote_nunchuk_mesg *mesg);
void process_classic_mesg(struct wiimote_classic_mesg *mesg);
void process_plugin(struct plugin *, int, union wiimote_mesg * []);

/* Globals */
wiimote_t *wiimote;
int init;

#ifndef GLOBAL_CWIID_DIR
#error Global plugin directory macro undefined
#endif

#define DEFAULT_CONFIG_FILE	"default"

#define USAGE "usage:%s [-h] [-c config] [bdaddr]\n"

#define GLOBAL_CONFIG_DIR	GLOBAL_CWIID_DIR "/wminput"
#define GLOBAL_PLUGIN_DIR	GLOBAL_CWIID_DIR "/plugins"

#define HOME_DIR_LEN	128
int main(int argc, char *argv[])
{
	char *config_search_dirs[3], *plugin_search_dirs[3];
	char *config_filename = DEFAULT_CONFIG_FILE;
	char home_config_dir[HOME_DIR_LEN];
	char home_plugin_dir[HOME_DIR_LEN];
	char *tmp;
	int c, i;
	char *str_addr;
	bdaddr_t bdaddr;
	sigset_t sigset;
	int signum, ret=0;
	struct uinput_listen_data uinput_listen_data;
	pthread_t uinput_listen_thread;

	init = -1;

	/* Parse Options */
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (c) {
		case 'h':
			printf(USAGE, argv[0]);
			return 0;
			break;
		case 'c':
			config_filename = optarg;
			break;
		case '?':
		default:
			wminput_err("unknown command-line option: -%c", c);
			break;
		}
	}

	/* Load Config */
	if ((tmp = getenv("HOME")) == NULL) {
		wminput_err("unable to find home directory");
		config_search_dirs[0] = GLOBAL_CONFIG_DIR;
		plugin_search_dirs[0] = GLOBAL_PLUGIN_DIR;
		config_search_dirs[1] = plugin_search_dirs[1] = NULL;
	}
	else {
		snprintf(home_config_dir, HOME_DIR_LEN, "%s/.CWiid/wminput", tmp);
		snprintf(home_plugin_dir, HOME_DIR_LEN, "%s/.CWiid/plugins", tmp);
		config_search_dirs[0] = home_config_dir;
		plugin_search_dirs[0] = home_plugin_dir;
		config_search_dirs[1] = GLOBAL_CONFIG_DIR;
		plugin_search_dirs[1] = GLOBAL_PLUGIN_DIR;
		config_search_dirs[2] = plugin_search_dirs[2] = NULL;
	}

	if (conf_load(&conf, config_filename, config_search_dirs,
	  plugin_search_dirs)) {
		return -1;
	}

	/* BDADDR */
	if (optind < argc) {
		if (str2ba(argv[optind], &bdaddr)) {
			wminput_err("invalid bdaddr");
			bdaddr = *BDADDR_ANY;
		}
		optind++;
		if (optind < argc) {
			wminput_err("invalid command-line");
			printf(USAGE, argv[0]);
			conf_unload(&conf);
			return -1;
		}
	}
	else if ((str_addr = getenv(WIIMOTE_BDADDR)) != NULL) {
		if (str2ba(str_addr, &bdaddr)) {
			wminput_err("invalid address in %s", WIIMOTE_BDADDR);
			bdaddr = *BDADDR_ANY;
		}
	}
	else {
		bdaddr = *BDADDR_ANY;
	}

	/* Wiimote connect */
	printf("Put Wiimote in discoverable mode now (press 1+2)...\n");
	if ((wiimote = wiimote_connect(bdaddr, wiimote_callback, NULL)) == NULL) {
		wminput_err("unable to connect");
		conf_unload(&conf);
		return -1;
	}

	/* init plugins */
	for (i=0; (i < CONF_MAX_PLUGINS) && conf.plugins[i].name; i++) {
		if ((*conf.plugins[i].init)(i, wiimote)) {
			wminput_err("error on %s init", conf.plugins[i].name);
			conf_unload(&conf);
			wiimote_disconnect(wiimote);
			return -1;
		}
	}

	if (wminput_set_report_mode()) {
		conf_unload(&conf);
		wiimote_disconnect(wiimote);
		return -1;
	}

	uinput_listen_data.wiimote = wiimote;
	uinput_listen_data.conf = &conf;
	if (pthread_create(&uinput_listen_thread, NULL,
	                   (void *(*)(void *))uinput_listen,
	                   &uinput_listen_data)) {
		wminput_err("error starting uinput listen thread");
		conf_unload(&conf);
		wiimote_disconnect(wiimote);
		return -1;
	}


	printf("Ready.\n");

	init = 0;

	/* wait */
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGINT);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	sigwait(&sigset, &signum);

	printf("Exiting.\n");

	if (pthread_cancel(uinput_listen_thread)) {
		wminput_err("error canceling uinput listen thread");
		ret = -1;
	}
	else if (pthread_join(uinput_listen_thread, NULL)) {
		wminput_err("error joing uinput listen thread");
		ret = -1;
	}

	/* disconnect */
	if (wiimote_disconnect(wiimote)) {
		wminput_err("error on disconnect");
		ret = -1;
	}

	if (conf_unload(&conf)) {
		ret = -1;
	}

	return ret;
}

int wmplugin_set_report_mode(int id, unsigned int flags)
{
	conf.plugins[id].rpt_mode_flags = flags;

	if (!init) {
		wminput_set_report_mode();
	}

	return 0;
}

int wminput_set_report_mode()
{
	unsigned char rpt_mode_flags;
	int i;

	rpt_mode_flags = conf.rpt_mode_flags;

	for (i=0; (i < CONF_MAX_PLUGINS) && conf.plugins[i].name; i++) {
		rpt_mode_flags |= conf.plugins[i].rpt_mode_flags;
	}

	if (wiimote_command(wiimote, WIIMOTE_CMD_RPT_MODE, rpt_mode_flags)) {
		wminput_err("error setting report mode");
		return -1;
	}

	return 0;
}

void wiimote_callback(int id, int mesg_count, union wiimote_mesg *mesg[])
{
	int i;

	for (i=0; i < mesg_count; i++) {
		switch (mesg[i]->type) {
		case WIIMOTE_MESG_BTN:
			process_btn_mesg((struct wiimote_btn_mesg *) mesg[i]);
			break;
		case WIIMOTE_MESG_NUNCHUK:
			process_nunchuk_mesg((struct wiimote_nunchuk_mesg *) mesg[i]);
			break;
		case WIIMOTE_MESG_CLASSIC:
			process_classic_mesg((struct wiimote_classic_mesg *) mesg[i]);
			break;
		default:
			break;
		}
	}
	for (i=0; (i < CONF_MAX_PLUGINS) && conf.plugins[i].name; i++) {
		process_plugin(&conf.plugins[i], mesg_count, mesg);
	}
	send_event(&conf, EV_SYN, SYN_REPORT, 0);
}

void process_btn_mesg(struct wiimote_btn_mesg *mesg)
{
	static unsigned short prev_buttons = 0;
	unsigned short pressed, released;
	int axis_value;
	int i;

	/* Wiimote Button/Key Events */
	pressed = mesg->buttons & ~prev_buttons;
	released = ~mesg->buttons & prev_buttons;
	for (i=0; i < CONF_WM_BTN_COUNT; i++) {
		if (pressed & conf.wiimote_bmap[i].mask) {
			send_event(&conf, EV_KEY, conf.wiimote_bmap[i].action, 1);
		}
		else if (released & conf.wiimote_bmap[i].mask) {
			send_event(&conf, EV_KEY, conf.wiimote_bmap[i].action, 0);
		}
	}
	prev_buttons = mesg->buttons;

	/* Wiimote.Dpad.X */
	if (conf.amap[CONF_WM_AXIS_DPAD_X].action != -1) {
		axis_value = 0;
		if (mesg->buttons & WIIMOTE_BTN_LEFT) {
			axis_value = -1;
		}
		else if (mesg->buttons & WIIMOTE_BTN_RIGHT) {
			axis_value = 1;
		}
		if (conf.amap[CONF_WM_AXIS_DPAD_X].flags & CONF_INVERT) {
			axis_value *= -1;
		}
		send_event(&conf, conf.amap[CONF_WM_AXIS_DPAD_X].axis_type,
		           conf.amap[CONF_WM_AXIS_DPAD_X].action, axis_value);
	}

	/* Wiimote.Dpad.Y */
	if (conf.amap[CONF_WM_AXIS_DPAD_Y].action != -1) {
		axis_value = 0;
		if (mesg->buttons & WIIMOTE_BTN_DOWN) {
			axis_value = -1;
		}
		else if (mesg->buttons & WIIMOTE_BTN_UP) {
			axis_value = 1;
		}
		if (conf.amap[CONF_WM_AXIS_DPAD_Y].flags & CONF_INVERT) {
			axis_value *= -1;
		}
		send_event(&conf, conf.amap[CONF_WM_AXIS_DPAD_Y].axis_type,
		           conf.amap[CONF_WM_AXIS_DPAD_Y].action, axis_value);
	}
}

void process_nunchuk_mesg(struct wiimote_nunchuk_mesg *mesg)
{
	static unsigned char prev_buttons = 0;
	unsigned char pressed, released;
	int axis_value;
	int i;

	/* Nunchuk Button/Key Events */
	pressed = mesg->buttons & ~prev_buttons;
	released = ~mesg->buttons & prev_buttons;
	for (i=0; i < CONF_NC_BTN_COUNT; i++) {
		if (pressed & conf.nunchuk_bmap[i].mask) {
			send_event(&conf, EV_KEY, conf.nunchuk_bmap[i].action, 1);
		}
		else if (released & conf.nunchuk_bmap[i].mask) {
			send_event(&conf, EV_KEY, conf.nunchuk_bmap[i].action, 0);
		}
	}
	prev_buttons = mesg->buttons;

	/* Nunchuk.Stick.X */
	if (conf.amap[CONF_NC_AXIS_STICK_X].action != -1) {
		axis_value = mesg->stick_x;
		if (conf.amap[CONF_NC_AXIS_STICK_X].flags & CONF_INVERT) {
			axis_value = 0xFF - axis_value;
		}
		send_event(&conf, conf.amap[CONF_NC_AXIS_STICK_X].axis_type,
		           conf.amap[CONF_NC_AXIS_STICK_X].action, axis_value);
	}

	/* Nunchuk.Stick.Y */
	if (conf.amap[CONF_NC_AXIS_STICK_Y].action != -1) {
		axis_value = mesg->stick_y;
		if (conf.amap[CONF_NC_AXIS_STICK_Y].flags & CONF_INVERT) {
			axis_value = 0xFF - axis_value;
		}
		send_event(&conf, conf.amap[CONF_NC_AXIS_STICK_Y].axis_type,
		           conf.amap[CONF_NC_AXIS_STICK_Y].action, axis_value);
	}
}

void process_classic_mesg(struct wiimote_classic_mesg *mesg)
{
	static unsigned short prev_buttons = 0;
	unsigned short pressed, released;
	int axis_value;
	int i;

	/* Classic Button/Key Events */
	pressed = mesg->buttons & ~prev_buttons;
	released = ~mesg->buttons & prev_buttons;
	for (i=0; i < CONF_CC_BTN_COUNT; i++) {
		if (pressed & conf.classic_bmap[i].mask) {
			send_event(&conf, EV_KEY, conf.classic_bmap[i].action, 1);
		}
		else if (released & conf.classic_bmap[i].mask) {
			send_event(&conf, EV_KEY, conf.classic_bmap[i].action, 0);
		}
	}
	prev_buttons = mesg->buttons;

	/* Classic.Dpad.X */
	if (conf.amap[CONF_CC_AXIS_DPAD_X].action != -1) {
		axis_value = 0;
		if (mesg->buttons & WIIMOTE_CLASSIC_BTN_LEFT) {
			axis_value = -1;
		}
		else if (mesg->buttons & WIIMOTE_CLASSIC_BTN_RIGHT) {
			axis_value = 1;
		}
		if (conf.amap[CONF_CC_AXIS_DPAD_X].flags & CONF_INVERT) {
			axis_value *= -1;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_DPAD_X].axis_type,
		           conf.amap[CONF_CC_AXIS_DPAD_X].action, axis_value);
	}

	/* Classic.Dpad.Y */
	if (conf.amap[CONF_CC_AXIS_DPAD_Y].action != -1) {
		axis_value = 0;
		if (mesg->buttons & WIIMOTE_CLASSIC_BTN_DOWN) {
			axis_value = -1;
		}
		else if (mesg->buttons & WIIMOTE_CLASSIC_BTN_UP) {
			axis_value = 1;
		}
		if (conf.amap[CONF_CC_AXIS_DPAD_Y].flags & CONF_INVERT) {
			axis_value *= -1;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_DPAD_Y].axis_type,
		           conf.amap[CONF_CC_AXIS_DPAD_Y].action, axis_value);
	}

	/* Classic.LStick.X */
	if (conf.amap[CONF_CC_AXIS_L_STICK_X].action != -1) {
		axis_value = mesg->l_stick_x;
		if (conf.amap[CONF_CC_AXIS_L_STICK_X].flags & CONF_INVERT) {
			axis_value = WIIMOTE_CLASSIC_L_STICK_MAX - axis_value;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_L_STICK_X].axis_type,
		           conf.amap[CONF_CC_AXIS_L_STICK_X].action, axis_value);
	}

	/* Classic.LStick.Y */
	if (conf.amap[CONF_CC_AXIS_L_STICK_Y].action != -1) {
		axis_value = mesg->l_stick_y;
		if (conf.amap[CONF_CC_AXIS_L_STICK_Y].flags & CONF_INVERT) {
			axis_value = WIIMOTE_CLASSIC_L_STICK_MAX - axis_value;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_L_STICK_Y].axis_type,
		           conf.amap[CONF_CC_AXIS_L_STICK_Y].action, axis_value);
	}

	/* Classic.RStick.X */
	if (conf.amap[CONF_CC_AXIS_R_STICK_X].action != -1) {
		axis_value = mesg->r_stick_x;
		if (conf.amap[CONF_CC_AXIS_R_STICK_X].flags & CONF_INVERT) {
			axis_value = WIIMOTE_CLASSIC_R_STICK_MAX - axis_value;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_R_STICK_X].axis_type,
		           conf.amap[CONF_CC_AXIS_R_STICK_X].action, axis_value);
	}

	/* Classic.RStick.Y */
	if (conf.amap[CONF_CC_AXIS_R_STICK_Y].action != -1) {
		axis_value = mesg->r_stick_y;
		if (conf.amap[CONF_CC_AXIS_R_STICK_Y].flags & CONF_INVERT) {
			axis_value = WIIMOTE_CLASSIC_R_STICK_MAX - axis_value;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_R_STICK_Y].axis_type,
		           conf.amap[CONF_CC_AXIS_R_STICK_Y].action, axis_value);
	}

	/* Classic.LAnalog */
	if (conf.amap[CONF_CC_AXIS_L].action != -1) {
		axis_value = mesg->l;
		if (conf.amap[CONF_CC_AXIS_L].flags & CONF_INVERT) {
			axis_value = WIIMOTE_CLASSIC_LR_MAX - axis_value;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_L].axis_type,
		           conf.amap[CONF_CC_AXIS_L].action, axis_value);
	}

	/* Classic.RAnalog */
	if (conf.amap[CONF_CC_AXIS_R].action != -1) {
		axis_value = mesg->r;
		if (conf.amap[CONF_CC_AXIS_R].flags & CONF_INVERT) {
			axis_value = WIIMOTE_CLASSIC_LR_MAX - axis_value;
		}
		send_event(&conf, conf.amap[CONF_CC_AXIS_R].axis_type,
		           conf.amap[CONF_CC_AXIS_R].action, axis_value);
	}
}

void process_plugin(struct plugin *plugin, int mesg_count,
                    union wiimote_mesg *mesg[])
{
	union wiimote_mesg *plugin_mesg[WIIMOTE_MAX_MESG_COUNT];
	int plugin_mesg_count = 0;
	int i;
	unsigned short flag;
	struct wmplugin_data *data;
	unsigned int pressed, released;
	int axis_value;

	for (i=0; i < mesg_count; i++) {
		switch (mesg[i]->type) {
		case WIIMOTE_MESG_STATUS:
			flag = WIIMOTE_RPT_STATUS;
			break;
		case WIIMOTE_MESG_BTN:
			flag = WIIMOTE_RPT_BTN;
			break;
		case WIIMOTE_MESG_ACC:
			flag = WIIMOTE_RPT_ACC;
			break;
		case WIIMOTE_MESG_IR:
			flag = WIIMOTE_RPT_IR;
			break;
		case WIIMOTE_MESG_NUNCHUK:
			flag = WIIMOTE_RPT_NUNCHUK;
			break;
		case WIIMOTE_MESG_CLASSIC:
			flag = WIIMOTE_RPT_CLASSIC;
			break;
		default:
			break;
		}
		if (plugin->rpt_mode_flags & flag) {
			plugin_mesg[plugin_mesg_count++] = mesg[i];
		}
	}

	if (plugin_mesg_count > 0) {
		if (!(data = (*plugin->exec)(plugin_mesg_count, plugin_mesg))) {
			return;
		}

		/* Plugin Button/Key Events */
		pressed = data->buttons & ~plugin->prev_buttons;
		released = ~data->buttons & plugin->prev_buttons;
		for (i=0; i < plugin->info->button_count; i++) {
			if (pressed & 1<<i) {
				send_event(&conf, EV_KEY, plugin->bmap[i].action, 1);
			}
			else if (released & 1<<i) {
				send_event(&conf, EV_KEY, plugin->bmap[i].action, 0);
			}
		}
		plugin->prev_buttons = data->buttons;

		/* Plugin Axis Events */
		for (i=0; i < plugin->info->axis_count; i++) {
			if ((plugin->amap[i].action != -1) && data->axes &&
			  data->axes[i].valid) {
				axis_value = data->axes[i].value;
				if (plugin->amap[i].flags & CONF_INVERT) {
					axis_value = plugin->info->axis_info[i].max +
					             plugin->info->axis_info[i].min - axis_value;
				}
				send_event(&conf, plugin->amap[i].axis_type,
				           plugin->amap[i].action, axis_value);
			}
		}
	}
}

