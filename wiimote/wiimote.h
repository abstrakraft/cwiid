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
 *  04/01/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote_info definition and macros
 *  * added wiimote_get_info_array prototype
 *  * changed wiimote_findfirst to wiimote_find_wiimote
 *
 *  03/05/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added wiimote_err_t definition
 *  * added wiimote_set_err prototype
 *
 *  03/01/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#ifndef WIIMOTE_H
#define WIIMOTE_H

#include <stdint.h>
#include <bluetooth/bluetooth.h>	/* bdaddr_t */

/* Report Mode Flags */
#define WIIMOTE_RPT_STATUS	0x01
#define WIIMOTE_RPT_BTN		0x02
#define WIIMOTE_RPT_ACC		0x04
#define WIIMOTE_RPT_IR		0x08
#define WIIMOTE_RPT_NUNCHUK	0x10
#define WIIMOTE_RPT_CLASSIC	0x20
#define WIIMOTE_RPT_EXT		(WIIMOTE_RPT_NUNCHUK | WIIMOTE_RPT_CLASSIC)

/* LED flags */
#define WIIMOTE_LED1_ON		0x01
#define WIIMOTE_LED2_ON		0x02
#define WIIMOTE_LED3_ON		0x04
#define WIIMOTE_LED4_ON		0x08

/* Button flags */
#define WIIMOTE_BTN_2		0x0001
#define WIIMOTE_BTN_1		0x0002
#define WIIMOTE_BTN_B		0x0004
#define WIIMOTE_BTN_A		0x0008
#define WIIMOTE_BTN_MINUS	0x0010
#define WIIMOTE_BTN_HOME	0x0080
#define WIIMOTE_BTN_LEFT	0x0100
#define WIIMOTE_BTN_RIGHT	0x0200
#define WIIMOTE_BTN_DOWN	0x0400
#define WIIMOTE_BTN_UP		0x0800
#define WIIMOTE_BTN_PLUS	0x1000

#define WIIMOTE_NUNCHUK_BTN_Z	0x01
#define WIIMOTE_NUNCHUK_BTN_C	0x02

#define WIIMOTE_CLASSIC_BTN_UP		0x0001
#define WIIMOTE_CLASSIC_BTN_LEFT	0x0002
#define WIIMOTE_CLASSIC_BTN_ZR		0x0004
#define WIIMOTE_CLASSIC_BTN_X		0x0008
#define WIIMOTE_CLASSIC_BTN_A		0x0010
#define WIIMOTE_CLASSIC_BTN_Y		0x0020
#define WIIMOTE_CLASSIC_BTN_B		0x0040
#define WIIMOTE_CLASSIC_BTN_ZL		0x0080
#define WIIMOTE_CLASSIC_BTN_R		0x0200
#define WIIMOTE_CLASSIC_BTN_PLUS	0x0400
#define WIIMOTE_CLASSIC_BTN_HOME	0x0800
#define WIIMOTE_CLASSIC_BTN_MINUS	0x1000
#define WIIMOTE_CLASSIC_BTN_L		0x2000
#define WIIMOTE_CLASSIC_BTN_DOWN	0x4000
#define WIIMOTE_CLASSIC_BTN_RIGHT	0x8000

/* Data Read/Write flags */
#define WIIMOTE_RW_EEPROM	0x00
#define WIIMOTE_RW_REG		0x04
#define WIIMOTE_RW_DECODE	0x01

/* Maximum Data Read Length */
#define WIIMOTE_MAX_READ_LEN	0xFFFF

/* IR Defs */
#define WIIMOTE_IR_SRC_COUNT	4
#define WIIMOTE_IR_X_MAX		1024
#define WIIMOTE_IR_Y_MAX		768

/* Battery */
#define WIIMOTE_BATTERY_MAX	0xD0

/* Classic Controller Maxes */
#define WIIMOTE_CLASSIC_L_STICK_MAX	0x3F
#define WIIMOTE_CLASSIC_R_STICK_MAX	0x1F
#define WIIMOTE_CLASSIC_LR_MAX	0x1F

/* Environment Variables */
#define WIIMOTE_BDADDR	"WIIMOTE_BDADDR"

/* Callback Maximum Message Count */
#define WIIMOTE_MAX_MESG_COUNT	5

enum wiimote_command {
	WIIMOTE_CMD_STATUS,
	WIIMOTE_CMD_LED,
	WIIMOTE_CMD_RUMBLE,
	WIIMOTE_CMD_RPT_MODE
};

enum wiimote_mesg_type {
	WIIMOTE_MESG_STATUS,
	WIIMOTE_MESG_BTN,
	WIIMOTE_MESG_ACC,
	WIIMOTE_MESG_IR,
	WIIMOTE_MESG_NUNCHUK,
	WIIMOTE_MESG_CLASSIC,
	WIIMOTE_MESG_UNKNOWN
};

enum wiimote_ext_type {
	WIIMOTE_EXT_NONE,
	WIIMOTE_EXT_NUNCHUK,
	WIIMOTE_EXT_CLASSIC,
	WIIMOTE_EXT_UNKNOWN
};

struct wiimote_status_mesg {
	enum wiimote_mesg_type type;
	uint8_t battery;
	enum wiimote_ext_type extension;
};	

struct wiimote_btn_mesg {
	enum wiimote_mesg_type type;
	uint16_t buttons;
};

struct wiimote_acc_mesg {
	enum wiimote_mesg_type type;
	uint8_t x;
	uint8_t y;
	uint8_t z;
};

struct wiimote_ir_src {
	int valid;
	uint16_t x;
	uint16_t y;
	int8_t size;
};

struct wiimote_ir_mesg {
	enum wiimote_mesg_type type;
	struct wiimote_ir_src src[WIIMOTE_IR_SRC_COUNT];
};

struct wiimote_nunchuk_mesg {
	enum wiimote_mesg_type type;
	uint8_t stick_x;
	uint8_t stick_y;
	uint8_t acc_x;
	uint8_t acc_y;
	uint8_t acc_z;
	uint8_t buttons;
};

struct wiimote_classic_mesg {
	enum wiimote_mesg_type type;
	uint8_t l_stick_x;
	uint8_t l_stick_y;
	uint8_t r_stick_x;
	uint8_t r_stick_y;
	uint8_t l;
	uint8_t r;
	uint16_t buttons;
};

union wiimote_mesg {
	enum wiimote_mesg_type type;
	struct wiimote_status_mesg status_mesg;
	struct wiimote_btn_mesg btn_mesg;
	struct wiimote_acc_mesg acc_mesg;
	struct wiimote_ir_mesg ir_mesg;
	struct wiimote_nunchuk_mesg nunchuk_mesg;
	struct wiimote_classic_mesg classic_mesg;
};

typedef struct wiimote wiimote_t;

typedef void wiimote_mesg_callback_t(int, int, union wiimote_mesg* []);
typedef void wiimote_err_t(int, const char *, ...);

/* getinfo flags */
#define BT_NO_WIIMOTE_FILTER 0x01
#define BT_WM_NAME_LEN 32

struct wiimote_info {
	bdaddr_t bdaddr;
	uint8_t class[3];
	char name[BT_WM_NAME_LEN];
};

#ifdef __cplusplus
extern "C" {
#endif

int wiimote_set_err(wiimote_err_t *err);
wiimote_t *wiimote_connect(bdaddr_t bdaddr,
                           wiimote_mesg_callback_t *mesg_callback,
                           int *id);
int wiimote_disconnect(wiimote_t *wiimote);
int wiimote_command(wiimote_t *wiimote, enum wiimote_command command,
                    uint8_t flags);
int wiimote_read(wiimote_t *wiimote, uint8_t flags, uint32_t offset,
                 uint16_t len, void *data);
int wiimote_write(wiimote_t *wiimote, uint8_t flags, uint32_t offset,
                  uint16_t len, const void *data);
/* int wiimote_beep(wiimote_t *wiimote); */
int wiimote_get_info_array(int dev_id, unsigned int timeout, int max_wm,
                           struct wiimote_info **wm, uint8_t flags);
int wiimote_find_wiimote(bdaddr_t *bdaddr, int timeout);

#ifdef __cplusplus
}
#endif

#endif

