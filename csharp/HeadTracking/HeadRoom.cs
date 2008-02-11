// /home/jestin/Projects/HeadTracking/HeadRoom.cs created with MonoDevelop
// User: jestin at 1:00 AM 2/8/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//

using System;
using System.Drawing;
using System.Collections;
using Gtk;
using GtkGL;
using Tao.OpenGl;
using MonoWii;

namespace HeadTracking
{
	public partial class Room : Gtk.Bin
	{
		private GtkGL.GLArea glArea = null;
		private bool m_bOpenGLInitialized = false;
		
		private int[] m_TexNames = new int[1];
		private byte[,,] m_TargetImage;
		private Bitmap m_TargetBitmap;
		
		public Room()
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
		}
		
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
			Gl.glTexImage2D(Gl.GL_TEXTURE_2D, 0, Gl.GL_RGBA, m_TargetBitmap.Width, m_TargetBitmap.Height, 0, Gl.GL_RGBA, Gl.GL_UNSIGNED_BYTE, m_TargetImage);
			
			// Blending
			Gl.glBlendFunc(Gl.GL_ONE, Gl.GL_ONE_MINUS_SRC_ALPHA);

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
			
			// fog stuff
			float[] fogColor = new float[4] { 0.0f, 0.0f, 0.0f, 1.0f };
			Gl.glEnable(Gl.GL_FOG);
			Gl.glFogi(Gl.GL_FOG_MODE, Gl.GL_EXP);
			Gl.glFogfv(Gl.GL_FOG_COLOR, fogColor);
			Gl.glFogf(Gl.GL_FOG_DENSITY, 0.10f);
			Gl.glHint(Gl.GL_FOG_HINT, Gl.GL_DONT_CARE);
			Gl.glFogf(Gl.GL_FOG_START, 1.0f);
			Gl.glFogf(Gl.GL_FOG_END, 5.0f);
			
			Gl.glHint(Gl.GL_PERSPECTIVE_CORRECTION_HINT, Gl.GL_NICEST);
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
			
            // Clear the scene
            Gl.glClear(Gl.GL_COLOR_BUFFER_BIT | Gl.GL_DEPTH_BUFFER_BIT);
			
			DrawRoom();
			DrawTargets();

            // bring back buffer to front, put front buffer in back
            glArea.SwapBuffers();
		}
		
		private void DrawRoom()
		{
			// Right Wall
			for(int y = -5; y < 6; y++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f(5.0f, (float)y, -30.0f);
					Gl.glVertex3f(5.0f, (float)y, 10.0f);
				Gl.glEnd();
			}
			
			for(int z = -30; z < 11; z++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f(5.0f, 5.0f, (float)z);
					Gl.glVertex3f(5.0f, -5.0f, (float)z);
				Gl.glEnd();
			}
			
			// Left Wall
			for(int y = -5; y < 6; y++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f(-5.0f, (float)y, -30.0f);
					Gl.glVertex3f(-5.0f, (float)y, 10.0f);
				Gl.glEnd();
			}
			
			for(int z = -30; z < 11; z++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f(-5.0f, 5.0f, (float)z);
					Gl.glVertex3f(-5.0f, -5.0f, (float)z);
				Gl.glEnd();
			}
			
			// Ceiling
			for(int x = -5; x < 6; x++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f((float)x, 5.0f, -30.0f);
					Gl.glVertex3f((float)x, 5.0f, 10.0f);
				Gl.glEnd();
			}
			
			for(int z = -30; z < 11; z++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f(5.0f, 5.0f, (float)z);
					Gl.glVertex3f(-5.0f, 5.0f, (float)z);
				Gl.glEnd();
			}
			
			// Floor
			for(int x = -5; x < 6; x++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f((float)x, -5.0f, -30.0f);
					Gl.glVertex3f((float)x, -5.0f, 10.0f);
				Gl.glEnd();
			}
			
			for(int z = -30; z < 11; z++)
			{
				Gl.glBegin(Gl.GL_LINES);
					Gl.glVertex3f(5.0f, -5.0f, (float)z);
					Gl.glVertex3f(-5.0f, -5.0f, (float)z);
				Gl.glEnd();
			}
		}
		
		private void DrawTargets()
		{
			Gl.glEnable(Gl.GL_TEXTURE_2D);
			
			Gl.glTexEnvf(Gl.GL_TEXTURE_ENV, Gl.GL_TEXTURE_ENV_MODE, Gl.GL_MODULATE);
			Gl.glBindTexture(Gl.GL_TEXTURE_2D, m_TexNames[0]);
			
			// Target 1
			Gl.glBegin(Gl.GL_LINES);
				Gl.glVertex3f(2.0f, -3.0f, -30.0f);
				Gl.glVertex3f(2.0f, -3.0f, -10.0f);
			Gl.glEnd();
			
			Gl.glEnable(Gl.GL_TEXTURE_2D);
			Gl.glEnable(Gl.GL_BLEND);
			
			Gl.glBegin(Gl.GL_POLYGON);
				Gl.glNormal3f(0.0f, 0.0f, 1.0f);
				Gl.glTexCoord2f(1.0f, 1.0f); Gl.glVertex3f(2.5f, -3.5f, -10.0f);
				Gl.glTexCoord2f(0.0f, 1.0f); Gl.glVertex3f(1.5f, -3.5f, -10.0f);
				Gl.glTexCoord2f(0.0f, 0.0f); Gl.glVertex3f(1.5f, -2.5f, -10.0f);
				Gl.glTexCoord2f(1.0f, 0.0f); Gl.glVertex3f(2.5f, -2.5f, -10.0f);
			Gl.glEnd();
			
			Gl.glDisable(Gl.GL_TEXTURE_2D);
			Gl.glDisable(Gl.GL_BLEND);
			
			// Target 2
			Gl.glBegin(Gl.GL_LINES);
				Gl.glVertex3f(4.0f, -1.0f, -30.0f);
				Gl.glVertex3f(4.0f, -1.0f, -15.0f);
			Gl.glEnd();
			
			Gl.glEnable(Gl.GL_TEXTURE_2D);
			Gl.glEnable(Gl.GL_BLEND);
			
			Gl.glBegin(Gl.GL_POLYGON);
				Gl.glNormal3f(0.0f, 0.0f, 1.0f);
				Gl.glTexCoord2f(1.0f, 1.0f); Gl.glVertex3f(4.5f, -1.5f, -15.0f);
				Gl.glTexCoord2f(0.0f, 1.0f); Gl.glVertex3f(3.5f, -1.5f, -15.0f);
				Gl.glTexCoord2f(0.0f, 0.0f); Gl.glVertex3f(3.5f, -0.5f, -15.0f);
				Gl.glTexCoord2f(1.0f, 0.0f); Gl.glVertex3f(4.5f, -0.5f, -15.0f);
			Gl.glEnd();
			
			Gl.glDisable(Gl.GL_TEXTURE_2D);
			Gl.glDisable(Gl.GL_BLEND);
			
			// Target 3
			Gl.glBegin(Gl.GL_LINES);
				Gl.glVertex3f(-1.2f, -1.0f, -30.0f);
				Gl.glVertex3f(-1.2f, -1.0f, -5.0f);
			Gl.glEnd();
			
			Gl.glEnable(Gl.GL_TEXTURE_2D);
			Gl.glEnable(Gl.GL_BLEND);
			
			Gl.glBegin(Gl.GL_POLYGON);
				Gl.glNormal3f(0.0f, 0.0f, 1.0f);
				Gl.glTexCoord2f(1.0f, 1.0f); Gl.glVertex3f(-0.7f, -1.5f, -5.0f);
				Gl.glTexCoord2f(0.0f, 1.0f); Gl.glVertex3f(-1.7f, -1.5f, -5.0f);
				Gl.glTexCoord2f(0.0f, 0.0f); Gl.glVertex3f(-1.7f, -0.5f, -5.0f);
				Gl.glTexCoord2f(1.0f, 0.0f); Gl.glVertex3f(-0.7f, -0.5f, -5.0f);
			Gl.glEnd();
			
			Gl.glDisable(Gl.GL_TEXTURE_2D);
			Gl.glDisable(Gl.GL_BLEND);
			
			// Target 4
			Gl.glBegin(Gl.GL_LINES);
				Gl.glVertex3f(-3.2f, 2.5f, -30.0f);
				Gl.glVertex3f(-3.2f, 2.5f, 0.0f);
			Gl.glEnd();
			
			Gl.glEnable(Gl.GL_TEXTURE_2D);
			Gl.glEnable(Gl.GL_BLEND);
			
			Gl.glBegin(Gl.GL_POLYGON);
				Gl.glNormal3f(0.0f, 0.0f, 1.0f);
				Gl.glTexCoord2f(1.0f, 1.0f); Gl.glVertex3f(-2.7f, 2.0f, 0.0f);
				Gl.glTexCoord2f(0.0f, 1.0f); Gl.glVertex3f(-3.7f, 2.0f, 0.0f);
				Gl.glTexCoord2f(0.0f, 0.0f); Gl.glVertex3f(-3.7f, 3.0f, 0.0f);
				Gl.glTexCoord2f(1.0f, 0.0f); Gl.glVertex3f(-2.7f, 3.0f, 0.0f);
			Gl.glEnd();
			
			Gl.glDisable(Gl.GL_TEXTURE_2D);
			Gl.glDisable(Gl.GL_BLEND);
			
			// Target 5
			Gl.glBegin(Gl.GL_LINES);
				Gl.glVertex3f(0.5f, 0.5f, -30.0f);
				Gl.glVertex3f(0.5f, 0.5f, 2.0f);
			Gl.glEnd();
			
			Gl.glEnable(Gl.GL_TEXTURE_2D);
			Gl.glEnable(Gl.GL_BLEND);
			
			Gl.glBegin(Gl.GL_POLYGON);
				Gl.glNormal3f(0.0f, 0.0f, 1.0f);
				Gl.glTexCoord2f(1.0f, 1.0f); Gl.glVertex3f(0.0f, 0.0f, 2.0f);
				Gl.glTexCoord2f(0.0f, 1.0f); Gl.glVertex3f(1.0f, 0.0f, 2.0f);
				Gl.glTexCoord2f(0.0f, 0.0f); Gl.glVertex3f(1.0f, 1.0f, 2.0f);
				Gl.glTexCoord2f(1.0f, 0.0f); Gl.glVertex3f(0.0f, 1.0f, 2.0f);
			Gl.glEnd();
			
			Gl.glDisable(Gl.GL_TEXTURE_2D);
			Gl.glDisable(Gl.GL_BLEND);
		}
		
		private void LoadTexture()
		{
			m_TargetBitmap = new Bitmap("target.png");
			
			m_TargetImage = new byte[m_TargetBitmap.Height, m_TargetBitmap.Width, 4];
			
			for(int y = 0; y < m_TargetBitmap.Height; y++)
			{
				for(int x = 0; x < m_TargetBitmap.Width; x++)
				{
					m_TargetImage[y, x, 0] = m_TargetBitmap.GetPixel(x, y).R;
					m_TargetImage[y, x, 1] = m_TargetBitmap.GetPixel(x, y).G;
					m_TargetImage[y, x, 2] = m_TargetBitmap.GetPixel(x, y).B;
					m_TargetImage[y, x, 3] = m_TargetBitmap.GetPixel(x, y).A;
				}
			}
		}
		
		public void SetIR(cwiid.cwiid_ir_mesg _IR)
		{
			if (glArea.MakeCurrent() == 0)
                return;
			
			ArrayList Points = new ArrayList();
			
			for(int j = 0; j < cwiid.CWIID_IR_SRC_COUNT; j++)
			{
				cwiid.cwiid_ir_src IRSource = _IR.src.GetValue(j);
				
				if (IRSource.valid > 0)
				{
					Points.Add(new Point(IRSource.posX, IRSource.posY));
					
					if(Points.Count >= 2)
					{
						break;
					}
				}
			}
			
			if(Points.Count < 2)
			{
				return;
			}
			
			Point PointOne = (Point)Points[0];
			Point PointTwo = (Point)Points[1];
			
			float dDistance = (float)Math.Sqrt(Math.Pow(Math.Abs(PointOne.X - PointTwo.X), 2.0) +
				Math.Pow(Math.Abs(PointOne.Y - PointTwo.Y), 2.0));
			
			float dAverageX = (float)(PointOne.X + PointTwo.X) / 2.0f;
			float dAverageY = (float)(PointOne.Y + PointTwo.Y) / 2.0f;
			
			Gl.glMatrixMode(Gl.GL_MODELVIEW);
			Gl.glLoadIdentity();
			
			float dLeft = -(((cwiid.CWIID_IR_X_MAX / 2) - dAverageX) * 0.75f) / (cwiid.CWIID_IR_X_MAX / 2);
			float dUp = (((cwiid.CWIID_IR_Y_MAX / 2) - dAverageY) * 0.75f) / (cwiid.CWIID_IR_Y_MAX / 2);
			float dForward = ((dDistance - 400.0f) * 4.0f) / 400.0f;
			
			Gl.glTranslatef(dLeft, dUp, dForward);
		}
	}
}
