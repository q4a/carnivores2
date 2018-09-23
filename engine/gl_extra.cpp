
#define OPENGL_VARS
#include "gl_extra.h"
#include <string.h>

#if defined( AF_PLATFORM_LINUX )
XFontStruct* g_SmlFontInfo;
#endif

GLboolean 	g_RenderState[0xFFFF];
GLboolean	g_NonPow2 = GL_FALSE;


bool oglPointInTriangle( Vector2df A, Vector2df B, Vector2df C, Vector2df P )
{//SunScrX
	bool b0x = ( (P.x - A.x) * (A.y - B.y) > 0);
	bool b0y = ( (P.y - A.y) * (B.x - A.x) > 0);
	bool b1x = ( (P.x - B.x) * (B.y - C.y) > 0);
	bool b1y = ( (P.y - B.y) * (C.x - B.x) > 0);
	bool b2x = ( (P.x - C.x) * (C.y - A.y) > 0);
	bool b2y = ( (P.y - C.y) * (A.x - C.x) > 0);

	bool b0 = ( b0x == b0y );
	bool b1 = ( b1x == b1y );
	bool b2 = ( b2x == b2y );

	return (b0 == b1 && b1 == b2);
}

void oglAddVertex( float x, float y, float z, float rhw, float fog, unsigned long diffuse, float tu, float tv )
{

	lpVertex->x		= x;
	lpVertex->y		= y;
	lpVertex->z		= z;
	lpVertex->rhw	= rhw;
	lpVertex->fog	= fog;
	lpVertex->color = diffuse;
	lpVertex->tu	= tu;
	lpVertex->tv	= tv;
	lpVertex++;

	pGLVerticesCount++;
}

bool oglInit()
{
	return true;
}

bool oglDeinit()
{
#if defined( AF_PLATFORM_LINUX )
	XFree( g_SmlFontInfo );
#elif defined( AF_PLATFORM_WINDOWS )
	// FooBar
#endif
	return true;
}

/*HGLRC oglCreateDummyContext( HDC* pDevice )
{
	//Create a dummy context for temporary use
	int			pf_dummy		= 0;
	HGLRC		hglrc_dummy		= 0;
	HDC			hdc_dummy		= 0;
	WNDCLASS	wc;
	HWND		wnd;

	ZeroMemory( &wc, sizeof(wc) );
	wc.style =			CS_OWNDC;
    wc.lpfnWndProc =	(WNDPROC)TempWinProc;
    wc.cbClsExtra =		0;
    wc.cbWndExtra =		0;
    wc.hInstance =		(HINSTANCE)GetModuleHandle( 0 );
    wc.hIcon =			(HICON)LoadIcon((HINSTANCE)hInst,"ACTION");
    wc.hCursor =		NULL;
	wc.hbrBackground =	(HBRUSH)GetStockObject( BLACK_BRUSH );
    wc.lpszMenuName =	NULL;
	wc.lpszClassName =	"DUMMY_OPENGL";

	if ( !RegisterClass( &wc ) )
	{
		DoHalt( "oglCreateDummyContext() -> Failed to register dummy class!" );
		return 0;
	}

	wnd = CreateWindow( wc.lpszClassName,"lol",WS_CAPTION | WS_SYSMENU | WS_BORDER, 0,0,0,0, 0,0,wc.hInstance,0);

	if ( !wnd )
	{
		uint32_t winerr = GetLastError();
		printf( "Windows Error: %d\n", winerr );
		DoHalt( "oglCreateDummyContext() -> Failed to create dummy window!" );
		return 0;
	}

	hdc_dummy = CreateDC( "DISPLAY", 0, 0, 0 );

	ZeroMemory (&pfd, sizeof (pfd));
    pfd.nSize = sizeof (pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

	pf_dummy = ChoosePixelFormat (hdc_dummy, &pfd);

	if ( pf_dummy == 0 )
	{
		DoHalt( "oglCreateDummyContext() -> Failed to retrieve a dummy pixel format!" );
		return 0;
	}

	if ( !SetPixelFormat (hdc_dummy, pf_dummy, &pfd) )
	{
		DoHalt( "oglCreateDummyContext() -> Failed to set the dummy pixel format!" );
		return 0;
	}

	hglrc_dummy = wglCreateContext( hdc_dummy );
	wglMakeCurrent( hdc_dummy, hglrc_dummy );

	if ( hglrc_dummy == 0 )
	{
		DoHalt( "oglCreateDummyContext() -> Failed to create the dummy Render Context!" );
		return 0;
	}

	// -- Initialize GLEW
	if ( hglrc_dummy != 0 )
	{
		GLenum _GLErrorCode = glewInit();
		if ( _GLErrorCode != GLEW_OK )
		{
			const GLubyte* errstr = glewGetErrorString( _GLErrorCode );
			char t[512];
			sprintf( t, "oglCreateDummyContext() -> Failed to initialize GLEW!\n%s\n", errstr );
			DoHalt( t );
			return 0;
		}
	}

	DestroyWindow( wnd );
	UnregisterClass( wc.lpszClassName, wc.hInstance );

	return hglrc_dummy;
}*/

/*HGLRC oglCreateContext( HDC pDevice, int* pDisplayAttributes, int* pContextAttributes )
{
	if ( pDisplayAttributes == 0 )
	{
		DoHalt( "oglCreateContext() -> pDisplayAttributes is empty!" ); return 0;
	}
	if ( pContextAttributes == 0 )
	{
		DoHalt( "oglCreateContext() -> pContextAttributes is empty!" ); return 0;
	}
	if ( pDevice == 0 )
	{
		DoHalt( "oglCreateContext() -> pDevice is null!" ); return 0;
	}

	// Create a dummy rendering context in a dummy window
	HGLRC		hglrc_dummy		= 0;

	ZeroMemory (&pfd, sizeof (pfd));
    pfd.nSize = sizeof (pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = pDisplayAttributes[11];
    pfd.cDepthBits = pDisplayAttributes[13];
    pfd.iLayerType = PFD_MAIN_PLANE;

	if ( 0 == (iFormat = ChoosePixelFormat (pDevice, &pfd)) )
	{
		DoHalt( "oglCreateContext() -> Failed to retrieve a dummy pixel format!" );
		return 0;
	}

	if ( !SetPixelFormat (pDevice, iFormat, &pfd) )
	{
		DoHalt( "oglCreateContext() -> Failed to set the dummy pixel format!" );
		return 0;
	}

	if ( 0 == (hglrc_dummy = wglCreateContext( pDevice )) )
	{
		DoHalt( "oglCreateContext() -> Failed to create the dummy Render Context!" );
		return 0;
	}

	wglMakeCurrent( pDevice, hglrc_dummy );

	GLenum _GLErrorCode = glewInit();
	if ( _GLErrorCode != GLEW_OK )
	{
		const GLubyte* errstr = glewGetErrorString( _GLErrorCode );
		char t[512];
		sprintf( t, "oglCreateContext() -> Failed to initialize GLEW!\n%s\n", errstr );
		DoHalt( t );
		return 0;
	}

	if ( WGLEW_ARB_pixel_format )
	{
		unsigned int iNumFormats = 0;

		if ( !wglChoosePixelFormatARB( pDevice, pDisplayAttributes, 0, 1, &iFormat, &iNumFormats ) )
		{
			printf( "oglCreateContext() - > Failed to choose the ARB pixel format!\r\n" );
			return hglrc_dummy;
		}

		SetPixelFormat( pDevice, iFormat, &pfd );
	}
	else
	{
		return hglrc_dummy;
	}

	return hglrc_dummy;
}*/

unsigned int oglLoadShader( const char* fname )
{

	FILE* fp = fopen( fname, "rb" );

	if ( fp == 0 )
	{
		PrintLog( (char*)fname );
		PrintLog( "\t->\tFailed to load the shader!\n" );
		return 0;
	}


	fclose( fp );

    return 0;
}


void oglCreateFont()
{
#if defined( AF_PLATFORM_LINUX )

	Display* dp = XOpenDisplay( 0 );

	/*char **names;
	int name_c = 0;

	names = XListFonts( dp, "*", 1024, &name_c );

	for ( int i=0; i<name_c; ++i )
	{
		fprintf( hlog, "font[%d]: \"%s\"\n", i, names[i] );
	}

	XFreeFontNames( names );
	names = 0;*/

	// Load in some default font glyphs as a texture
	XFontStruct *fontInfo;
	Font id;
	unsigned int first, last;
	fontInfo = XLoadQueryFont(dp, "fixed" );//"-misc-fixed-normal-o-normal--10-120-75-75-c-80-iso8859-5");
	g_SmlFontInfo = fontInfo;

	if(!fontInfo)
	{
		printf("Failed to query the X Font!");
		return;
	}

	id = fontInfo->fid;
	first = fontInfo->min_char_or_byte2;
	last = fontInfo->max_char_or_byte2;
	if ( !pFont) pFont = glGenLists((GLuint) last + 1);

	if(!pFont)
	{
		printf("Error: unable to allocate display lists");
		return;
	}

	glXUseXFont(id, first, last - first + 1, pFont + first);

	XCloseDisplay( dp );

#else // windows

	//if ( HARD3D == false ) return;
	//if ( glMajor >= 3 ) return;

    HFONT   font;
    HFONT   oldfont;

	int H = 10;
	if (WinH>=600) H = 12;
	if (WinH>=768) H = 14;
	//if (WinH>=1024) H = 16;

    if (!pFont) pFont = glGenLists(255);
	if (pFont == 0)
	{
		DoHalt( "glGenLists() <- oglCreateFont() == Failed!" );
		return;
	}

    font = CreateFont(  -H,0,
                        0,0,
                        FW_NORMAL,//FW_BOLD,
                        false,false,false,
                        ANSI_CHARSET,//RUSSIAN_CHARSET,
                        OUT_TT_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        ANTIALIASED_QUALITY,
                        FF_DONTCARE|DEFAULT_PITCH,
                        "Arial");

    oldfont = (HFONT)SelectObject(hdcMain, (HFONT)GetStockObject(DEFAULT_GUI_FONT));

    wglUseFontBitmaps(hdcMain,0,255, pFont);

    SelectObject(hdcMain,oldfont);
    DeleteObject(font);

	PrintLog( "OpenGL Font Created!\n" );

	//font = (HFONT)SelectObject(DeviceContext, (HFONT)GetStockObject(DEFAULT_GUI_FONT));
    //wglUseFontBitmaps(DeviceContext,0,255, pFont);
    //SelectObject(DeviceContext,font);
#endif
}

void oglGetCaps()
{
	if ( HARD3D == false ) return;

	memset( g_RenderState, 0, sizeof(GLboolean)*0xFFFF );

	// -- Old style OpenGL 2.1 and previous version retrieval
    char *glVerStr = (char*)glGetString(GL_VERSION);

	if ( glVerStr )
	{
		//glMajor = glVerStr[0]-48;
		//glMinor = glVerStr[2]-48;
		//glRevision = glVerStr[4]-48;

		sprintf( logt, "\tOpenGL Version: %s\n", glVerStr );
		PrintLog( logt );
	}

	glGetIntegerv( GL_MAJOR_VERSION, &glMajor );
	glGetIntegerv( GL_MINOR_VERSION, &glMinor );

	sprintf( logt, "\tCore: %d.%d\n", glMajor, glMinor );
	PrintLog( logt );

	sprintf( logt, "\tVendor: %s\n", (char*)glGetString(GL_VENDOR) ); PrintLog(logt);
	sprintf( logt, "\tRenderer: %s\n", (char*)glGetString(GL_RENDERER) ); PrintLog(logt);

	GLint Red,Green,Blue;
	glGetIntegerv( GL_RED_BITS, &Red );
	glGetIntegerv( GL_GREEN_BITS, &Green );
	glGetIntegerv( GL_BLUE_BITS, &Blue );
	//VFORMAT565 = false
	//if ( Red==5 && Green==6 && Blue==5 ) VFORMAT565 = true;

	sprintf( logt, "\tBackBufferFormat: RGB%d%d%d\n", Red, Green, Blue );
    PrintLog( logt );

	{
		GLint iTexSize = 0;
		glGetIntegerv( GL_MAX_TEXTURE_SIZE, &iTexSize);
		sprintf( logt, "\tMaxTextureWidth: %dx%d\n", iTexSize,iTexSize);
		PrintLog( logt );
	}

	AnisoTexture = GLEW_EXT_texture_filter_anisotropic;
	if ( AnisoTexture )
	{
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fMaxAniso);
        sprintf( logt, "\tMaxAnisotropy: %0.0fx\n",fMaxAniso);
        PrintLog( logt );
	}
	if ( GLEW_ARB_vertex_buffer_object )	PrintLog("\tVertex Buffer Object Supported!\n");
	if ( GLEW_EXT_framebuffer_object )		PrintLog("\tFrame Buffer Object Supported!\n");
	if ( GLEW_EXT_separate_specular_color )	PrintLog("\tSeparate Specular Color Supported!\n");
	if ( GLEW_EXT_fog_coord )				PrintLog("\tFog Coordinate Supported!\n");
	//if ( GLEW_ARB_texture_non_power_of_two ){PrintLog("\tNon Power of 2 Textures Supported!\n");g_NonPow2=GL_TRUE;}
	//if ( WGLEW_EXT_swap_control )			PrintLog("\tV-Sync available!\n");
	//if ( WGLEW_ARB_multisample )			PrintLog("\tMultisampling available!\n" );

	//NightVision = GLLoadShader("nightvision.shader");
}

void oglDeleteFont()
{
	if (pFont==0) return;
	glDeleteLists(pFont,255);
	pFont = 0;
}

void oglDrawTriangle(GLVertex &vtx1, GLVertex &vtx2, GLVertex &vtx3)
{
	// -> Draws a Triangle or adds it to the buffer

    if (!NightVision && OptDayNight==2)
    {
        vtx1.color = (((vtx1.color>>24) & 255)<<24) | DarkRGBX(vtx1.color);
        vtx2.color = (((vtx2.color>>24) & 255)<<24) | DarkRGBX(vtx2.color);
        vtx3.color = (((vtx3.color>>24) & 255)<<24) | DarkRGBX(vtx3.color);
    }

	glBegin(GL_TRIANGLES);

	glColor4ubv((GLubyte*)&vtx1.color);
	glTexCoord2f(vtx1.tu,vtx1.tv);
	//glFogCoordf( vtx1.fog );
	glVertex4f(vtx1.x,vtx1.y,vtx1.z,vtx1.rhw);

	glColor4ubv((GLubyte*)&vtx2.color);
	glTexCoord2f(vtx2.tu,vtx2.tv);
	//glFogCoordf( vtx2.fog );
	glVertex4f(vtx2.x,vtx2.y,vtx2.z,vtx2.rhw);

	glColor4ubv((GLubyte*)&vtx3.color);
	glTexCoord2f(vtx3.tu,vtx3.tv);
	//glFogCoordf( vtx3.fog );
	glVertex4f(vtx3.x,vtx3.y,vtx3.z,vtx3.rhw);

	glEnd();
	DrawCalls++;

	dFacesCount++;
}

/*
	Render a 'window' element for the in-game user interface
	ToDo:
	[ ] Implement a customizeable interface setup, allowing for images to make up the graphical part
	[ ] Ensure the elements are all optimized for rendering as the game will continue to render in the background
	[ ]
*/
void oglDrawWindow( int pLeft, int pTop, int pWidth, int pHeight )
{
	if ( HARD3D == false ) return;

	// -- We aren't using any images
	oglSetTexture( 0 );

	// -- Refresh the buffer!
	oglStartBuffer();

	oglAddVertex( pLeft, pTop, 0.0f, 1.0f, 0.0f, Color_ARGB( 0,0,80,128 ), 0.0f, 0.0f );
	oglAddVertex( pLeft, pTop, 0.0f, 1.0f, 0.0f, Color_ARGB( 0,0,80,128 ), 0.0f, 0.0f );
	oglAddVertex( pLeft, pTop, 0.0f, 1.0f, 0.0f, Color_ARGB( 0,0,80,128 ), 0.0f, 0.0f );
	oglAddVertex( pLeft, pTop, 0.0f, 1.0f, 0.0f, Color_ARGB( 0,0,80,128 ), 0.0f, 0.0f );

	glDrawArrays( GL_QUADS, 0, pGLVerticesCount );

	// -- Stat tracking and memory zeroing
	DrawCalls++;
	dFacesCount += pGLVerticesCount / 2;
	lpVertex = 0;
	pGLVerticesCount = 0;
}

void oglSetRenderState( GLenum State, GLuint Value )
{
	// -- Direct3D 8/9 Style SetRenderState method.

	if ( State==GL_ALPHA_TEST ||
		 State==GL_BLEND ||
		 State==GL_CULL_FACE ||
		 State==GL_FOG ||
		 State==GL_DEPTH_TEST ||
		 State==GL_TEXTURE_2D ||
		 State==GL_COLOR_MATERIAL ||
		 State==GL_MULTISAMPLE )
	{
		if ( g_RenderState[State] == Value ) return;
		g_RenderState[State] = Value;

		if ( Value == (GLuint)true )	glEnable( State );
		else							glDisable( State );
		return;
	}

	if ( State==GL_BLEND_SRC )
	{
		gStateGLBlendSrc = Value;
		glBlendFunc( Value, gStateGLBlendDst );
	}

	if ( State==GL_BLEND_DST )
	{
		gStateGLBlendDst = Value;
		glBlendFunc( gStateGLBlendSrc, Value );
	}

	if ( State==GL_CULL_FACE_MODE ) glCullFace( Value );

	if ( State==GL_DEPTH_FUNC ) glDepthFunc( Value );

	if ( State==GL_TEXTURE_MIN_FILTER ) CurrentMinFilter = Value;
	if ( State==GL_TEXTURE_MAG_FILTER ) CurrentMagFilter = Value;

	if ( GLEW_EXT_texture_filter_anisotropic )
	if ( State==GL_TEXTURE_MAX_ANISOTROPY_EXT ) CurrentAnisotropy = (float)Value;
}

void oglStartBufferBMP()
{
	// -> Initialize/Re-Initialize the Vertex Buffer
	if (lpVertex) return;
	if (pGLVerticesCount > 0 ) return;

	if ( gStateGLFogColor != CurFogColor )
	{
		float fogColor[] = {
			((CurFogColor>>0 ) & 255)/255.0f,
			((CurFogColor>>8 ) & 255)/255.0f,
			((CurFogColor>>16) & 255)/255.0f,
			1
		};
		gStateGLFogColor = CurFogColor;
		glFogfv(GL_FOG_COLOR, fogColor);
	}

	oglSetRenderState( GL_ALPHA_TEST, g->ALPHAKEY );

	g_VertBufferBMP = true;

	lpVertex = pGLVertices;
	pGLVerticesCount = 0;
}

void oglStartBuffer()
{
	// -> Initialize/Re-Initialize the Vertex Buffer
	if (lpVertex) return;
	if (pGLVerticesCount > 0 ) return;

	if ( gStateGLFogColor != CurFogColor )
	{
		float fogColor[] = {
			((CurFogColor>>0 ) & 255)/255.0f,
			((CurFogColor>>8 ) & 255)/255.0f,
			((CurFogColor>>16) & 255)/255.0f,
			1
		};
		gStateGLFogColor = CurFogColor;
		glFogfv(GL_FOG_COLOR, fogColor);
	}

	g_VertBufferBMP = false;

	lpVertex = pGLVertices;
	pGLVerticesCount = 0;
}

void oglEndBuffer( )
{
	if (!lpVertex) return;
	if (pGLVerticesCount == 0) return;

	glDrawArrays( GL_TRIANGLES, 0, pGLVerticesCount );
	DrawCalls++;
	dFacesCount += pGLVerticesCount / 3;

	lpVertex = 0;
	pGLVerticesCount = 0;
}

void oglFlushBuffer( int fproc1, int fproc2 )
{
	// -> Flush the vertex array
	// -- Draw the 'normal' triangles
	glDrawArrays( GL_TRIANGLES, 0, fproc1*3 );
	DrawCalls++;

	// -- Draw the 'special' triangles
	if ( fproc2 > 0 )
	{
		LastMinFilter = CurrentMinFilter;
		LastMagFilter = CurrentMagFilter;

		oglSetRenderState( GL_ALPHA_TEST, true );//g->ALPHAKEY );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		oglSetRenderState( GL_CULL_FACE, false );

		glDrawArrays( GL_TRIANGLES, fproc1*3, fproc2*3 );
		DrawCalls++;

		oglSetRenderState( GL_CULL_FACE, true );
		oglSetRenderState( GL_ALPHA_TEST, false );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, LastMagFilter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, LastMinFilter );
	}

	oglSetRenderState( GL_CULL_FACE, true );

	lpVertex = 0;
	pGLVerticesCount = 0;

	dFacesCount += fproc1+fproc2;
}


void oglDrawBox(int x1, int y1, int x2, int y2, uint32_t color)
{
	// -> Draw a filled rectangle

	glBegin(GL_QUADS);

	glColor4ubv((GLubyte*)&color);

	glVertex4f((float)x1,(float)y1, 0, 1);
	glVertex4f((float)x2+1,(float)y1, 0, 1);
	glVertex4f((float)x2+1,(float)y2+1, 0, 1);
	glVertex4f((float)x1,(float)y2+1, 0, 1);

	glEnd();
	DrawCalls++;
}

void oglDrawLine(float x1,float y1,float x2,float y2,uint32_t color)
{
	// -> Draws a Line

	glBegin(GL_LINES);

	glColor4ubv((GLubyte*)&color);
	glVertex4f(x1,y1,1,1);

	glColor4ubv((GLubyte*)&color);
	glVertex4f(x2,y2,1,1);

	glEnd();
	DrawCalls++;
}

void oglDrawCircle(int x,int y,int r, uint32_t color)
{
	// -> Draws the outline of a circle

	glBegin(GL_LINE_LOOP);

	glColor4ubv((GLubyte*)&color);

	for (int i=0; i<16; i++)
	{
		float p = float(i/16.0f);
		float rx = sinf( ((360*p)*3.14f/180) ) *cosf(0)* (float)r;
		float ry = cosf( ((360*p)*3.14f/180) ) *cosf(0)* (float)r;
		glVertex4f((float)x+rx,(float)y+ry,0.f,1.f);
	}

	glEnd();
	DrawCalls++;
}

bool oglIsTextureBound( GLuint p_texture )
{
	if ( gStateGLTextureBind == p_texture )
		return true;
	else
		return false;
}

void oglSetTexture( GLuint Texture )
{
	if ( Texture == gStateGLTextureBind ) return;

	VideoOptions* VidOpt = GlobalVideoOptions::SharedVariable();

	/*
	Bind the texture
	*/
	glBindTexture( GL_TEXTURE_2D, Texture );
	gStateGLTextureBind = Texture;

	/*
	Set the environment mode to MODULATE, this multiplies the source and destination values.
	*/
	//glTexEnvi( GL_TEXTURE_2D, GL_CTexture_ENV_MODE, GL_MODULATE );

	/*
	Set the filter mode to BILINEAR magnification and TRILINEAR minification
	*/
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, CurrentMagFilter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, CurrentMinFilter );
	if ( CurrentMinFilter == GL_NEAREST || CurrentMinFilter == GL_LINEAR )
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
	else
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, VidOpt->mAnisotropyLevels );

	/*
	Set the wrap mode for 2D co-ordiantes to CLAMP
	*/
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	TextureBinds++;
}

void oglTextOut(int x,int y, char *text,uint32_t color)
{
	// -> Outputs text to the OpenGL Scene

	// Determine if the function should continue
	//if ( glMajor >= 3 ) return;
	if (text == NULL) return;
    if (pFont == 0) { return; DoHalt("oglTextOut()->pFont invalid!"); }

	int Alpha = ((color>>24) & 255) - 15;
	if ( Alpha < 0 ) Alpha = 0;

	// Set OpenGL states
	oglSetRenderState( GL_DEPTH_TEST, false );
	oglSetRenderState( GL_FOG, GL_FALSE );
	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_TEXTURE_2D, GL_FALSE );

	if ( CurrentTexture != 0 )
	{
		LastTexture = CurrentTexture;
		CurrentTexture = 0;
		//oglSetTexture( 0 );
	}

	if ( GLEW_EXT_fog_coord ) glFogCoordf( 0.0f );

	// Gather some information
	vec2i sz;
	sz.y = GetTextH( 0, text );

	//Shadow
	glColor4ub(0,0,0,Alpha);

    glPushAttrib(GL_LIST_BIT);
    glListBase(pFont);

	glRasterPos2i( x+1, y + (sz.y) );
	glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text);

	//Actual text
    glColor4ubv((GLubyte*)&color);
    //glRasterPos3f(float(x), float(y+(10/2)+5), 0.0f);

	glRasterPos2i( x, y + (sz.y) );
	glCallLists( strlen( text ), GL_UNSIGNED_BYTE, text);

	glPopAttrib();

    //if (OptDayNight==2 && !NightVision) glEnable(GL_FOG);

	oglSetRenderState( GL_TEXTURE_2D, GL_TRUE );
	oglSetRenderState( GL_BLEND, false );
	oglSetRenderState( GL_DEPTH_TEST, true );
}

int glSkyR=0,glSkyG=0,glSkyB=0;

void oglClearBuffers(void)
{
	if ( glSkyR != SkyR || glSkyG != SkyG || glSkyB != SkyB )
	{
		glSkyR = SkyR; glSkyG = SkyG; glSkyB = SkyB;
		glClearColor(SkyR/255.0f, SkyG/255.0f, SkyB/255.0f, 1.0f);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,WinW,WinH,0, -1.0f, 1.0f );

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,WinW,WinH);

	/*if (OptDayNight==2 && !NightVision)
	{
	    //glDepthRange(0.0f,1.0f);
	    float fogColor[] = {0,0,0,1};
	    glFogi(GL_FOG_MODE, GL_LINEAR);
	    glFogfv(GL_FOG_COLOR,fogColor);
	    //glFogf(GL_FOG_DENSITY, -0.35f);
	    glFogf(GL_FOG_START, 0.015f);
        glFogf(GL_FOG_END, 0.f);
        glEnable(GL_FOG);
	}*/
}


int oglCreateSprite(bool Alpha, TPicture &pic)
{
	uint8_t ByteDepth = pic.m_bpp / 8;
	void* Raster = 0;

	//Process Alpha g->TRANSPARENCY
	if ( pic.m_bpp == 16 && Alpha )
	for(int x=0; x<pic.m_width*pic.m_height; x++)
	{
		uint16_t w = *((uint16_t*)pic.m_data + x);
		if ( w & 0x7FFF )	*((uint16_t*)pic.m_data + x) |= (1<<15);
		else				*((uint16_t*)pic.m_data + x) |= (0<<15);
	}

	if ( !g_NonPow2 )
	{
		if (pic.m_width==3) pic.m_cwidth = 4;
		if (pic.m_width>4   && pic.m_width<=8   ) pic.m_cwidth = 8;
		if (pic.m_width>8   && pic.m_width<=16  ) pic.m_cwidth = 16;
		if (pic.m_width>16  && pic.m_width<=32  ) pic.m_cwidth = 32;
		if (pic.m_width>32  && pic.m_width<=64  ) pic.m_cwidth = 64;
		if (pic.m_width>64  && pic.m_width<=128 ) pic.m_cwidth = 128;
		if (pic.m_width>128 && pic.m_width<=256 ) pic.m_cwidth = 256;
		if (pic.m_width>256 && pic.m_width<=512 ) pic.m_cwidth = 512;
		if (pic.m_width>512 && pic.m_width<=1024) pic.m_cwidth = 1024;
		if (pic.m_height==3) pic.m_cheight = 4;
		if (pic.m_height>4  && pic.m_height<=8 ) pic.m_cheight = 8;
		if (pic.m_height>8  && pic.m_height<=16) pic.m_cheight = 16;
		if (pic.m_height>16 && pic.m_height<=32) pic.m_cheight = 32;
		if (pic.m_height>32 && pic.m_height<=64) pic.m_cheight = 64;
		if (pic.m_height>64 && pic.m_height<=128) pic.m_cheight = 128;
		if (pic.m_height>128 && pic.m_height<=256) pic.m_cheight = 256;
		if (pic.m_height>256 && pic.m_height<=512) pic.m_cheight = 512;
		if (pic.m_height>512 && pic.m_height<=1024) pic.m_cheight = 1024;

		if (pic.m_cwidth>pic.m_cheight) pic.m_cheight = pic.m_cwidth;
		if (pic.m_cheight>pic.m_cwidth) pic.m_cwidth = pic.m_cheight;

		if (pic.m_width!=pic.m_cwidth || pic.m_height!=pic.m_cheight)
		{
			if ( pic.m_bpp == 16 )
			{
				Raster = malloc( sizeof(uint16_t) * (pic.m_cwidth * pic.m_cheight) );
				for (unsigned int y=0; y<(unsigned int)pic.m_height; y++)
				{
					unsigned int pos1 = 0+y*pic.m_width;
					unsigned int pos2 = 0+y*pic.m_cwidth;

					memcpy(((uint16_t*)Raster + pos2), ((uint16_t*)pic.m_data + pos1), pic.m_width * ByteDepth);
				}
			}
			else if ( pic.m_bpp == 24 )
			{
				Raster = malloc( ByteDepth * (pic.m_cwidth * pic.m_cheight) );
				for (unsigned int y=0; y<(unsigned int)pic.m_height; y++)
				{
					unsigned int pos1 = 0+y*pic.m_width;
					unsigned int pos2 = 0+y*pic.m_cwidth;

					memcpy(((rgb_t*)Raster + pos2), ((rgb_t*)pic.m_data + pos1), pic.m_width * ByteDepth);
				}
			}
			else if ( pic.m_bpp == 32 )
			{
				Raster = malloc( sizeof(uint32_t) * (pic.m_cwidth * pic.m_cheight) );
				for (unsigned int y=0; y<(unsigned int)pic.m_height; y++)
				{
					unsigned int pos1 = 0+y*pic.m_width;
					unsigned int pos2 = 0+y*pic.m_cwidth;

					memcpy(((uint32_t*)Raster + pos2), ((uint32_t*)pic.m_data + pos1), pic.m_width * ByteDepth);
				}
			}
		}
		else
		Raster = pic.m_data;

		//fprintf( stderr, "oglCreateSprite :: Power of 2 sprite\n" );
	}
	else
	{
		Raster = pic.m_data;
		pic.m_cwidth = pic.m_width;
		pic.m_cheight = pic.m_cheight;
	}

	//Bind the texture and apply it's parameters
	gStateGLTextureBind = pic.m_texid;
	glBindTexture(GL_TEXTURE_2D, pic.m_texid );
	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE );

	//Transfer the texture from CPU memory to GPU (High Performance) memory
	if ( pic.m_bpp == 16 ) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, pic.m_cwidth,pic.m_cheight, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, Raster);
	if ( pic.m_bpp == 24 ) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, pic.m_cwidth,pic.m_cheight, 0, GL_BGR, GL_UNSIGNED_BYTE, Raster);
	if ( pic.m_bpp == 32 ) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pic.m_cwidth,pic.m_cheight, 0, GL_BGRA, GL_UNSIGNED_BYTE, Raster);

	//if (pic.m_width!=pic.m_cwidth || pic.m_height!=pic.m_cheight) free( Raster ); // what the fuck was I thinking? lol
	if ( Raster != pic.m_data ) free(Raster);

	return pic.m_texid;
}


void funcTextureColorKey( int w, int h, uint16_t* tptr, uint16_t color )
{
	for(int x=0; x<w*h; x++)
	{
		if ( color == 0xFFFF )
		{
			*(tptr + x) |= 0x8000;
			continue;
		}

		uint16_t c = *(tptr + x);

		if ( c != 0x0000 )
		{
			*(tptr + x) |= 0x8000;
		}
	}
}


bool oglCreateTexture(bool alpha, bool mipmaps, CTexture* tptr )
{
	if ( tptr->m_texid == 0 ) return false;

	funcTextureColorKey( tptr->m_width, tptr->m_height, (uint16_t*)tptr->m_data, (alpha)?0x00:0xFFFF );

	gStateGLTextureBind = tptr->m_texid;
	glBindTexture(		GL_TEXTURE_2D, tptr->m_texid );
	glTexParameteri(	GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 8 ); // we only want 2 mips, 256x256, 128x128, 64x64
	glTexParameteri(	GL_TEXTURE_2D, GL_GENERATE_MIPMAP, mipmaps ? GL_TRUE : GL_FALSE );

	switch ( tptr->m_bpp )
	{
	case 16:
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB5_A1, tptr->m_width, tptr->m_height, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, tptr->m_data );
		break;
	case 24:
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, tptr->m_width, tptr->m_height, 0, GL_BGR, GL_UNSIGNED_BYTE, tptr->m_data );
		break;
	case 32:
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, tptr->m_width, tptr->m_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, tptr->m_data );
		break;
	default:
		glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, tptr->m_width, tptr->m_height, 0, GL_RED, GL_UNSIGNED_BYTE, tptr->m_data );
		break;
	}

	return true;
}
