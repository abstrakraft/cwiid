// /home/jestin/Projects/GLWii/GLWii/MainWindow.cs created with MonoDevelop
// User: jestin at 6:22 PM 1/20/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
using System;
using Gtk;
using MonoWii;

public partial class MainWindow: Gtk.Window
{
	private Wiimote MyWiimote = new Wiimote();
	private bool m_bCalibrateMode = false;
	
	private int m_nCalibratePosition = 0;
	
	private double[] XValues = new double[3];
	private double[] YValues = new double[3];
	private double[] ZValues = new double[3];
	
	private double m_dLastAccX;
	private double m_dLastAccY;
	private double m_dLastAccZ;
	
	public MainWindow (): base (Gtk.WindowType.Toplevel)
	{
		Build ();
		
		MyWiimote.AccelEvent += new AccelHandler(OnAccelEvent);
		MyWiimote.ButtonEvent += new ButtonHandler(OnButtonEvent);
		MyWiimote.MinACCInterval = MyWiimote.MinACCInterval * 3 / 4;
		
		MyWiimote.AccelerometerBuffereing = true;
		MyWiimote.AccelerometerBufferSize = 10;
		MyWiimote.MinACCInterval = 0;
	}
	
	protected void OnDeleteEvent (object sender, DeleteEventArgs a)
	{
		MyWiimote.Disconnect();
		Application.Quit ();
		a.RetVal = true;
	}

	protected virtual void OnConnectBtnClicked (object sender, System.EventArgs e)
	{
		if(MyWiimote.Connect())
		{
			MyWiimote.SetReportMode(cwiid.CWIID_RPT_FLAGS.BTN | cwiid.CWIID_RPT_FLAGS.ACC);
			MyWiimote.SetLEDState(MonoWii.cwiid.CWIID_LED_FLAGS.LED1_ON);
			MyWiimote.SetRumble(true);
			System.Threading.Thread.Sleep(100);
			MyWiimote.SetRumble(false);
		}
	}
	
	private void OnAccelEvent(double dX, double dY, double dZ)
	{
		Gtk.Application.Invoke (delegate {
			WiimoteView.SetAcc(dX, dY, dZ);
			WiimoteView.QueueDraw();
		});
		
		if(m_bCalibrateMode)
		{
			m_dLastAccX = dX;
			m_dLastAccY = dY;
			m_dLastAccZ = dZ;
		}
	}
	
	private void OnButtonEvent(cwiid.CWIID_BTN_FLAGS _Buttons)
	{
		if((_Buttons & cwiid.CWIID_BTN_FLAGS.BTN_A) == cwiid.CWIID_BTN_FLAGS.BTN_A)
		{
			if(m_bCalibrateMode)
			{
				XValues[m_nCalibratePosition] = m_dLastAccX;
				YValues[m_nCalibratePosition] = m_dLastAccY;
				ZValues[m_nCalibratePosition] = m_dLastAccZ;
				
				System.Console.WriteLine("Postion: " + m_nCalibratePosition + "  X: " + m_dLastAccX + "  Y: " + m_dLastAccY + "  Z: " + m_dLastAccZ);
				
				if(m_nCalibratePosition >= 2)
				{
					m_nCalibratePosition = 0;
					m_bCalibrateMode = false;
					WiimoteView.SetCalibrateMode(false);
					
					WiimoteView.X_GRAV_ZERO = (XValues[0] + XValues[1]) / 2;
					WiimoteView.X_GRAV_MAX = XValues[2];
					
					WiimoteView.Y_GRAV_ZERO = (YValues[0] + YValues[2]) / 2;
					WiimoteView.Y_GRAV_MAX = YValues[1];
					
					WiimoteView.Z_GRAV_ZERO = (ZValues[1] + ZValues[1]) / 2;
					WiimoteView.Z_GRAV_MAX = ZValues[0];
					
					System.Console.WriteLine("X Zero: " + WiimoteView.X_GRAV_ZERO);
					System.Console.WriteLine("X Max: " + WiimoteView.X_GRAV_MAX);
					
					System.Console.WriteLine("Y Zero: " + WiimoteView.Y_GRAV_ZERO);
					System.Console.WriteLine("Y Max: " + WiimoteView.Y_GRAV_MAX);
					
					System.Console.WriteLine("Z Zero: " + WiimoteView.Z_GRAV_ZERO);
					System.Console.WriteLine("Z Max: " + WiimoteView.Z_GRAV_MAX);
				}
				
				m_nCalibratePosition++;
			}
			else
			{
				Gtk.Application.Invoke (delegate {
					WiimoteView.ToggleBlade();
					WiimoteView.QueueDraw();
				});
			}
		}
		else if((_Buttons & cwiid.CWIID_BTN_FLAGS.BTN_B) == cwiid.CWIID_BTN_FLAGS.BTN_B)
		{
			Gtk.Application.Invoke (delegate {
				WiimoteView.ToggleTexture();
				WiimoteView.QueueDraw();
			});
		}
	}

	protected virtual void OnCalibrateButtonClicked (object sender, System.EventArgs e)
	{
		m_bCalibrateMode = true;
		m_nCalibratePosition = 0;
		WiimoteView.SetCalibrateMode(true);
	}
}