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

#ifndef WIIMOTE_H
#define WIIMOTE_H

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
	unsigned char battery;
	enum wiimote_ext_type extension;
};	

struct wiimote_btn_mesg {
	enum wiimote_mesg_type type;
	unsigned short buttons;
};

struct wiimote_acc_mesg {
	enum wiimote_mesg_type type;
	unsigned short x;
	unsigned short y;
	unsigned short z;
};

struct wiimote_ir_src {
	int valid;
	unsigned short x;
	unsigned short y;
	char size;
};

struct wiimote_ir_mesg {
	enum wiimote_mesg_type type;
	struct wiimote_ir_src src[WIIMOTE_IR_SRC_COUNT];
};

struct wiimote_nunchuk_mesg {
	enum wiimote_mesg_type type;
	unsigned char stick_x;
	unsigned char stick_y;
	unsigned char acc_x;
	unsigned char acc_y;
	unsigned char acc_z;
	unsigned char buttons;
};

struct wiimote_classic_mesg {
	enum wiimote_mesg_type type;
	unsigned char l_stick_x;
	unsigned char l_stick_y;
	unsigned char r_stick_x;
	unsigned char r_stick_y;
	unsigned char l;
	unsigned char r;
	unsigned short buttons;
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

#ifdef __cplusplus
extern "C" {
#endif

wiimote_t *wiimote_connect(bdaddr_t bdaddr,
                           wiimote_mesg_callback_t *mesg_callback,
                           int *id);
int wiimote_disconnect(wiimote_t *wiimote);
int wiimote_command(wiimote_t *wiimote, enum wiimote_command command,
                    unsigned char flags);
int wiimote_read(wiimote_t *wiimote, unsigned int flags,
                 unsigned int offset, unsigned int len, unsigned char *data);
int wiimote_write(wiimote_t *wiimote, unsigned int flags,
                  unsigned int offset, unsigned int len, unsigned char *data);
/* int wiimote_beep(wiimote_t *wiimote); */
int wiimote_findfirst(bdaddr_t *bdaddr);

#ifdef __cplusplus
}
#endif

#endif

