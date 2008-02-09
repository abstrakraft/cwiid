// /home/jestin/Projects/HeadTracking/Main.cs created with MonoDevelop
// User: jestin at 12:56 AM 2/8/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
// project created on 2/8/2008 at 12:56 AM
using System;
using Gtk;


namespace HeadTracking
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			Application.Init ();
			MainWindow win = new MainWindow ();
			win.Show ();
			Application.Run ();
		}
	}
}