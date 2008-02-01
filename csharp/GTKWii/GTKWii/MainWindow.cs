// /home/jestin/Projects/GTKWii/GTKWii/MainWindow.cs created with MonoDevelop
// User: jestin at 6:19 PM 12/24/2007
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
using System;
using System.Drawing;
using System.Collections;
using Cairo;
using Gtk;
using MonoWii;

public partial class MainWindow: Gtk.Window
{
	private Wiimote MyWiimote;
	
	private cwiid.cwiid_ir_mesg m_IRMesg;
	private byte[] m_NunchukStick = new Byte[2];
	
	public MainWindow (): base (Gtk.WindowType.Toplevel)
	{
		Build ();
		
		MyWiimote = new Wiimote();
		MyWiimote.ButtonEvent += new ButtonHandler(OnWiimoteButtonPressed);
		MyWiimote.RawAccelEvent += new RawAccelHandler(OnWiimoteAccelChanged);
		MyWiimote.IREvent += new IRHandler(OnWiimoteIRChanged);
		MyWiimote.NunchukButtonEvent += new NunchukButtonHandler(OnNunchukButtonPressed);
		MyWiimote.NunchukAccelEvent += new NunchukAccelHandler(OnNunchukAccelChanged);
		MyWiimote.NunchukStickEvent += new NunchukStickHandler(OnNunchukStick);
	}
	
	protected void OnDeleteEvent (object sender, DeleteEventArgs a)
	{
		MyWiimote.Disconnect();
		Application.Quit ();
		a.RetVal = true;
	}

	protected virtual void OnConnectButtonClicked (object sender, System.EventArgs e)
	{
		if(MyWiimote.Connect())
		{
			StatusLabel.Text = "Connected";
			MyWiimote.SetReportMode(cwiid.CWIID_RPT_FLAGS.BTN | cwiid.CWIID_RPT_FLAGS.IR | cwiid.CWIID_RPT_FLAGS.ACC | cwiid.CWIID_RPT_FLAGS.EXT);
		}
		else
		{
			StatusLabel.Text = "Disconnected";
		}
	}

	private void OnWiimoteButtonPressed(cwiid.CWIID_BTN_FLAGS _Buttons)
	{
		Gtk.Application.Invoke (delegate {
             ButtonLabel.Text = (_Buttons != 0) ? _Buttons.ToString() : "<no button>";
          });
	}
	
	private void OnWiimoteAccelChanged(cwiid.cwiid_acc_mesg _Acc)
	{
		Gtk.Application.Invoke (delegate {
			XAccelBar.Fraction = _Acc.accX / (double) cwiid.CWIID_ACC_MAX;
			XAccelBar.Text = _Acc.accX.ToString();
			
			YAccelBar.Fraction = (double) _Acc.accY / (double) cwiid.CWIID_ACC_MAX;
			YAccelBar.Text = _Acc.accY.ToString();
			
			ZAccelBar.Fraction = (double) _Acc.accZ / (double) cwiid.CWIID_ACC_MAX;
			ZAccelBar.Text = _Acc.accZ.ToString();
          });
	}
	
	private void OnWiimoteIRChanged(cwiid.cwiid_ir_mesg _IR)
	{
		m_IRMesg = _IR;
		
		Gtk.Application.Invoke (delegate {
			InfraRedArea.QueueDraw();
		});
	}
	
	protected virtual void OnLED1Toggled (object sender, System.EventArgs e)
	{
		if(LED1Toggle.Active)
		{
			MyWiimote.SetLED(Wiimote.LEDS.LED1);
		}
		else
		{
			MyWiimote.UnSetLED(Wiimote.LEDS.LED1);
		}
	}

	protected virtual void OnLED2Toggled (object sender, System.EventArgs e)
	{
		if(LED2Toggle.Active)
		{
			MyWiimote.SetLED(Wiimote.LEDS.LED2);
		}
		else
		{
			MyWiimote.UnSetLED(Wiimote.LEDS.LED2);
		}
	}

	protected virtual void OnLED3Toggled (object sender, System.EventArgs e)
	{
		if(LED3Toggle.Active)
		{
			MyWiimote.SetLED(Wiimote.LEDS.LED3);
		}
		else
		{
			MyWiimote.UnSetLED(Wiimote.LEDS.LED3);
		}
	}

	protected virtual void OnLED4Toggled (object sender, System.EventArgs e)
	{
		if(LED4Toggle.Active)
		{
			MyWiimote.SetLED(Wiimote.LEDS.LED4);
		}
		else
		{
			MyWiimote.UnSetLED(Wiimote.LEDS.LED4);
		}
	}

	protected virtual void OnRumbleToggled (object sender, System.EventArgs e)
	{
		MyWiimote.SetRumble(RumbleToggle.Active);
	}

	protected virtual void OnIRExpose (object o, Gtk.ExposeEventArgs args)
	{
		if(MyWiimote.Connected)
		{
			DrawIR();
		}
	}
	
	private void DrawIR()
	{
		int nWidth = 0;
		int nHeight = 0;
		
		Cairo.Context gc = Gdk.CairoHelper.Create(InfraRedArea.GdkWindow);
		
		InfraRedArea.GdkWindow.GetSize(out nWidth, out nHeight);
		
		gc.Color = new Cairo.Color(0, 0, 0);
		gc.Rectangle(0, 0, nWidth, nHeight);
		gc.Fill();
		
		gc.Color = new Cairo.Color(1, 1, 1);
		
		ArrayList Points = new ArrayList();
		
		for(int j = 0; j < cwiid.CWIID_IR_SRC_COUNT; j++)
		{
			cwiid.cwiid_ir_src IRSource = m_IRMesg.src.GetValue(j);
			
			if (IRSource.valid > 0)
			{
				int nSize = 0;
				
				if(IRSource.size == -1)
				{
					nSize = 3;
				}
				else
				{
					nSize = IRSource.size + 1;
				}
				
				nSize *= 3;
				
				int nX = ((IRSource.posX * nWidth) / cwiid.CWIID_IR_X_MAX) - (nSize / 2);
				int nY = (nHeight - (IRSource.posY * nHeight) / cwiid.CWIID_IR_Y_MAX) - (nSize / 2);
				
				gc.Arc(nX, nY, nSize, 0, 2 * Math.PI);
				gc.Fill();
				
				Points.Add(new Cairo.Point(nX, nY));
			}
		}
		
		gc.Color = new Cairo.Color(1, 0, 0);
		
		for(int nIndex = 0; nIndex < Points.Count; nIndex++)
		{
			if(nIndex > 0)
			{
				Cairo.Point To = (Cairo.Point) Points[nIndex];
				gc.LineTo(To.X, To.Y);
			}
			
			Cairo.Point From = (Cairo.Point) Points[nIndex];
			gc.MoveTo(From.X, From.Y);
		}
		
		if(Points.Count > 0)
		{
			Cairo.Point First = (Cairo.Point) Points[0];
			gc.LineTo(First.X, First.Y);
		}
		
		gc.Stroke();
		
		((IDisposable) gc.Target).Dispose ();
	}
	
	private void OnNunchukButtonPressed(cwiid.CWIID_NUNCHUK_BTN_FLAGS _Buttons)
	{
		Gtk.Application.Invoke (delegate {
             NunchukButtonLabel.Text = (_Buttons != 0) ? _Buttons.ToString() : "<no button>";
			
			NunchukStatusLabel.Text = "Connected";
          });
	}
	
	private void OnNunchukAccelChanged(byte _accX, byte _accY, byte _accZ)
	{
		Gtk.Application.Invoke (delegate {
			NunchukXAccelBar.Fraction = (double) _accX / (double) cwiid.CWIID_ACC_MAX;
			NunchukXAccelBar.Text = _accX.ToString();
			
			NunchukYAccelBar.Fraction = (double) _accY / (double) cwiid.CWIID_ACC_MAX;
			NunchukYAccelBar.Text = _accY.ToString();
			
			NunchukZAccelBar.Fraction = (double) _accZ / (double) cwiid.CWIID_ACC_MAX;
			NunchukZAccelBar.Text = _accZ.ToString();
          });
	}
	
	private void OnNunchukStick(byte _StickX, byte _StickY)
	{
		m_NunchukStick[0] = _StickX;
		m_NunchukStick[1] = _StickY;
		
		Gtk.Application.Invoke (delegate {
			NunchukStickArea.QueueDraw();
		});
	}

	protected virtual void OnNunchukStickExposed (object o, Gtk.ExposeEventArgs args)
	{
		if(MyWiimote.Connected)
		{
			DrawNunchukStick();
		}
	}
	
	private void DrawNunchukStick()
	{
		int nWidth = 0;
		int nHeight = 0;
		int nMin = 0;
		int nSize = 0;
		int nX = 0;
		int nY = 0;
		
		Cairo.Context gc = Gdk.CairoHelper.Create(NunchukStickArea.GdkWindow);
		
		NunchukStickArea.GdkWindow.GetSize(out nWidth, out nHeight);
		
		nMin = Math.Min(nWidth, nHeight);
		
		nSize = nMin / 5;
		
		gc.Color = new Cairo.Color(0, 0, 0);
		
		gc.LineWidth = nSize / 4;
		
		gc.Arc(nWidth / 2, nHeight / 2, (nMin / 2) - 2, 0, Math.PI * 2);
		gc.Stroke();
		
		nX = ((nWidth / 2) - (nMin / 2)) + ((m_NunchukStick[0] * nMin) / 0xFF);
		nY = ((nHeight / 2) - (nMin / 2)) + (((0xFF - m_NunchukStick[1]) * nMin) / 0xFF);
		
		gc.Arc(nX,
		       nY,
		       nSize, 0, Math.PI * 2);
		gc.Fill();
		
		gc.LineCap = LineCap.Round;
		gc.MoveTo(nWidth / 2, nHeight / 2);
		gc.LineTo(nX, nY);
		gc.Stroke();
		
		((IDisposable) gc.Target).Dispose ();
	}
}