#include "wmplugin.h"

cwiid_wiimote_t *wiimote;

static unsigned char info_init = 0;
static struct wmplugin_info info;
static struct wmplugin_data data;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;

static int led1 = 0;
static int led2 = 0;
static int led3 = 0;
static int led4 = 0;

struct wmplugin_info *wmplugin_info()
{
	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 0;
		info.param_count = 4;
		info.param_info[0].name = "led1";
		info.param_info[0].type = WMPLUGIN_PARAM_INT;
		info.param_info[0].ptr = &led1;
		info.param_info[1].name = "led2";
		info.param_info[1].type = WMPLUGIN_PARAM_INT;
		info.param_info[1].ptr = &led2;
		info.param_info[2].name = "led3";
		info.param_info[2].type = WMPLUGIN_PARAM_INT;
		info.param_info[2].ptr = &led3;
		info.param_info[3].name = "led4";
		info.param_info[3].type = WMPLUGIN_PARAM_INT;
		info.param_info[3].ptr = &led4;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	uint8_t led_state = (led1 ? CWIID_LED1_ON : 0)
	                  | (led2 ? CWIID_LED2_ON : 0)
	                  | (led3 ? CWIID_LED3_ON : 0)
	                  | (led4 ? CWIID_LED4_ON : 0);

	cwiid_command(wiimote, CWIID_CMD_LED, led_state);

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[])
{
	return &data;
}
