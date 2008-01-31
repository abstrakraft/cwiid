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
	public static class Bluetooth
	{
		// Bluetooth Section
		
		[StructLayout(LayoutKind.Sequential)]
		public struct bdaddr_t
		{
			public byte b0;
			public byte b1;
			public byte b2;
			public byte b3;
			public byte b4;
			public byte b5;
			
			public bdaddr_t(byte bd5, byte bd4, byte bd3, byte bd2, byte bd1, byte bd0)
			{
				this.b0 = bd0;
				this.b1 = bd1;
				this.b2 = bd2;
				this.b3 = bd3;
				this.b4 = bd4;
				this.b5 = bd5;
			}
			
			public bdaddr_t(string addressString)
			{
				Bluetooth.str2ba(addressString, ref this);
			}
			
			public override string ToString()
			{
				return b5.ToString("X2") + ":" +b4.ToString("X2") + ":" +b3.ToString("X2") + ":" +b2.ToString("X2") + ":" +b1.ToString("X2") + ":" +b0.ToString("X2");
			}
		}
		
		public static bdaddr_t BDADDR_ANY = new bdaddr_t(); 
		public static bdaddr_t BDADDR_ALL = new bdaddr_t( 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
		public static bdaddr_t BDADDR_LOCAL = new bdaddr_t(0, 0, 0, 0xff, 0xff, 0xff);
		
		// int str2ba(const char *str, bdaddr_t *ba);
		[DllImport("libbluetooth.so.2")]
		public static extern int str2ba(string addressString, ref bdaddr_t ba);
	}
}
