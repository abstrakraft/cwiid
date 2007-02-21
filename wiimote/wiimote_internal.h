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

#ifndef WIIMOTE_INTERNAL_H
#define WIIMOTE_INTERNAL_H

#include <pthread.h>
#include "wiimote.h"

/* Bluetooth magic numbers */
#define BT_TRANS_MASK		0xF0
#define BT_TRANS_HANDSHAKE	0x00
#define BT_TRANS_SET_REPORT	0x50
#define BT_TRANS_DATA		0xA0
#define BT_TRANS_DATAC		0xB0

#define BT_PARAM_MASK		0x0F
/* HANDSHAKE params */
#define BT_PARAM_SUCCESSFUL					0x00
#define BT_PARAM_NOT_READY					0x01
#define BT_PARAM_ERR_INVALID_REPORT_ID		0x02
#define BT_PARAM_ERR_UNSUPPORTED_REQUEST	0x03
#define BT_PARAM_ERR_INVALID_PARAMETER		0x04
#define BT_PARAM_ERR_UNKNOWN				0x0E
#define BT_PARAM_ERR_FATAL					0x0F
/* SET_REPORT, DATA, DATAC params */
#define BT_PARAM_INPUT		0x01
#define BT_PARAM_OUTPUT		0x02
#define BT_PARAM_FEATURE	0x03

/* Wiimote specific magic numbers */
#define WIIMOTE_NAME "Nintendo RVL-CNT-01"
#define WIIMOTE_CMP_LEN sizeof(WIIMOTE_NAME)
#define WIIMOTE_CLASS_0 0x04
#define WIIMOTE_CLASS_1 0x25
#define WIIMOTE_CLASS_2 0x00

/* Wiimote port/channel/PSMs */
#define CTL_PSM	17
#define INT_PSM	19

/* Report numbers */
#define RPT_LED_RUMBLE			0x11
#define RPT_RPT_MODE			0x12
#define RPT_IR_ENABLE1			0x13
#define RPT_SPEAKER_ENABLE		0x14
#define RPT_STATUS_REQ			0x15
#define RPT_WRITE				0x16
#define RPT_READ_REQ			0x17
#define RPT_SPEAKER_DATA		0x18
#define RPT_SPEAKER_MUTE		0x19
#define RPT_IR_ENABLE2			0x1A
#define RPT_STATUS				0x20
#define RPT_READ_DATA			0x21
#define RPT_WRITE_ACK			0x22
#define RPT_BTN					0x30
#define RPT_BTN_ACC				0x31
#define RPT_BTN_EXT8			0x32
#define RPT_BTN_ACC_IR12		0x33
#define RPT_BTN_EXT19			0x34
#define RPT_BTN_ACC_EXT16		0x35
#define RPT_BTN_IR10_EXT9		0x36
#define RPT_BTN_ACC_IR10_EXT6	0x37
#define RPT_EXT21				0x3D
#define RPT_BTN_ACC_IR36_1		0x3E
#define RPT_BTN_ACC_IR36_2		0x3F

/* Button Mask (masks unknown bits in button bytes) */
#define BTN_MASK_0			0x1F
#define BTN_MASK_1			0x9F
#define NUNCHUK_BTN_MASK	0x03

/* Extension Values */
#define EXT_NONE	0x2E
#define EXT_PARTIAL 0xFF
#define EXT_NUNCHUK 0x00
#define EXT_CLASSIC 0x01

/* IR Enable blocks */
#define MARCAN_IR_BLOCK_1	"\x00\x00\x00\x00\x00\x00\x90\x00\xC0"
#define MARCAN_IR_BLOCK_2	"\x40\x00"
#define CLIFF_IR_BLOCK_1	"\x02\x00\x00\x71\x01\x00\xAA\x00\x64"
#define CLIFF_IR_BLOCK_2	"\x63\x03"

/* Extension Decode */
#define DECODE(a)	(((a ^ 0x17)+0x17)&0xFF)

enum write_seq_type {
	WRITE_SEQ_RPT,
	WRITE_SEQ_MEM
};

/* send_report flags */
#define SEND_RPT_NO_RUMBLE	0x01

struct write_seq {
	enum write_seq_type type;
	unsigned int report_offset;
	unsigned char *data;
	unsigned int len;
	unsigned int flags;
};

#define SEQ_LEN(seq) (sizeof(seq)/sizeof(struct write_seq))

enum rw_status {
	RW_NONE,
	RW_PENDING,
	RW_READY,
	RW_ERROR
};

struct mesg_array {
	int count;
	union wiimote_mesg *mesg[WIIMOTE_MAX_MESG_COUNT];
};

struct wiimote {
	int id;
	int ctl_socket;
	int int_socket;
	unsigned char led_rumble_state;
	unsigned char rpt_mode_flags;
	unsigned short buttons;
	enum wiimote_ext_type extension;
	wiimote_mesg_callback_t *mesg_callback;
	pthread_t int_listen_thread;
	pthread_t dispatch_thread;
	struct queue *dispatch_queue;
	pthread_mutex_t wiimote_mutex;
	pthread_mutex_t rw_mutex;
	pthread_cond_t rw_cond;
	pthread_mutex_t rw_cond_mutex;
	enum rw_status rw_status;
	unsigned char *read_buf;
	unsigned int read_len;
	unsigned int read_received;
};

/* prototypes */
void *int_listen(struct wiimote *wiimote);
void *dispatch(struct wiimote *wiimote);

int update_rpt_mode(struct wiimote *wiimote, int flags);

void wiimote_err(char *str, ...);
int verify_handshake(struct wiimote *wiimote);
int send_report(struct wiimote *wiimote, unsigned int flags,
                unsigned char report, unsigned int len, unsigned char *data);
int exec_write_seq(struct wiimote *wiimote, unsigned int len,
                   struct write_seq *seq);
void free_mesg_array(struct mesg_array *array);
int wiimote_findfirst(bdaddr_t *bdaddr);

#endif
