// /home/jestin/Projects/GLWii/GLWii/Main.cs created with MonoDevelop
// User: jestin at 6:22 PM 1/20/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
// project created on 1/20/2008 at 6:22 PM
using System;
using Gtk;

namespace GLWii
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