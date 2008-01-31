// /home/jestin/Projects/MonoWii/Wiimote.cs created with MonoDevelop
// User: jestin at 6:25 PM 12/28/2007
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//

using System;
using System.Collections;

namespace MonoWii
{
#region Delegates
	/// <summary>
	/// This is the delegate that gets called when a button event occurs
	/// </summary>
	public delegate void ButtonHandler(cwiid.CWIID_BTN_FLAGS _Buttons);
	
	/// <summary>
	/// This is the delegate that gets called when an accelerometer event occurs
	/// </summary>
	public delegate void AccelHandler(double dX, double dY, double dZ);
	
	/// <summary>
	/// This is the delegate that gets called when an infrared event occurs
	/// </summary>
	public delegate void IRHandler(cwiid.cwiid_ir_mesg _IR);
	
	/// <summary>
	/// This is the delegate that gets called when nunchuk button event occurs
	/// </summary>
	public delegate void NunchukButtonHandler(cwiid.CWIID_NUNCHUK_BTN_FLAGS _Buttons);
	
	/// <summary>
	/// This is the delegate that gets called when nunchuk accelerometer event occurs
	/// </summary>
	public delegate void NunchukAccelHandler(byte _accX, byte _accY, byte _accZ);
	
	/// <summary>
	/// This is the delegate that gets called when nunchuk stick event occurs
	/// </summary>
	public delegate void NunchukStickHandler(byte _StickX, byte _StickY);
#endregion
	
	public class Wiimote
	{
#region Enums
		/// <summary>
		/// This is the delegate that gets called when an infrared event occurs
		/// </summary>
		public enum LEDS : int
		{
			LED1 = 0,
			LED2,
			LED3,
			LED4
		}
#endregion
		
#region Member Variables
		/// <summary>
		/// The minimum time lapse between calls to IREvent
		/// </summary>
		private System.UInt32 m_nMinIRInterval = 50000000;
		
		/// <summary>
		/// The minimum time lapse between calls to AccelEvent
		/// </summary>
		private System.UInt32 m_nMinACCInterval = 75000000;
		
		/// <summary>
		/// The minimum time lapse between calls to NunchukAccelEvent
		/// </summary>
		private System.UInt32 m_nMinNunchukACCInterval = 100000000;
		
		/// <summary>
		/// The minimum time lapse between calls to NunchukStickEvent
		/// </summary>
		private System.UInt32 m_nMinNunchukStickInterval = 100000000;
		
		/// <summary>
		/// Whether we are using accelerometer buffereing
		/// </summary>
		private bool m_bAccelBufferOn = false;
		
		/// <summary>
		/// The size of the accelerometer buffer
		/// </summary>
		private int m_nAccelBufferSize = 6;
		
		/// <summary>
		/// The accelerometer buffer
		/// </summary>
		private ArrayList m_AccelBuffer = new ArrayList();
		
		/// <summary>
		/// An event that hits when a button is pressed
		/// </summary>
		public event ButtonHandler ButtonEvent = null;
		
		/// <summary>
		/// An event that hits when an accelerometer event is recieved
		/// </summary>
		public event AccelHandler AccelEvent = null;
		
		/// <summary>
		/// An event that hits when an infrared event is recieved
		/// </summary>
		public event IRHandler IREvent = null;
		
		/// <summary>
		/// An event that hits when a nunchuk button event is recieved
		/// </summary>
		public event NunchukButtonHandler NunchukButtonEvent = null;
		
		/// <summary>
		/// An event that hits when a nunchuk accelerometer event is recieved
		/// </summary>
		public event NunchukAccelHandler NunchukAccelEvent = null;
		
		/// <summary>
		/// An event that hits when a nunchuk stick event is recieved
		/// </summary>
		public event NunchukStickHandler NunchukStickEvent = null;
		
		/// <summary>
		/// The handle to the wiimote
		/// </summary>
		IntPtr m_hWiimote;
		
		/// <summary>
		/// The address of the wiimote
		/// </summary>
		private Bluetooth.bdaddr_t m_Address = Bluetooth.BDADDR_ANY;
		
		/// <summary>
		/// The state of the the LEDs
		/// </summary>
		private cwiid.CWIID_LED_FLAGS m_LEDState = 0;
		
		/// <summary>
		/// Whether the wiimote is connected
		/// </summary>
		private bool m_bConnected = false;
		
		/// <summary>
		/// The timestamp of the last infrared event sent out
		/// </summary>
		private cwiid.timespec m_LastIRTimeStamp;
		
		/// <summary>
		/// The timestamp of the last accelerometer event sent out
		/// </summary>
		private cwiid.timespec m_LastACCTimeStamp;
		
		/// <summary>
		/// The timestamp of the last nunchuk accelerometer event sent out
		/// </summary>
		private cwiid.timespec m_LastNunchukACCTimeStamp;
		
		/// <summary>
		/// The timestamp of the last nunchuk stick event sent out
		/// </summary>
		private cwiid.timespec m_LastNunchukStickTimeStamp;
		
		/// <summary>
		/// This is the last received nunchuk button
		/// </summary>
		private cwiid.CWIID_NUNCHUK_BTN_FLAGS m_LastNunchukButtons = 0x00;
		
		/// <summary>
		/// The error callback
		/// </summary>
		private static cwiid.cwiid_err_t WiimoteError = null;
		
		/// <summary>
		/// The main event callback
		/// </summary>
		private static cwiid.cwiid_mesg_callback_t WiimoteCallback = null;
#endregion
		
#region Constructors
		/// <summary>
		/// The standard constructor for a Wiimote object
		/// </summary>
		public Wiimote()
		{
			WiimoteError = new MonoWii.cwiid.cwiid_err_t(OnWiimoteError);
			WiimoteCallback = new MonoWii.cwiid.cwiid_mesg_callback_t(OnWiimoteCallback);
		}
#endregion
		
#region Public Methods
		
		/// <summary>
		/// Connects whatever wiimote device has been previously specified (Any available is the default).
		/// </summary>
		public bool Connect()
		{
			return Connect(m_Address);
		}
		
		/// <summary>
		/// Connects the wiimote with the specified address
		/// </summary>
		public bool Connect(Bluetooth.bdaddr_t _Address)
		{
			m_hWiimote = cwiid.cwiid_open(ref _Address, cwiid.CWIID_FLAGS.MESG_IFC);
			if (m_hWiimote.ToInt32() == 0)
			{
				System.Console.WriteLine( "Unable to connect to wiimote");
				return false;
			}
			
			if (cwiid.cwiid_set_mesg_callback(m_hWiimote, WiimoteCallback) > 0)
			{
				System.Console.WriteLine( "Unable to set message callback");
				return false;
			}
			
			m_Address = new Bluetooth.bdaddr_t(_Address.ToString());
			m_bConnected = true;
			
			return true;
		}
		
		/// <summary>
		/// Disconnects the connect wiimote
		/// </summary>
		public int Disconnect()
		{
			if(m_bConnected)
			{
				return cwiid.cwiid_close(m_hWiimote);
			}
			else
			{
				return 0;
			}
		}
		
		/// <summary>
		/// Enables a flag
		/// </summary>
		public int Enable(cwiid.CWIID_FLAGS flags)
		{
			return cwiid.cwiid_enable(m_hWiimote, flags);
		}
		
		/// <summary>
		/// Disables a flag
		/// </summary>
		public int Disable(cwiid.CWIID_FLAGS flags)
		{
			return cwiid.cwiid_disable(m_hWiimote, flags);
		}
		
		/// <summary>
		/// Beeps
		/// </summary>
		public int Beep()
		{
			return cwiid.cwiid_beep(m_hWiimote);
		}
		
		/// <summary>
		/// Sets the states of the LEDs
		/// </summary>
		public void SetLEDState(cwiid.CWIID_LED_FLAGS _State)
		{
			m_LEDState = _State;
			
			if (cwiid.cwiid_set_led(m_hWiimote, _State) > 0)
			{
				System.Console.Write( "Error setting LEDs \n");
			}
		}
		
		/// <summary>
		/// Sets the reporting mode of the wiimote
		/// </summary>
		public void SetReportMode(cwiid.CWIID_RPT_FLAGS _Mode)
		{
			if (cwiid.cwiid_set_rpt_mode(m_hWiimote, _Mode) > 0)
			{
				System.Console.Write( "Error setting report mode \n");
			}
		}
		
		/// <summary>
		/// Toggles on/off the specified LED
		/// </summary>
		public void ToggleLED(LEDS _LED)
		{
			switch(_LED)
			{
				case LEDS.LED1:
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED1_ON;
					break;
				case LEDS.LED2:
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED2_ON;
					break;
				case LEDS.LED3:
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED3_ON;
					break;
				case LEDS.LED4:
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED4_ON;
					break;
			}
			
			SetLEDState(m_LEDState);
		}
		
		/// <summary>
		/// Turns on the specified LED
		/// </summary>
		public void SetLED(LEDS _LED)
		{
			switch(_LED)
			{
				case LEDS.LED1:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED1_ON;
					break;
				case LEDS.LED2:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED2_ON;
					break;
				case LEDS.LED3:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED3_ON;
					break;
				case LEDS.LED4:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED4_ON;
					break;
			}
			
			SetLEDState(m_LEDState);
		}
		
		/// <summary>
		/// Turns off the specified LED
		/// </summary>
		public void UnSetLED(LEDS _LED)
		{
			switch(_LED)
			{
				case LEDS.LED1:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED1_ON;
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED1_ON;
					break;
				case LEDS.LED2:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED2_ON;
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED2_ON;
					break;
				case LEDS.LED3:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED3_ON;
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED3_ON;
					break;
				case LEDS.LED4:
					m_LEDState = m_LEDState | cwiid.CWIID_LED_FLAGS.LED4_ON;
					m_LEDState = m_LEDState ^ cwiid.CWIID_LED_FLAGS.LED4_ON;
					break;
			}
			
			SetLEDState(m_LEDState);
		}
		
		/// <summary>
		/// Sets the state of the rumble
		/// </summary>
		public void SetRumble(bool _bRumble)
		{
			byte rumble = (byte) ((_bRumble) ? 1 : 0);
			
			if (cwiid.cwiid_set_rumble(m_hWiimote, rumble) > 0)
			{
				System.Console.Write( "Error setting rumble \n");
			}
		}
#endregion
		
#region Callbacks
		/// <summary>
		/// The main wiimote callback used by libcwiid
		/// </summary>
		private void OnWiimoteCallback(IntPtr wiimote, int mesg_count, IntPtr mesgArray, ref cwiid.timespec timestamp)
		{
			bool bUpdateIR = true;
			bool bUpdateACC = true;
			bool bUpdateNunchukACC = true;
			bool bUpdateNunchukStick = true;
			
			// decide to update IR
			if(timestamp.tv_sec == m_LastIRTimeStamp.tv_sec)
			{
				if(timestamp.tv_nsec - m_LastIRTimeStamp.tv_nsec < MinIRInterval)
				{
					bUpdateIR = false;
				}
			}
			else
			{
				if((System.UInt32.MaxValue - m_LastIRTimeStamp.tv_nsec) + timestamp.tv_nsec < MinIRInterval)
				{
					bUpdateIR = false;
				}
			}
			
			// decide to update ACC
			if(timestamp.tv_sec == m_LastACCTimeStamp.tv_sec)
			{
				if(timestamp.tv_nsec - m_LastACCTimeStamp.tv_nsec < MinACCInterval)
				{
					bUpdateACC = false;
				}
			}
			else
			{
				if((System.UInt32.MaxValue - m_LastACCTimeStamp.tv_nsec) + timestamp.tv_nsec < MinACCInterval)
				{
					bUpdateACC = false;
				}
			}
			
			// decide to update nunchuk ACC
			if(timestamp.tv_sec == m_LastNunchukACCTimeStamp.tv_sec)
			{
				if(timestamp.tv_nsec - m_LastNunchukACCTimeStamp.tv_nsec < MinNunchukACCInterval)
				{
					bUpdateNunchukACC = false;
				}
			}
			else
			{
				if((System.UInt32.MaxValue - m_LastNunchukACCTimeStamp.tv_nsec) + timestamp.tv_nsec < MinNunchukACCInterval)
				{
					bUpdateNunchukACC = false;
				}
			}
			
			// decide to update nunchuk stick
			if(timestamp.tv_sec == m_LastNunchukStickTimeStamp.tv_sec)
			{
				if(timestamp.tv_nsec - m_LastNunchukStickTimeStamp.tv_nsec < MinNunchukStickInterval)
				{
					bUpdateNunchukStick = false;
				}
			}
			else
			{
				if((System.UInt32.MaxValue - m_LastNunchukStickTimeStamp.tv_nsec) + timestamp.tv_nsec < MinNunchukStickInterval)
				{
					bUpdateNunchukStick = false;
				}
			}
			
			unsafe
			{
				cwiid.cwiid_mesg* messagePtr = (cwiid.cwiid_mesg*) mesgArray.ToPointer();
				for (int i = 0;i < mesg_count ; i++)
				{
					cwiid.cwiid_mesg message = *(messagePtr + i);
					// System.Console.WriteLine(message.type.ToString() +"\n");
					switch(message.type)
					{
					case cwiid.cwiid_mesg_type.CWIID_MESG_STATUS:
						System.Console.Write("Status Report: battery={0} extension=",
						              message.status_mesg.battery.ToString());
						switch (message.status_mesg.ext_type)
						{
							case cwiid.cwiid_ext_type.CWIID_EXT_NONE:
								System.Console.Write("none");
								break;
							case cwiid.cwiid_ext_type.CWIID_EXT_NUNCHUK:
								System.Console.Write("Nunchuk");
								break;
							case cwiid.cwiid_ext_type.CWIID_EXT_CLASSIC:
								System.Console.Write("Classic Controller");
								break;
							default:
								System.Console.Write("Unknown Extension");
								break;
						}
						System.Console.Write("\n");
						break;
					case  cwiid.cwiid_mesg_type.CWIID_MESG_BTN:
						if(ButtonEvent != null)
						{
							ButtonEvent(message.btn_mesg.buttons);
						}
						break;
					case cwiid.cwiid_mesg_type.CWIID_MESG_ACC:
						
						m_LastACCTimeStamp.tv_sec = timestamp.tv_sec;
						m_LastACCTimeStamp.tv_nsec = timestamp.tv_nsec;
					
						if(m_bAccelBufferOn)
						{
							m_AccelBuffer.Insert(0, message.acc_mesg);
							
							while(m_AccelBuffer.Count > m_nAccelBufferSize)
							{
								m_AccelBuffer.RemoveAt(m_nAccelBufferSize);
							}
							
							if(AccelEvent != null && bUpdateACC)
							{
								double dX = 0.0;
								double dY = 0.0;
								double dZ = 0.0;
								
								for(int nIndex = 0; nIndex < m_AccelBuffer.Count; nIndex++)
								{
									cwiid.cwiid_acc_mesg Current = (cwiid.cwiid_acc_mesg)m_AccelBuffer[nIndex];
									
									dX += (double)Current.accX;
									dY += (double)Current.accY;
									dZ += (double)Current.accZ;
								}
								
								dX /= (double) m_AccelBuffer.Count;
								dY /= (double) m_AccelBuffer.Count;
								dZ /= (double) m_AccelBuffer.Count;
								
								AccelEvent(dX, dY, dZ);
							}
						}
						else
						{
							if(AccelEvent != null && bUpdateACC)
							{
								AccelEvent((double)message.acc_mesg.accX,
								           (double)message.acc_mesg.accY,
								           (double)message.acc_mesg.accZ);
							}
						}
						break;
					case cwiid.cwiid_mesg_type.CWIID_MESG_IR:
						if(IREvent != null && bUpdateIR)
						{
							m_LastIRTimeStamp.tv_sec = timestamp.tv_sec;
							m_LastIRTimeStamp.tv_nsec = timestamp.tv_nsec;
							IREvent(message.ir_mesg);
						}
						break;
					case cwiid.cwiid_mesg_type.CWIID_MESG_NUNCHUK:
						if(NunchukButtonEvent != null)
						{
							if(message.nunchuk_mesg.buttons != m_LastNunchukButtons)
							{
								NunchukButtonEvent(message.nunchuk_mesg.buttons);
								m_LastNunchukButtons = message.nunchuk_mesg.buttons;
							}
						}
						
						if(NunchukAccelEvent != null && bUpdateNunchukACC)
						{
							m_LastNunchukACCTimeStamp.tv_sec = timestamp.tv_sec;
							m_LastNunchukACCTimeStamp.tv_nsec = timestamp.tv_nsec;
							NunchukAccelEvent(message.nunchuk_mesg.accX, message.nunchuk_mesg.accY, message.nunchuk_mesg.accZ);
						}
						
						if(NunchukStickEvent != null && bUpdateNunchukStick)
						{
							m_LastNunchukStickTimeStamp.tv_sec = timestamp.tv_sec;
							m_LastNunchukStickTimeStamp.tv_nsec = timestamp.tv_nsec;
							NunchukStickEvent(message.nunchuk_mesg.stick[0], message.nunchuk_mesg.stick[1]);
						}
						break;
					case cwiid.cwiid_mesg_type.CWIID_MESG_CLASSIC:
						System.Console.Write("Classic Report: btns={0} l_stick=({1},{2}) r_stick=({3},{4}) " +
						              "l={5} r={5}\n", message.classic_mesg.buttons.ToString(), message.classic_mesg.l_stick[0].ToString(),
					                             message.classic_mesg.l_stick[1].ToString(), message.classic_mesg.r_stick[0].ToString(), message.classic_mesg.r_stick[1].ToString(),
					                              message.classic_mesg.l.ToString(), message.classic_mesg.r.ToString());
						break;
					case cwiid.cwiid_mesg_type.CWIID_MESG_ERROR:
						if (cwiid.cwiid_close(wiimote) > 0)
						{
							System.Console.Write( "Error on wiimote disconnect\n");
							System.Environment.Exit(-1);
						}
						System.Environment.Exit(0);
						break;
					case cwiid.cwiid_mesg_type.CWIID_MESG_UNKNOWN:
						System.Console.WriteLine("Unknown Report");
						break;
					default:
						break;
					}
				}
			}
		}
		
		/// <summary>
		/// The error callback used by libcwiid
		/// </summary>
		private void OnWiimoteError(IntPtr wiimote, string s, IntPtr ap)
		{
			if (wiimote.ToInt32() != 0)
			{
				System.Console.Write("{0}:", cwiid.cwiid_get_id(wiimote));
			}
			else
			{
				System.Console.Write("-1:");
				System.Console.WriteLine(s);
			}
		}
#endregion
		
#region Properties
		/// <summary>
		/// Whether the wiimote is connected
		/// </summary>
		public bool Connected
		{
			get { return m_bConnected; }
		}
		
		/// <summary>
		/// The minimum time lapse between calls to IREvent
		/// </summary>
		public System.UInt32 MinIRInterval
		{
			get { return m_nMinIRInterval; }
			set { m_nMinIRInterval = value; }
		}
		
		/// <summary>
		/// The minimum time lapse between calls to AccelEvent
		/// </summary>
		public System.UInt32 MinACCInterval
		{
			get { return m_nMinACCInterval; }
			set { m_nMinACCInterval = value; }
		}
		
		/// <summary>
		/// The minimum time lapse between calls to NunchukAccelEvent
		/// </summary>
		public System.UInt32 MinNunchukACCInterval
		{
			get { return m_nMinNunchukACCInterval; }
			set { m_nMinNunchukACCInterval = value; }
		}
		
		/// <summary>
		/// The minimum time lapse between calls to NunchukStickEvent
		/// </summary>
		public System.UInt32 MinNunchukStickInterval
		{
			get { return m_nMinNunchukStickInterval; }
			set { m_nMinNunchukStickInterval = value; }
		}
		
		/// <summary>
		/// The address of the wiimote
		/// </summary>
		public Bluetooth.bdaddr_t Address
		{
			get { return m_Address; }
			set { m_Address = value; }
		}
		
		/// <summary>
		/// Whether we are using accelerometer buffereing
		/// </summary>
		public bool AccelerometerBuffereing
		{
			get { return m_bAccelBufferOn; }
			set { m_bAccelBufferOn = value; }
		}
		
		/// <summary>
		/// The size of the accelerometer buffer
		/// </summary>
		public int AccelerometerBufferSize
		{
			get { return m_nAccelBufferSize; }
			set { m_nAccelBufferSize = value; }
		}
#endregion
	}
}
