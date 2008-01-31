/*  Originial cwiid Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 *	
 *  .Net/Mono Bindings by Marek Wyborski
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
 * 
 *  2007-12-31 Jestin Stoffel
 *  * Removed some unreachable lines
 * 
 *  2007-12-28 Jestin Stoffel
 *  * Factored classes into separate files for ease of use
 * 
 *  2007-11-11 Marek Wyborski 
 *  * .Net/Mono Bindings for cwiid 0.6.00
 *  * Most of the bindings have been tested 
 */


using System;
using System.Runtime.InteropServices;

namespace MonoWii
{
	public static class cwiid
	{
		/// <summary>
		/// Option flags can be enabled by cwiid_connect, or subsequently with cwiid_enable.
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_FLAGS
		{
			/// <summary>
			/// Enable the message based interfaces (message callback and cwiid_get_mesg)
			/// </summary>
			MESG_IFC    = 0x01,	
			/// <summary>
			/// Enable continuous wiimote reports
			/// </summary>
			CONTINUOUS	= 0x02,
			/// <summary>
			/// Deliver a button message for each button value received, even if it hasn't changed.
			/// </summary>
			REPEAT_BTN	= 0x04,
			/// <summary>
			/// Causes cwiid_get_mesg to fail instead of block if no messages are ready.
			/// </summary>
			NONBLOCK	= 0x08
		}

		/// <summary>
		/// Report Mode Flags
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_RPT_FLAGS : byte
		{
			STATUS	= 0x01,
			BTN	= 0x02,
			ACC	= 0x04,
			IR	= 0x08,
			NUNCHUK =	0x10,
			CLASSIC =	0x20,
			EXT	=	(NUNCHUK | CLASSIC)
		}

		/// <summary>
		/// LED flags
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_LED_FLAGS :byte
		{
			LED1_ON =	0x01,
			LED2_ON	=   0x02,
			LED3_ON	=   0x04,
			LED4_ON	=   0x08
		}
		
		/// <summary>
		/// Button flags
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_BTN_FLAGS: ushort
		{
			BTN_2	=	0x0001,
			BTN_1	=	0x0002,
			BTN_B	=	0x0004,
			BTN_A	=	0x0008,
			MINUS	=   0x0010,
			HOME	=   0x0080,
			LEFT	=   0x0100,
			RIGHT	=   0x0200,
			DOWN	=   0x0400,
			UP	    =   0x0800,
			PLUS	=   0x1000
		}
		
		/// <summary>
		/// Nunchuk Button Flags
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_NUNCHUK_BTN_FLAGS : byte
		{
			BTN_Z =	0x01,
			BTN_C =	0x02
		}

		/// <summary>
		/// Classic Controller Button Flags
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_CLASSIC_BTN_FLAGS : ushort
		{
			BTN_UP =	0x0001,
			BTN_LEFT =	0x0002,
			BTN_ZR   =	0x0004,
			BTN_X	 =	0x0008,
			BTN_A    =	0x0010,
			BTN_Y	 =	0x0020,
			BTN_B	 =	0x0040,
			BTN_ZL	 =  0x0080,
			BTN_R	 =	0x0200,
			BTN_PLUS =	0x0400,
			BTN_HOME =	0x0800,
			BTN_MINUS=	0x1000,
			BTN_L	 =	0x2000,
			BTN_DOWN =	0x4000,
			BTN_RIGHT=	0x8000
		}

		/// <summary>
		/// Data Read/Write flags
		/// </summary>
		[FlagsAttribute]
		public enum CWIID_RW_FLAGS : byte
		{
			EEPROM =	0x00,
			REG    =	0x04,
			DECODE =	0x01
		}
		
		/// <summary>
		/// Maximum Data Read Length
		/// </summary>
		public const int CWIID_MAX_READ_LEN	= 0xFFFF;

		///* Array Index Defs */
		//#define CWIID_X		0
		//#define CWIID_Y		1
		//#define CWIID_Z		2

		/* Acc Defs */
		public const int  CWIID_ACC_MAX =	0xFF;

		/* IR Defs */
		public const int CWIID_IR_SRC_COUNT = 4;
		public const int CWIID_IR_X_MAX = 1024;
		public const int CWIID_IR_Y_MAX	= 768;

		/* Battery */
		public const float CWIID_BATTERY_MAX =	(float) 0xD0;

		/* Classic Controller Maxes */
		public const int CWIID_CLASSIC_L_STICK_MAX	= 0x3F;
		public const int CWIID_CLASSIC_R_STICK_MAX	= 0x1F;
		public const int CWIID_CLASSIC_LR_MAX	= 0x1F;

		/* Callback Maximum Message Count */
		public const int  CWIID_MAX_MESG_COUNT =	5;
		
		/* get_bdinfo */
		public const int BT_NO_WIIMOTE_FILTER = 0x01;
		public const int BT_NAME_LEN = 32;
		
		struct cwiid_bdinfo {
			Bluetooth.bdaddr_t bdaddr;
			fixed byte btclass[3];
			fixed char name[BT_NAME_LEN];
		}
	
		/* Enumerations */
		public enum cwiid_command_Flags
		{
			CWIID_CMD_STATUS,
			CWIID_CMD_LED,
			CWIID_CMD_RUMBLE,
			CWIID_CMD_RPT_MODE
		}

		public enum cwiid_mesg_type {
			CWIID_MESG_STATUS,
			CWIID_MESG_BTN,
			CWIID_MESG_ACC,
			CWIID_MESG_IR,
			CWIID_MESG_NUNCHUK,
			CWIID_MESG_CLASSIC,
			CWIID_MESG_ERROR,
			CWIID_MESG_UNKNOWN
		}
		
		public enum cwiid_ext_type {
			CWIID_EXT_NONE,
			CWIID_EXT_NUNCHUK,
			CWIID_EXT_CLASSIC,
			CWIID_EXT_UNKNOWN
		}
		
		public enum cwiid_error {
			CWIID_ERROR_NONE,
			CWIID_ERROR_DISCONNECT,
			CWIID_ERROR_COMM
		}
		
		[StructLayout(LayoutKind.Sequential)]
		public struct acc_cal {
			public fixed byte zero[3];
			public fixed byte one[3];
		}
		
		/* Message Structs */
		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_status_mesg 
		{
			public cwiid_mesg_type type;
			public byte battery;
			public cwiid_ext_type ext_type;
		}
		
		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_btn_mesg {
			public cwiid_mesg_type type;
			public cwiid.CWIID_BTN_FLAGS buttons;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_acc_mesg {
			public cwiid_mesg_type type;
			public byte accX;
			public byte accY;
			public byte accZ;
			
			public override string ToString()
			{
				return "x: " +accX.ToString() + " y: "+ accY.ToString() + " z: "+ accZ.ToString();
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_ir_src {
			public byte valid;
			public ushort posX;
			public ushort posY;
			public sbyte size;
		}	

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_ir_src4
		{
			public cwiid_ir_src src0;
			public cwiid_ir_src src1;
			public cwiid_ir_src src2;
			public cwiid_ir_src src3;
			
			public cwiid_ir_src GetValue(int index)
			{
				switch(index)
				{
				case 0:
					return src0;
				case 1:
					return src1;
				case 2:
					return src2;
				case 3:
					return src3;
				default:
					throw new Exception("Index out of bounds!");
					return new cwiid_ir_src();
				}
			}
		}
		
		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_ir_mesg {
			public cwiid_mesg_type type;
			public cwiid_ir_src4 src;
			// CWIID_IR_SRC_COUNT = 4
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_nunchuk_mesg {
			public cwiid_mesg_type type;
			public fixed byte stick[2];
			public byte accX;
			public byte accY;
			public byte accZ;
			public cwiid.CWIID_NUNCHUK_BTN_FLAGS buttons;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_classic_mesg {
			public cwiid_mesg_type type;
			public fixed byte l_stick[2];
			public fixed byte r_stick[2];
			public byte l;
			public byte r;
			public cwiid.CWIID_CLASSIC_BTN_FLAGS buttons;
		}	

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_error_mesg {
			public cwiid_mesg_type type;
			public cwiid_error error;
		}

		// was union before
		[StructLayout(LayoutKind.Explicit)]
		public struct cwiid_mesg {
			[FieldOffset(0)] public cwiid_mesg_type type;
			[FieldOffset(0)] public cwiid_status_mesg status_mesg;
			[FieldOffset(0)] public cwiid_btn_mesg btn_mesg;
			[FieldOffset(0)] public cwiid_acc_mesg acc_mesg;
			[FieldOffset(0)] public cwiid_ir_mesg ir_mesg;
			[FieldOffset(0)] public cwiid_nunchuk_mesg nunchuk_mesg;
			[FieldOffset(0)] public cwiid_classic_mesg classic_mesg;
			[FieldOffset(0)] public cwiid_error_mesg error_mesg;
		}

		/* State Structs */
		[StructLayout(LayoutKind.Sequential)]
		public struct nunchuk_state {
			public fixed byte stick[2];
			public fixed byte acc[3];
			public cwiid.CWIID_NUNCHUK_BTN_FLAGS buttons;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct classic_state {
			public fixed byte l_stick[2];
			public fixed byte r_stick[2];
			public byte l;
			public byte r;
			public cwiid.CWIID_CLASSIC_BTN_FLAGS buttons;
		}

		// union
		[StructLayout(LayoutKind.Explicit)]
		public struct ext_state {
			[FieldOffset(0)] public nunchuk_state nunchuk;
			[FieldOffset(0)] public classic_state classic;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct cwiid_state {
			public cwiid.CWIID_RPT_FLAGS rpt_mode;
			public cwiid.CWIID_LED_FLAGS led;
			public byte rumble;
			public byte battery;
			public cwiid.CWIID_BTN_FLAGS buttons;
			public byte accX;
			public byte accY;
			public byte accZ;
			public cwiid_ir_src4 ir_src;
			//CWIID_IR_SRC_COUNT = 4
			public cwiid_ext_type ext_type;
			public ext_state ext;
			public cwiid_error error;
           
	        public override string ToString()
            {
				bool validSource = false;
				string returnString = "Report Mode: " + rpt_mode.ToString() + "\n" + "Active LEDs: " + led.ToString() + "\n" ; 
					returnString += "Rumble: " + ((rumble > 0) ? "On" : "Off") + "\n" + "Battery: " + ((int) ((float) battery / cwiid.CWIID_BATTERY_MAX)).ToString() +"\n" +
					"Buttons: " + buttons.ToString() + "\n";
					returnString += string.Format("ACC: x={0} y={1} z={2}\n",accX.ToString(), accY.ToString(), accZ.ToString()) + "IR: ";
				for(int i = 0; i < cwiid.CWIID_IR_SRC_COUNT; i++)
				{
					cwiid.cwiid_ir_src irSrc = ir_src.GetValue(i);
					if (irSrc.valid > 0)
					{
						validSource = true;
						returnString += "("+ irSrc.posX.ToString() + "," + irSrc.posY.ToString() + ")";
					}
				}
				if (!validSource) {
					returnString += "no sources detected";
				}
				returnString+= "\n";
				
				switch(ext_type)
				{
				case cwiid.cwiid_ext_type.CWIID_EXT_NONE:
					returnString += "No extension\n";
					break;
				case cwiid.cwiid_ext_type.CWIID_EXT_UNKNOWN:
					returnString += "Unknown extension attached\n";
					break;
				case cwiid.cwiid_ext_type.CWIID_EXT_NUNCHUK:
					returnString += string.Format("Nunchuk: btns={0} stick=({1},{2}) acc.x={3} acc.y={4} "+
					                              "acc.z={5}\n", ext.nunchuk.buttons.ToString(),  ext.nunchuk.stick[0].ToString(),
					                              ext.nunchuk.stick[1].ToString(), ext.nunchuk.acc[0].ToString(), ext.nunchuk.acc[1].ToString(),
					                              ext.nunchuk.acc[2].ToString());
					break;
				case cwiid.cwiid_ext_type.CWIID_EXT_CLASSIC:
					returnString += string.Format("Classic: btns={0} l_stick=({1},{2}) r_stick=({3},{4}) " +
					                              "l={5} r={5}\n", ext.classic.buttons.ToString(), ext.classic.l_stick[0].ToString(),
					                              ext.classic.l_stick[1].ToString(), ext.classic.r_stick[0].ToString(), ext.classic.r_stick[1].ToString(),
					                              ext.classic.l.ToString(), ext.classic.r.ToString());
					break;
				}
				
				return returnString;
			}
		}
		
		// time.h
		[StructLayout(LayoutKind.Sequential)]
		public struct timespec {
			/// <summary>
			/// Seconds
			/// </summary>
			public System.Int32 tv_sec;
			/// <summary>
			/// Nanoseconds
			/// </summary>
			public System.Int32 tv_nsec;
		}
		
#region Error reporting (library wide)
		// typedef void cwiid_err_t(cwiid_wiimote_t *, const char *, va_list ap);
		public delegate void cwiid_err_t(IntPtr wiimote, string s, IntPtr va_list_ap);
		
		//int cwiid_set_err(cwiid_err_t *err);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_set_err(cwiid.cwiid_err_t err);
		
		//void cwiid_err_default(struct wiimote *wiimote, const char *str, va_list ap);
		[DllImport("libcwiid.so")]
		public static extern void cwiid_err_default(IntPtr wiimote, string str, IntPtr[] ap);
		
#endregion
					
#region Connection
		
		/// <summary>
		/// Establish a connection with a wiimote.
		/// Pass the address of a bdaddr set to *BDADDR_ANY to connect to any (one) available wiimote. In this case, the bdaddr will be filled with the address of the connected wiimote. Due to the bizarre way in which BlueZ implements BDADDR_ANY, it is recommended that BDADDR_ANY not be passed directly to wiimote_connect, as this may change the value of BDADDR_ANY.
		/// </summary>
		/// <param name="bdaddr">
		/// A <see cref="Bluetooth.bdaddr_t"/>
		/// </param>
		/// <param name="flags">
		/// A <see cref="CWIID_FLAGS"/>
		/// </param>
		/// <returns>
		/// A <see cref="IntPtr"/>
		/// </returns>
		[DllImport("libcwiid.so")]
		public static extern IntPtr cwiid_open(ref Bluetooth.bdaddr_t bdaddr, CWIID_FLAGS flags);
		
		//int cwiid_close(cwiid_wiimote_t *wiimote);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_close(IntPtr wiimote);
		
		//int cwiid_get_id(cwiid_wiimote_t *wiimote);
		/// <summary>
		/// Accessor function for the wiimote id, which is guaranteed to be unique among all wiimotes in a single process.Accessor function for the wiimote id, which is guaranteed to be unique among all wiimotes in a single process.
		/// </summary>
		[DllImport("libcwiid.so")]
		public static extern int cwiid_get_id(IntPtr wiimote);
		
		//int cwiid_set_data(cwiid_wiimote_t *wiimote, const void *data);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_set_data(IntPtr wiimote, IntPtr data);
		
		//const void *cwiid_get_data(cwiid_wiimote_t *wiimote);
		[DllImport("libcwiid.so")]
		public static extern IntPtr cwiid_get_data(IntPtr wiimote);
		
		//int cwiid_enable(cwiid_wiimote_t *wiimote, int flags);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_enable(IntPtr wiimote, CWIID_FLAGS flags);
		
		//int cwiid_disable(cwiid_wiimote_t *wiimote, int flags);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_disable(IntPtr wiimote, CWIID_FLAGS flags);
					
#endregion
		
#region Interfaces
		//typedef void cwiid_mesg_callback_t(cwiid_wiimote_t *, int,
        //                           union cwiid_mesg [], struct timespec *);
		public delegate void cwiid_mesg_callback_t(IntPtr wiimote, int mesg_count,IntPtr mesgArray, ref cwiid.timespec timestamp); 
		
		//int cwiid_set_mesg_callback(cwiid_wiimote_t *wiimote, cwiid_mesg_callback_t *callback);
		/// <summary>
		/// Sets the message callback function, which is called when a set of messages are received from the wiimote (each message in a set was generated from the same packet and therefore arrived simultaneously). Note that option flag CWIID_FLAG_MESG_IFC must be enabled for the callback to be called. Passing NULL for the callback cancels the current callback, if there is one.
		/// </summary>
		[DllImport("libcwiid.so")]
		public static extern int cwiid_set_mesg_callback(IntPtr wiimote, cwiid_mesg_callback_t callback);
		
		//int cwiid_get_mesg(cwiid_wiimote_t *wiimote, int *mesg_count, union cwiid_mesg *mesg[], struct timespec *timestamp);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_get_mesg(IntPtr wiimote, ref int mesg_count, IntPtr mesgArray, ref cwiid.timespec timestamp);
		
		[DllImport("libcwiid.so")]
        public static extern int cwiid_get_state(IntPtr wiimote, ref cwiid_state state);
		
		//int cwiid_get_acc_cal(struct wiimote *wiimote, enum cwiid_ext_type ext_type, struct acc_cal *acc_cal);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_get_acc_cal(IntPtr wiimote, cwiid.cwiid_ext_type ext_type, ref cwiid.acc_cal acc_cal);
		
#endregion
		
#region Operations
		
		//int cwiid_command(cwiid_wiimote_t *wiimote, enum cwiid_command command, int flags);
		/// <summary>
		/// Issue command to wiimote. Available commands and their associated flags are as follows:
		/// 
		/// CWIID_CMD_STATUS
		///     Request a status message (delivered to the message callback) (flags ignored) 
		/// CWIID_CMD_LED
		///     Set the LED state. The following flags may be bitwise ORed:
		/// 
		///         * CWIID_LED1_ON
		///         * CWIID_LED2_ON
		///         * CWIID_LED3_ON
		///         * CWIID_LED4_ON 
		/// 
		/// CWIID_CMD_RUMBLE
		///     Set the Rumble state. Set flags to 0 for off, anything else for on. 
		/// CWIID_CMD_RPT_MODE
		///     Set the reporting mode of the wiimote, which determines what wiimote peripherals are enabled, and what data is received by the host. The following flags may be bitwise ORed (Note that it may not be assumed that each flag is a single bit - specifically, CWIID_RPT_EXT = CWIID_RPT_NUNCHUK | CWIID_RPT_CLASSIC):
		/// 
		///         * CWIID_RPT_STATUS
		///         * CWIID_RPT_BTN
		///         * CWIID_RPT_ACC
		///         * CWIID_RPT_IR
		///         * CWIID_RPT_NUNCHUK
		///         * CWIID_RPT_CLASSIC
		///         * CWIID_RPT_EXT 
		/// </summary>
		[DllImport("libcwiid.so")]
		public static extern int cwiid_command(IntPtr wiimote, cwiid.cwiid_command_Flags command, int flags); 
		
		//int cwiid_request_status(cwiid_wiimote_t *wiimote);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_request_status(IntPtr wiimote);
		
		//int cwiid_set_led(cwiid_wiimote_t *wiimote, uint8_t led);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_set_led(IntPtr wiimote, CWIID_LED_FLAGS ledFlags); 
		
		//int cwiid_set_rumble(cwiid_wiimote_t *wiimote, uint8_t rumble);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_set_rumble(IntPtr wiimote, byte rumble); 
		
		// int cwiid_set_rpt_mode(cwiid_wiimote_t *wiimote, uint8_t rpt_mode);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_set_rpt_mode(IntPtr wiimote, CWIID_RPT_FLAGS rpt_mode); 
		
		//int cwiid_read(cwiid_wiimote_t *wiimote, uint8_t flags, uint32_t offset, uint16_t len, void *data);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_read(IntPtr wiimote, cwiid.CWIID_RW_FLAGS flags, System.UInt32 offset, ushort length, IntPtr data);
		
		//int cwiid_write(cwiid_wiimote_t *wiimote, uint8_t flags, uint32_t offset, uint16_t len, const void *data);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_write(IntPtr wiimote, cwiid.CWIID_RW_FLAGS flags, System.UInt32 offset, ushort length, IntPtr data);
		
		// int cwiid_beep(cwiid_wiimote_t *wiimote);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_beep(IntPtr wiimote);
#endregion
		
#region HCI Functions
		// int cwiid_get_bdinfo_array(int dev_id, unsigned int timeout, int max_bdinfo, struct cwiid_bdinfo **bdinfo, uint8_t flags);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_get_bdinfo_array(int dev_id, uint timeout, int max_bdinfo, IntPtr bdinfo, byte flags);
		
		// int cwiid_find_wiimote(bdaddr_t *bdaddr, int timeout);
		[DllImport("libcwiid.so")]
		public static extern int cwiid_find_wiimote(ref Bluetooth.bdaddr_t bdaddr, int timeout);
#endregion
	}
}