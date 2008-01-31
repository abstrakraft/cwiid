// /home/jestin/Projects/GTKWii/GTKWii/WiimoteGL.cs created with MonoDevelop
// User: jestin at 12:52 AM 1/17/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//

using System;
using GtkGL;
using Tao.OpenGl;

namespace GTKWii
{
	
	
	public partial class WiimoteGL : Gtk.Bin
	{
		static GtkGL.GLArea glArea = null;
		
		public WiimoteGL()
		{
			this.Build();
			
			this.Add(glArea);
		}
	}
}
