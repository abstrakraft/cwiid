#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <cwiid.h>

/* This is a sample program written to demonstrate basic CWiid libwiimote
 * usage, until _actual_ documentation can be written.  It's quick and dirty
 * has a horrible interface, but it's sparce enough to pick out the important
 * parts easily.  For examples of read and write code, see wmgui.  Speaker
 * support is "experimental" (read: bad) enough to be disabled.  The beginnings
 * of a speaker output function are in libwiimote source. */
/* Note: accelerometer (including nunchuk) and IR outputs produce a
 * lot of data - the purpose of this program is demonstration, not good
 * interface, and it shows. */

cwiid_mesg_callback_t cwiid_callback;

#define toggle_bit(bf,b)	\
	(bf) = ((bf) & b)		\
	       ? ((bf) & ~(b))	\
	       : ((bf) | (b))

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state);
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode);

cwiid_err_t err;
void err(int id, const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	printf("%d:", id);
	vprintf(s, ap);
	printf("\n");
	va_end(ap);
}

/* wiimote handle */
cwiid_wiimote_t *wiimote;

int main(int argc, char *argv[])
{
	bdaddr_t bdaddr;	/* bluetooth device address */
	int cwiid_id;		/* wiimote id: useful for handling multiple wiimotes
	                       with a single callback */
	unsigned char led_state = 0;
	unsigned char rpt_mode = 0;
	unsigned char rumble = 0;
	int exit = 0;
	char *str_addr;
	int c;

	cwiid_set_err(err);

	
	/* Connect to address given on command-line, if present */
	if (argc > 1) {
		str2ba(argv[1], &bdaddr);
	}
	else {
		bdaddr = *BDADDR_ANY;
	}

	/* Connect to the wiimote */
	printf("Put Wiimote in discoverable mode now (press 1+2)...\n");
	if (!(wiimote = cwiid_connect(&bdaddr, cwiid_callback, &cwiid_id))) {
		fprintf(stderr, "Unable to connect to wiimote\n");
		return -1;
	}

	/* Menu */
	printf("1: toggle LED 1\n"
	       "2: toggle LED 2\n"
	       "3: toggle LED 3\n"
	       "4: toggle LED 4\n"
	       "a: toggle accelerometer output\n"
	       "b: toggle button output\n"
	       "e: toggle extension output\n"
	       "i: toggle ir output\n"
	       "r: toggle rumble\n"
	       "s: request status message (use t to turn on output)\n"
	       "t: toggle status output\n"
	       "x: exit\n");

	while (!exit) {
		switch (getchar()) {
		case '1':
			toggle_bit(led_state, CWIID_LED1_ON);
			set_led_state(wiimote, led_state);
			break;
		case '2':
			toggle_bit(led_state, CWIID_LED2_ON);
			set_led_state(wiimote, led_state);
			break;
		case '3':
			toggle_bit(led_state, CWIID_LED3_ON);
			set_led_state(wiimote, led_state);
			break;
		case '4':
			toggle_bit(led_state, CWIID_LED4_ON);
			set_led_state(wiimote, led_state);
			break;
		case 'a':
			toggle_bit(rpt_mode, CWIID_RPT_ACC);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'b':
			toggle_bit(rpt_mode, CWIID_RPT_BTN);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'e':
			/* CWIID_RPT_EXT is actually
			 * CWIID_RPT_NUNCHUK | CWIID_RPT_CLASSIC */
			toggle_bit(rpt_mode, CWIID_RPT_EXT);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'i':
			/* libwiimote picks the highest quality IR mode available with the
			 * other options selected (not including as-yet-undeciphered
			 * interleaved mode */
			toggle_bit(rpt_mode, CWIID_RPT_IR);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'r':
			toggle_bit(rumble, 1);
			if (cwiid_command(wiimote, CWIID_CMD_RUMBLE, rumble)) {
				fprintf(stderr, "Error setting rumble\n");
			}
			break;
		case 's':
			if (cwiid_command(wiimote, CWIID_CMD_STATUS, 0)) {
				fprintf(stderr, "Error requesting status message\n");
			}
			break;
		case 't':
			toggle_bit(rpt_mode, CWIID_RPT_STATUS);
			set_rpt_mode(wiimote, rpt_mode);
			break;
		case 'x':
			exit = -1;
			break;
		case '\n':
			break;
		default:
			fprintf(stderr, "invalid option\n");
		}
	}

	if (cwiid_disconnect(wiimote)) {
		fprintf(stderr, "Error on wiimote disconnect\n");
		return -1;
	}

	return 0;
}

void set_led_state(cwiid_wiimote_t *wiimote, unsigned char led_state)
{
	if (cwiid_command(wiimote, CWIID_CMD_LED, led_state)) {
		fprintf(stderr, "Error setting LEDs \n");
	}
}
	
void set_rpt_mode(cwiid_wiimote_t *wiimote, unsigned char rpt_mode)
{
	if (cwiid_command(wiimote, CWIID_CMD_RPT_MODE, rpt_mode)) {
		fprintf(stderr, "Error setting report mode\n");
	}
}

/* Prototype cwiid_callback with cwiid_callback_t, define it with the actual
 * type - this will cause a compile error (rather than some undefined bizarre
 * behavior) if cwiid_callback_t changes */
/* cwiid_mesg_callback_t has undergone a few changes lately, hopefully this
 * will be the last.  Some programs need to know which messages were received
 * simultaneously (e.g. for correlating accelerometer and IR data), and the
 * sequence number mechanism used previously proved cumbersome, so we just
 * pass an array of messages, all of which were received at the same time.
 * The id is to distinguish between multiple wiimotes using the same callback.
 * */
void cwiid_callback(int id, int mesg_count, union cwiid_mesg *mesg[])
{
	int i, j;
	int valid_source;

	for (i=0; i < mesg_count; i++)
	{
		switch (mesg[i]->type) {
		case CWIID_MESG_STATUS:
			printf("Status Report: battery=%d extension=",
			       mesg[i]->status_mesg.battery);
			switch (mesg[i]->status_mesg.extension) {
			case CWIID_EXT_NONE:
				printf("none");
				break;
			case CWIID_EXT_NUNCHUK:
				printf("Nunchuk");
				break;
			case CWIID_EXT_CLASSIC:
				printf("Classic Controller");
				break;
			default:
				printf("Unknown Extension");
				break;
			}
			printf("\n");
			break;
		case CWIID_MESG_BTN:
			printf("Button Report: %.4X\n", mesg[i]->btn_mesg.buttons);
			break;
		case CWIID_MESG_ACC:
			printf("Acc Report: x=%d, y=%d, z=%d\n", mesg[i]->acc_mesg.x,
			                                         mesg[i]->acc_mesg.y,
			                                         mesg[i]->acc_mesg.z);
			break;
		case CWIID_MESG_IR:
			printf("IR Report: ");
			valid_source = 0;
			for (j = 0; j < CWIID_IR_SRC_COUNT; j++) {
				if (mesg[i]->ir_mesg.src[j].valid) {
					valid_source = 1;
					printf("(%d,%d) ", mesg[i]->ir_mesg.src[j].x,
					                   mesg[i]->ir_mesg.src[j].y);
				}
			}
			if (!valid_source) {
				printf("no sources detected");
			}
			printf("\n");
			break;
		case CWIID_MESG_NUNCHUK:
			printf("Nunchuk Report: btns=%.2X stick=(%d,%d) acc.x=%d acc.y=%d "
			       "acc.z=%d\n", mesg[i]->nunchuk_mesg.buttons,
			       mesg[i]->nunchuk_mesg.stick_x,
			       mesg[i]->nunchuk_mesg.stick_y, mesg[i]->nunchuk_mesg.acc_x,
			       mesg[i]->nunchuk_mesg.acc_y, mesg[i]->nunchuk_mesg.acc_z);
			break;
		case CWIID_MESG_CLASSIC:
			printf("Classic Report: btns=%.4X l_stick=(%d,%d) r_stick=(%d,%d) "
			       "l=%d r=%d\n", mesg[i]->classic_mesg.buttons,
			       mesg[i]->classic_mesg.l_stick_x,
			       mesg[i]->classic_mesg.l_stick_y,
			       mesg[i]->classic_mesg.r_stick_x,
			       mesg[i]->classic_mesg.r_stick_y,
			       mesg[i]->classic_mesg.l, mesg[i]->classic_mesg.r);
			break;
		case CWIID_MESG_ERROR:
			if (cwiid_disconnect(wiimote)) {
				fprintf(stderr, "Error on wiimote disconnect\n");
				exit(-1);
			}
			exit(0);
			break;
		default:
			printf("Unknown Report");
			break;
		}
	}
}

