// /home/jestin/Projects/GTKWii/GTKWii/Main.cs created with MonoDevelop
// User: jestin at 6:19 PM 12/24/2007
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
// project created on 12/24/2007 at 6:19 PM
using System;
using Gtk;

namespace GTKWii
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