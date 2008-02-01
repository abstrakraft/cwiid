// /home/jestin/Projects/GLWii/GLWii/WiimoteGL.cs created with MonoDevelop
// User: jestin at 6:24 PM 1/20/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//

using System;
using System.Drawing;
using Gtk;
using GtkGL;
using Tao.OpenGl;
using MonoWii;

namespace GLWii
{
	public partial class WiimoteGL : Gtk.Bin
	{
		public double X_GRAV_MAX = 155;
		public double X_GRAV_ZERO = 129;
		
		public double Y_GRAV_MAX = 155;
		public double Y_GRAV_ZERO = 129;
		
		public double Z_GRAV_MAX = 155;
		public double Z_GRAV_ZERO = 129;
		
		private GtkGL.GLArea glArea = null;
		
		private int[] m_TexNames = new int[1];
		private byte[,,] m_WiimoteImage;
		private Bitmap m_WiimoteBitmap;
		private bool m_bOpenGLInitialized = false;
		private bool m_bDrawBlade = false;
		private bool m_bDrawTexture = true;
		private bool m_bCalibrateMode = false;
		
		public WiimoteGL()
		{
			this.Build();
			
			// GLArea
            int[] attrlist =
			{
			    (int)GtkGL._GDK_GL_CONFIGS.Rgba,
			    (int)GtkGL._GDK_GL_CONFIGS.RedSize,1,
			    (int)GtkGL._GDK_GL_CONFIGS.GreenSize,1,
			    (int)GtkGL._GDK_GL_CONFIGS.BlueSize,1,
			    (int)GtkGL._GDK_GL_CONFIGS.DepthSize,1,
			    (int)GtkGL._GDK_GL_CONFIGS.Doublebuffer,
			    (int)GtkGL._GDK_GL_CONFIGS.None,
			};
			
			glArea = new GLArea(attrlist);
			
			this.Add(glArea);
			
			// Connect signals		
            glArea.ExposeEvent += new ExposeEventHandler(OnGlAreaExposeEvent);
            glArea.Shown += new EventHandler(OnGlAreaShown);
            glArea.SizeAllocated += new SizeAllocatedHandler(OnGlAreaSizeAllocated);
            glArea.ConfigureEvent += new ConfigureEventHandler(OnGlAreaConfigureEvent);
            glArea.SetSizeRequest(400, 400);
			
			// Activate an IdleTimer
            //GLib.Idle.Add(new GLib.IdleHandler(OnIdle));
		}
		
		private bool OnIdle()
		{
			return true;
		}
		
		private void OnGlAreaShown(object sender, EventArgs e)
		{
			if (glArea.MakeCurrent() == 0)
                return;

            // Run the state setup routine
            InitOpenGL();
		}
		
		private void OnGlAreaConfigureEvent(object o, ConfigureEventArgs args)
		{
			if (glArea.MakeCurrent() == 0)
                return;

            Gl.glViewport(0, 0, glArea.Allocation.Width, glArea.Allocation.Height);
		}
		
		private void OnGlAreaSizeAllocated(object o, SizeAllocatedArgs args)
		{
			int height = args.Allocation.Height, width = args.Allocation.Width;
			
			Resize(width, height);
		}
		
		private void Resize(int _nWidth, int _nHeight)
		{
			if (glArea.MakeCurrent() == 0)
                return;
			
			// Avoid devide-by-zero error
            if (_nHeight == 0)
            {
                _nHeight = 1;
            }

            // Set our Viewport size
            Gl.glViewport(0, 0, _nWidth, _nHeight);

            Gl.glMatrixMode(Gl.GL_PROJECTION);				// Select The Projection Matrix
            Gl.glLoadIdentity();							// Reset The Projection Matrix

            // Calculate The Aspect Ratio Of The Window
            // http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/glu/perspective.html
            Tao.OpenGl.Glu.gluPerspective(60.0f, (float)_nWidth / (float)_nHeight, 0.1f, 30.0f);

            Gl.glTranslatef(0.0f, 0.0f, -3.6f);				// Move away from the drawing area 6.0

            Gl.glMatrixMode(Gl.GL_MODELVIEW);				// Select The Modelview Matrix
            Gl.glLoadIdentity();							// Reset The Modelview Matrix
		}
		
		private void OnGlAreaExposeEvent(object o, ExposeEventArgs args)
		{
			if (glArea.MakeCurrent() == 0)
                return;
			
			if(!m_bOpenGLInitialized)
			{
				InitOpenGL();
				Resize(glArea.WidthRequest, glArea.HeightRequest);
				m_bOpenGLInitialized = true;
			}
			
            //Console.WriteLine("Exposed");
            // Clear the scene
            Gl.glClear(Gl.GL_COLOR_BUFFER_BIT | Gl.GL_DEPTH_BUFFER_BIT);

            // Replace current matrix with the identity matrix
            //Gl.glLoadIdentity();
			
			// Draw the wiimote
			DrawWiimote();
			
			if(m_bDrawBlade)
			{
				DrawBlade();
			}

            // bring back buffer to front, put front buffer in back
            glArea.SwapBuffers();
		}
		
		public void SetAcc(double dX, double dY, double dZ)
		{
			double dValueX = (double)(dX - X_GRAV_ZERO) / (double)(X_GRAV_MAX - X_GRAV_ZERO);
			double dValueY = (double)(dY - Y_GRAV_ZERO) / (double)(Y_GRAV_MAX - Y_GRAV_ZERO);
			double dValueZ = (double)(dZ - Z_GRAV_ZERO) / (double)(Z_GRAV_MAX - Z_GRAV_ZERO);
			
			if(dValueY >= -1.0 && dValueY <= 1.0 &&
			   dValueX >= -1.0&& dValueX <= 1.0)
			{
				float dPitch = -(float)(180 / Math.PI) * (float)Math.Asin(dValueY);
				float dRoll = (float)(180 / Math.PI) * (float)Math.Asin(dValueX);
				
				if(dZ < Z_GRAV_ZERO)
				{
					dRoll = 180 - dRoll;
				}
				
				//System.Console.WriteLine("Pitch: " + dPitch + "  Roll: " + dRoll);
				
				if (glArea.MakeCurrent() == 0)
	                return;
				
				Gl.glLoadIdentity();
				
				// we want pitch to be measured from vertically, the way we originally draw the wiimote
				Gl.glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
				
				Gl.glRotatef(dPitch, 1.0f, 0.0f, 0.0f);
				Gl.glRotatef(dRoll, 0.0f, 1.0f, 0.0f);
			}
		}
		
		public void SetCalibrateMode(bool _bOn)
		{
			m_bCalibrateMode = _bOn;
		}
		
		// One-time configuration of opengl states happens here
        private void InitOpenGL()
        {
			Gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			
			// Texture stuff
			Gl.glShadeModel(Gl.GL_SMOOTH);
			Gl.glEnable(Gl.GL_DEPTH_TEST);
			LoadTexture();
			Gl.glPixelStorei(Gl.GL_UNPACK_ALIGNMENT, 1);

			Gl.glGenTextures(1, m_TexNames);
			Gl.glBindTexture(Gl.GL_TEXTURE_2D, m_TexNames[0]);

			Gl.glTexParameteri(Gl.GL_TEXTURE_2D, Gl.GL_TEXTURE_WRAP_S, Gl.GL_REPEAT);
			Gl.glTexParameteri(Gl.GL_TEXTURE_2D, Gl.GL_TEXTURE_WRAP_T, Gl.GL_REPEAT);
			Gl.glTexParameteri(Gl.GL_TEXTURE_2D, Gl.GL_TEXTURE_MAG_FILTER, Gl.GL_NEAREST);
			Gl.glTexParameteri(Gl.GL_TEXTURE_2D, Gl.GL_TEXTURE_MIN_FILTER, Gl.GL_NEAREST);
			Gl.glTexImage2D(Gl.GL_TEXTURE_2D, 0, Gl.GL_RGBA, m_WiimoteBitmap.Width, m_WiimoteBitmap.Height, 0, Gl.GL_RGBA, Gl.GL_UNSIGNED_BYTE, m_WiimoteImage);

			// Lighting stuff
			float[] mat_specular = new float[4] { 1.0f, 1.0f, 1.0f, 1.0f};
			float[] mat_shininess = new float[1] {50.0f};
			float[] light_position = new float[4] {-1.0f, 1.0f, 2.0f, 0.0f};
			float[] white_light = new float[4] { 1.0f, 1.0f, 1.0f, 1.0f};
			float[] amibient_light = new float[4] { 1.0f, 1.0f, 1.0f, 1.0f};
			float[] lmodel_ambiant = new float[4] {0.1f, 0.1f, 0.1f, 1.0f};

			Gl.glMaterialfv(Gl.GL_FRONT, Gl.GL_SPECULAR, mat_specular);
			Gl.glMaterialfv(Gl.GL_FRONT, Gl.GL_SHININESS, mat_shininess);
			Gl.glLightfv(Gl.GL_LIGHT0, Gl.GL_POSITION, light_position);
			Gl.glLightfv(Gl.GL_LIGHT0, Gl.GL_DIFFUSE, white_light);
			Gl.glLightfv(Gl.GL_LIGHT0, Gl.GL_SPECULAR, white_light);
			Gl.glLightfv(Gl.GL_LIGHT0, Gl.GL_AMBIENT, amibient_light);
			Gl.glLightModelfv(Gl.GL_LIGHT_MODEL_AMBIENT, lmodel_ambiant);

			Gl.glEnable(Gl.GL_LIGHTING);
			Gl.glEnable(Gl.GL_LIGHT0);
			Gl.glEnable(Gl.GL_NORMALIZE);
			Gl.glEnable(Gl.GL_GENERATE_MIPMAP);
			
			Gl.glHint(Gl.GL_PERSPECTIVE_CORRECTION_HINT, Gl.GL_NICEST);
        }
		
		private void LoadTexture()
		{
			m_WiimoteBitmap = new Bitmap("Wiimote.png");
			
			m_WiimoteImage = new byte[m_WiimoteBitmap.Height, m_WiimoteBitmap.Width, 4];
			
			for(int y = 0; y < m_WiimoteBitmap.Height; y++)
			{
				for(int x = 0; x < m_WiimoteBitmap.Width; x++)
				{
					m_WiimoteImage[y, x, 0] = m_WiimoteBitmap.GetPixel(x, y).R;
					m_WiimoteImage[y, x, 1] = m_WiimoteBitmap.GetPixel(x, y).G;
					m_WiimoteImage[y, x, 2] = m_WiimoteBitmap.GetPixel(x, y).B;
					m_WiimoteImage[y, x, 3] = m_WiimoteBitmap.GetPixel(x, y).A;
				}
			}
		}
		
		private void DrawWiimote()
		{
			if(m_bCalibrateMode)
			{
				Gl.glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				Gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			}
			
			if(m_bDrawTexture)
			{
				Gl.glEnable(Gl.GL_TEXTURE_2D);
			}

			Gl.glTexEnvf(Gl.GL_TEXTURE_ENV, Gl.GL_TEXTURE_ENV_MODE, Gl.GL_MODULATE);
			Gl.glBindTexture(Gl.GL_TEXTURE_2D, m_TexNames[0]);

			// Front
			Gl.glBegin(Gl.GL_QUADS);
				Gl.glNormal3f(0.0f, 0.0f, 1.0f);
				Gl.glTexCoord2f(0.0f, 1.0f);  Gl.glVertex3f(-0.3f, -1.3f, 0.2f);
				Gl.glTexCoord2f(0.0f, 0.0f);  Gl.glVertex3f(-0.3f, 1.3f, 0.2f);
				Gl.glTexCoord2f(1.0f, 0.0f);  Gl.glVertex3f(0.3f, 1.3f, 0.2f);
				Gl.glTexCoord2f(1.0f, 1.0f);  Gl.glVertex3f(0.3f, -1.3f, 0.2f);
			Gl.glEnd();

			Gl.glDisable(Gl.GL_TEXTURE_2D);
			
			// Back
			Gl.glBegin(Gl.GL_QUADS);
				Gl.glNormal3f(0.0f, 0.0f, -1.0f);
				Gl.glVertex3f(-0.3f, -1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, 1.3f, -0.2f);
				Gl.glVertex3f(0.3f, 1.3f, -0.2f);
				Gl.glVertex3f(0.3f, -1.3f, -0.2f);
			Gl.glEnd();

			// Right Side
			Gl.glBegin(Gl.GL_QUADS);
				Gl.glNormal3f(1.0f, 0.0f, 0.0f);
				Gl.glVertex3f(0.3f, -1.3f, -0.2f);
				Gl.glVertex3f(0.3f, 1.3f, -0.2f);
				Gl.glVertex3f(0.3f, 1.3f, 0.2f);
				Gl.glVertex3f(0.3f, -1.3f, 0.2f);
			Gl.glEnd();

			// Left Side
			Gl.glBegin(Gl.GL_QUADS);
				Gl.glNormal3f(-1.0f, 0.0f, 0.0f);
				Gl.glVertex3f(-0.3f, -1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, 1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, 1.3f, 0.2f);
				Gl.glVertex3f(-0.3f, -1.3f, 0.2f);
			Gl.glEnd();

			// Top Side
			Gl.glBegin(Gl.GL_QUADS);
				Gl.glNormal3f(0.0f, 1.0f, 0.0f);
				Gl.glVertex3f(0.3f, 1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, 1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, 1.3f, 0.2f);
				Gl.glVertex3f(0.3f, 1.3f, 0.2f);
			Gl.glEnd();

			// Bottom Side
			Gl.glBegin(Gl.GL_QUADS);
				Gl.glNormal3f(0.0f, -1.0f, 0.0f);
				Gl.glVertex3f(0.3f, -1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, -1.3f, -0.2f);
				Gl.glVertex3f(-0.3f, -1.3f, 0.2f);
				Gl.glVertex3f(0.3f, -1.3f, 0.2f);
			Gl.glEnd();
		}
		
		private void DrawBlade()
		{
			Gl.glEnable(Gl.GL_BLEND);
			Gl.glDisable(Gl.GL_LIGHTING);

			Gl.glBlendFunc(Gl.GL_SRC_ALPHA, Gl.GL_ONE_MINUS_SRC_ALPHA);


			float dRadius = 0.02f;
			float dR = 1.0f;
			float dG = 1.0f;
			float dB = 1.0f;
			float dA = 1.0f;

			while(dRadius < 0.2f)
			{
				Gl.glColor4f(dR, dG, dB, dA);

				for(int i = 0; i < 16; i++)
				{
					int nNext = (i + 1 >= 16)? 0 : i + 1;
					
					float dFirstX = (float)Math.Cos((i * 2 * Math.PI) /16) * dRadius;
					float dFirstZ = (float)Math.Sin((i * 2 * Math.PI) / 16) * dRadius;
					float dSecondX = (float)Math.Cos((nNext * 2 * Math.PI) / 16) * dRadius;
					float dSecondZ = (float)Math.Sin((nNext * 2 * Math.PI) / 16) * dRadius;

					Gl.glBegin(Gl.GL_QUADS);
						Gl.glNormal3f(dFirstX, 0.0f, dFirstZ);
						Gl.glVertex3f(dFirstX, 1.3f, dFirstZ);
						Gl.glVertex3f(dFirstX, 10.0f, dFirstZ);
						Gl.glNormal3f(dSecondX, 0.0f, dSecondZ);
						Gl.glVertex3f(dSecondX, 10.0f, dSecondZ);
						Gl.glVertex3f(dSecondX, 1.3f, dSecondZ);
					Gl.glEnd();
				}

				dRadius += 0.01f;
				dR = (dR * 4) / 5;
				dB = (dB * 4) / 5;
				dA = (dA * 4) / 5;
			}

			Gl.glDisable(Gl.GL_BLEND);

			Gl.glEnable(Gl.GL_LIGHTING);
		}
		
		public void ToggleBlade()
		{
			m_bDrawBlade = !m_bDrawBlade;
		}
		
		public void ToggleTexture()
		{
			m_bDrawTexture = !m_bDrawTexture;
		}
	}
}
