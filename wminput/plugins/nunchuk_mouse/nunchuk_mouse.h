/* Button flags */
#define STICK_KEY_UP		0x0001
#define STICK_KEY_DOWN		0x0002
#define STICK_KEY_RIGHT		0x0004
#define STICK_KEY_LEFT		0x0008

/* array indices */
#define X                   0
#define Y                   1
#define OFF_X               0
#define OFF_Y               3
#define OFF_MAX             0
#define OFF_MIN             1
#define OFF_CENTER          2

/* misc */
#define DEADZONE            10
#define RANGE               12.0 /* TODO: make this a parameter */

/* calibration data.  Since the center isn't guaranteed to be the
 * average of min and max, we distinguish (as in the cases are not
 * symmetric) between being below center vs. being above center. */
static uint8_t center_x, center_y;
static uint8_t x_neg_range, x_pos_range, y_neg_range, y_pos_range;

/* in data, accumulate mouse motions for all the events in each
 * batch.  After the batch, return it to wminput. */
static struct wmplugin_data data;

/* bookkeeping info */
static unsigned char info_init = 0;
static cwiid_wiimote_t *wiimote;
static int plugin_id;
static struct wmplugin_info info;

/* function declarations */
wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;
static void process_nunchuk(struct cwiid_nunchuk_mesg *mesg);
static void calibrate_joystick(void);
