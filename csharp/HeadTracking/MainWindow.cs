// /home/jestin/Projects/HeadTracking/MainWindow.cs created with MonoDevelop
// User: jestin at 12:56 AM 2/8/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
using System;
using System.Drawing;
using Gtk;
using GtkGL;
using Tao.OpenGl;
using MonoWii;

public partial class MainWindow: Gtk.Window
{	
	Wiimote MyWiimote = new Wiimote();
	
	public MainWindow (): base (Gtk.WindowType.Toplevel)
	{
		Build ();
		
		MyWiimote.MinIRInterval = 10000000;
		MyWiimote.IREvent += new IRHandler(OnWiimoteIRChanged);
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
			MyWiimote.SetReportMode(cwiid.CWIID_RPT_FLAGS.IR);
			MyWiimote.SetLEDState(MonoWii.cwiid.CWIID_LED_FLAGS.LED1_ON);
			MyWiimote.SetRumble(true);
			System.Threading.Thread.Sleep(100);
			MyWiimote.SetRumble(false);
			
			RoomTracker.GdkWindow.Fullscreen();
		}
	}
	
	private void OnWiimoteIRChanged(cwiid.cwiid_ir_mesg _IR)
	{
		Gtk.Application.Invoke (delegate {
			RoomTracker.SetIR(_IR);
			RoomTracker.QueueDraw();
		});
	}

	protected virtual void OnExitBtnClicked (object sender, System.EventArgs e)
	{
		MyWiimote.Disconnect();
		Application.Quit ();
	}
}