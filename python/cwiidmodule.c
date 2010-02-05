/*
 * Copyright (C) 2007 Justin M. Tulloss <jmtulloss@gmail.com>, L. Donnie Smith <donnie.smith@gatech.edu>
 *
 * Interface from Python to libcwiid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 *
 */

#include "Python.h"

#include <stdlib.h>

#include <cwiid.h>
#include "structmember.h"

/* externally defined types */
extern PyTypeObject Wiimote_Type;
extern PyObject *ConvertMesgArray(int, union cwiid_mesg []);

/* cwiid module initializer */
PyMODINIT_FUNC initcwiid(void);

/* constants, enumerations */
#define CWIID_CONST_MACRO(a) {#a, CWIID_##a}
static struct {
	char *name;
	int value;
} cwiid_constants[] = {
	CWIID_CONST_MACRO(FLAG_MESG_IFC),
	CWIID_CONST_MACRO(FLAG_CONTINUOUS),
	CWIID_CONST_MACRO(FLAG_REPEAT_BTN),
	CWIID_CONST_MACRO(FLAG_NONBLOCK),
	CWIID_CONST_MACRO(FLAG_MOTIONPLUS),
	CWIID_CONST_MACRO(RPT_STATUS),
	CWIID_CONST_MACRO(RPT_BTN),
	CWIID_CONST_MACRO(RPT_ACC),
	CWIID_CONST_MACRO(RPT_IR),
	CWIID_CONST_MACRO(RPT_NUNCHUK),
	CWIID_CONST_MACRO(RPT_CLASSIC),
	CWIID_CONST_MACRO(RPT_BALANCE),
	CWIID_CONST_MACRO(RPT_MOTIONPLUS),
	CWIID_CONST_MACRO(RPT_EXT),
	CWIID_CONST_MACRO(LED1_ON),
	CWIID_CONST_MACRO(LED2_ON),
	CWIID_CONST_MACRO(LED3_ON),
	CWIID_CONST_MACRO(LED4_ON),
	CWIID_CONST_MACRO(BTN_2),
	CWIID_CONST_MACRO(BTN_1),
	CWIID_CONST_MACRO(BTN_B),
	CWIID_CONST_MACRO(BTN_A),
	CWIID_CONST_MACRO(BTN_MINUS),
	CWIID_CONST_MACRO(BTN_HOME),
	CWIID_CONST_MACRO(BTN_LEFT),
	CWIID_CONST_MACRO(BTN_RIGHT),
	CWIID_CONST_MACRO(BTN_DOWN),
	CWIID_CONST_MACRO(BTN_UP),
	CWIID_CONST_MACRO(BTN_PLUS),
	CWIID_CONST_MACRO(NUNCHUK_BTN_Z),
	CWIID_CONST_MACRO(NUNCHUK_BTN_C),
	CWIID_CONST_MACRO(CLASSIC_BTN_UP),
	CWIID_CONST_MACRO(CLASSIC_BTN_LEFT),
	CWIID_CONST_MACRO(CLASSIC_BTN_ZR),
	CWIID_CONST_MACRO(CLASSIC_BTN_X),
	CWIID_CONST_MACRO(CLASSIC_BTN_A),
	CWIID_CONST_MACRO(CLASSIC_BTN_Y),
	CWIID_CONST_MACRO(CLASSIC_BTN_B),
	CWIID_CONST_MACRO(CLASSIC_BTN_ZL),
	CWIID_CONST_MACRO(CLASSIC_BTN_R),
	CWIID_CONST_MACRO(CLASSIC_BTN_PLUS),
	CWIID_CONST_MACRO(CLASSIC_BTN_HOME),
	CWIID_CONST_MACRO(CLASSIC_BTN_MINUS),
	CWIID_CONST_MACRO(CLASSIC_BTN_L),
	CWIID_CONST_MACRO(CLASSIC_BTN_DOWN),
	CWIID_CONST_MACRO(CLASSIC_BTN_RIGHT),
	CWIID_CONST_MACRO(SEND_RPT_NO_RUMBLE),
	CWIID_CONST_MACRO(RW_EEPROM),
	CWIID_CONST_MACRO(RW_REG),
	CWIID_CONST_MACRO(RW_DECODE),
	CWIID_CONST_MACRO(MAX_READ_LEN),
	CWIID_CONST_MACRO(X),
	CWIID_CONST_MACRO(Y),
	CWIID_CONST_MACRO(Z),
	CWIID_CONST_MACRO(PHI),
	CWIID_CONST_MACRO(THETA),
	CWIID_CONST_MACRO(PSI),
	CWIID_CONST_MACRO(IR_SRC_COUNT),
	CWIID_CONST_MACRO(IR_X_MAX),
	CWIID_CONST_MACRO(IR_Y_MAX),
	CWIID_CONST_MACRO(BATTERY_MAX),
	CWIID_CONST_MACRO(CLASSIC_L_STICK_MAX),
	CWIID_CONST_MACRO(CLASSIC_R_STICK_MAX),
	CWIID_CONST_MACRO(CLASSIC_LR_MAX),
	CWIID_CONST_MACRO(CMD_STATUS),
	CWIID_CONST_MACRO(CMD_LED),
	CWIID_CONST_MACRO(CMD_RUMBLE),
	CWIID_CONST_MACRO(CMD_RPT_MODE),
	CWIID_CONST_MACRO(MESG_STATUS),
	CWIID_CONST_MACRO(MESG_BTN),
	CWIID_CONST_MACRO(MESG_ACC),
	CWIID_CONST_MACRO(MESG_IR),
	CWIID_CONST_MACRO(MESG_NUNCHUK),
	CWIID_CONST_MACRO(MESG_CLASSIC),
	CWIID_CONST_MACRO(MESG_BALANCE),
	CWIID_CONST_MACRO(MESG_MOTIONPLUS),
	CWIID_CONST_MACRO(MESG_ERROR),
	CWIID_CONST_MACRO(MESG_UNKNOWN),
	CWIID_CONST_MACRO(EXT_NONE),
	CWIID_CONST_MACRO(EXT_NUNCHUK),
	CWIID_CONST_MACRO(EXT_CLASSIC),
	CWIID_CONST_MACRO(EXT_BALANCE),
	CWIID_CONST_MACRO(EXT_MOTIONPLUS),
	CWIID_CONST_MACRO(EXT_UNKNOWN),
	CWIID_CONST_MACRO(ERROR_DISCONNECT),
	CWIID_CONST_MACRO(ERROR_COMM),
	{NULL, 0}
};

/* Associates cwiid functions with python ones */
static PyMethodDef Module_Methods[] = 
{
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initcwiid(void)
{
	PyObject *Module;
	PyObject *CObj;
	int i;

	PyEval_InitThreads();

	if (PyType_Ready(&Wiimote_Type) < 0) {
		return;
	}

	if (!(Module = Py_InitModule3("cwiid", Module_Methods,
	  "CWiid Wiimote Interface"))) {
		return;
	}

	Py_INCREF(&Wiimote_Type);
	PyModule_AddObject(Module, "Wiimote", (PyObject*)&Wiimote_Type);

	for (i = 0; cwiid_constants[i].name; i++) {
		/* No way to report errors from here, so just ignore them and hope
		 * for segfault */
		PyModule_AddIntConstant(Module, cwiid_constants[i].name,
		                        cwiid_constants[i].value);
	}

	if (!(CObj = PyCObject_FromVoidPtr(ConvertMesgArray, NULL))) {
		return;
	}
	PyModule_AddObject(Module, "ConvertMesgArray", CObj);
}

