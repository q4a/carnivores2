#define INITGUID
#include "Hunt.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct _TMenuSet {
	int x0, y0;
	int Count;
	char Item[32][32];
} TMenuSet;


TMenuSet Options[3];
char HMLtxt[3][12];
char CKtxt[2][16];
char Restxt[8][24];
char Textxt[3][12];
char Ontxt[2][12];
char Systxt[2][12];
int CurPlayer = -1;
int MaxDino, AreaMax, LoadCount;

#define REGLISTX 320
#define REGLISTY 370

bool NEWPLAYER = false;

int  MapVKKey(int k)
{
	/*if (k==VK_LBUTTON) return 124;
	if (k==VK_RBUTTON) return 125;
	return MapVirtualKey(k , 0);*/
	return 0;
}




vec2i p;
int OptMode = 0;
int OptLine = 0;


void wait_mouse_release()
{
    //while (GetAsyncKeyState(VK_RBUTTON) & 0x80000000);
	//while (GetAsyncKeyState(VK_LBUTTON) & 0x80000000);

}


int GetTextW(HDC hdc, const char* s)
{
	#if defined( AF_PLATFORM_LINUX )
	vec2i sz;
	sz.x = 0;
	sz.y = 10;

	for ( int c = 0; c<strlen(s); ++c )
	{
		XCharStruct* ci = ( (XCharStruct*)g_SmlFontInfo->per_char + c );
		sz.x += ci->width;
	}

	return sz.x;

	#elif defined( AF_PLATFORM_WINDOWS )
	SIZE sz;
	sz.cx = sz.cy = 0;

	GetTextExtentPointA( hdcMain, s, strlen(s), &sz);

	return sz.cx;
	#endif
}

int GetTextH(HDC hdc, const char* s)
{
	#if defined( AF_PLATFORM_LINUX )
	vec2i sz;
	sz.x = 0;
	sz.y = 0;

	for ( int c = 0; c<strlen(s); ++c )
	{
		if ( s[c] == '\n' )
		sz.y += g_SmlFontInfo->max_bounds.ascent;
	}
	sz.y += g_SmlFontInfo->max_bounds.ascent;

	#elif defined( AF_PLATFORM_WINDOWS )
	SIZE sz;

	GetTextExtentPoint( hdcMain, "A", 1, &sz);

	return sz.cy;
	#endif
}

void PrintText(char* s, int x, int y, int rgb)
{
    /*HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcCMain,hbmpVideoBuf);
    SetBkMode(hdcCMain, TRANSPARENT);

    SetTextColor(hdcCMain, 0x00000000);
    TextOut(hdcCMain, x+1, y+1, s, strlen(s));
    SetTextColor(hdcCMain, rgb);
    TextOut(hdcCMain, x, y, s, strlen(s));

    SelectObject(hdcCMain,hbmpOld);*/
}


void DoHalt(const char* Mess)
{
	AudioStop();

	if ( Mess != 0 )
	{
		PrintLog("ABNORMAL_HALT: ");
		PrintLog(Mess);
		PrintLog("\n");
		ShutDown3DHardware();
		g_AudioDevice->shutdown();
		delete g_AudioDevice;
		ShutdownNetwork();
#if defined( AF_PLATFORM_WINDOWS )
		MessageBox(NULL, Mess, "Carnivores Termination", MB_OK | MB_SYSTEMMODAL | MB_ICONEXCLAMATION);
#endif
		CloseLog();
		glfwEnable( GLFW_MOUSE_CURSOR );
		glfwCloseWindow();
		assert( false );
	}
	else
	{
		QUITMODE=1;
	}

}


void WaitRetrace()
{
    bool bv = false;
}



void Wait(int time)
{
    unsigned int t = (glfwGetTime()/1000.0) + time;
    while (t>(glfwGetTime()/1000.0)) ;
}


#include "gl_extra.h"
char loadtxt[128];
TPicture LoadWall;

void UpdateLoadingWindow()
{
    glViewport( 0,0, LoadWall.m_width, LoadWall.m_height );

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, LoadWall.m_width, LoadWall.m_height, 0 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	DrawPicture( 0,0, LoadWall );

    Draw_Text( 19, LoadWall.m_height-22, loadtxt, 0xFF000000 );
    Draw_Text( 18, LoadWall.m_height-23, loadtxt, 0xFF70B0B0 );

	glfwSwapBuffers();
	//glfwSleep( 1.0/2.0 );
}

GLuint LoadingTexture;

void StartLoading()
{
	//LoadPicture(LoadWall,   "HUNTDAT/MENU/LOADBANNER.BMP");

    LoadPicture(LoadWall,   "HUNTDAT/MENU/LOADBANNER.TGA"); // 24-bit TARGA Loading Image
    //LoadPicture(LoadWall,   "HUNTDAT/MENU/LOADING.TGA");

	LoadWall.m_texid = oglCreateSprite( false, LoadWall );
	gStateGLTextureBind = 0;

	int s_w = 800, s_h = 600;

	GetScreenSize( &s_w, &s_h );

	glfwSetWindowPos( (s_w - LoadWall.m_width)/2, (s_h - LoadWall.m_height)/2 );
	glfwSetWindowSize( LoadWall.m_width, LoadWall.m_height );

}

void EndLoading()
{
    if ( lpVideoBuf ) memset(lpVideoBuf, 0, 1024*768*2 );
    LoadWall.Release();
	//_HeapFree(Heap, 0, (void*)LoadWall.lpImage);
}


void PrintLoad(const char *t)
{
	strcpy(loadtxt, t);
	LoadCount++;
	UpdateLoadingWindow();

}


void SetVideoMode(int W, int H)
{
    WinW = W;
    WinH = H;

    WinEX = WinW - 1;
    WinEY = WinH - 1;
    VideoCX = WinW / 2;
    VideoCY = WinH / 2;

    float Aspect = float(WinW)/float(WinH);
	Aspect = 1.3333f;
    CameraW = (float)(VideoCX) * 1.25f;
    CameraH = CameraW * (WinH * Aspect / WinW);

	vec2i rc;
	glfwGetWindowSize( &rc.x, &rc.y );
	WindowCX = rc.x / 2;
	WindowCY = rc.y / 2;
	ResetMousePos();

    LoDetailSky = ( WinW <= 640 );
    //SetCursor(hcArrow);
    //while (ShowCursor(false)>=0) ;
}



