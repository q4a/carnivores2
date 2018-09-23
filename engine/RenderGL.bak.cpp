#ifdef _gl

/*********************************************
* === Creation ===
* @author: James Ogden, alias: Rexhunter99
* @date: 31/July/2010
* @contact: rexhunter99@gmail.com
* === Last Edit ===
* @author: James Ogden, alias: Rexhunter99
* @date: 18/August/2010
* @time: 11:07 AM
*********************************************/


// == Includes == //
#include "Hunt.h"
#include <stdio.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glext.h>


// == Libraries == //
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glu32.lib")


// == Definitions == //
#undef  TCMAX
#undef  TCMIN
#define TCMAX		1.0f
#define TCMIN		0.0f
#define _ZSCALE		16.f
#define _AZSCALE	-(1.f /  16.f);


// == Types & Structures == //
typedef struct _TagGLVertex {
	float x,y,z,rhw;
	DWORD color;
	float tu,tv;
} GLVertex;


// == Global Variables == //
int			MyPrevHealth = MAX_HEALTH;
float		SkyTrans = 0.0f;
bool		NEEDWATER; // Does the player see water?
bool		VBENABLE;
Vector2di	ORList[2048]; // List of objects that the player can see
int			ORLCount = 0;
PIXELFORMATDESCRIPTOR pfd;
int			iFormat;
int			zs =0;
HDC			DeviceContext;
HGLRC		RenderContext;
GLVertex    gvtx[16],gVtxBuf[1024],*pVtxBuf = gVtxBuf;
GLuint		pTextures[2048];
int			glTextureCount;
int			iVtxCount = 0;
bool		bBufNeedOp = false;
GLuint      pFont;

float	SunLight;
float	TraceK,SkyTraceK,FogYGrad,FogYBase;;
int		SunScrX, SunScrY;
int		SkySumR, SkySumG, SkySumB;
int		LowHardMemory;
int		lsw;
int		vFogT[1024];
char	gLog[512];


// == Function Declarations == //
void ClipVector(CLIPPLANE& C, int vn);
int BuildTreeClipNoSort();
void BuildTreeNoSortf();
void RenderObject(int x, int y);
float GetSkyK(int x, int y);
float GetTraceK(int x, int y);


//=========================================================================//
// Internal Source (RenderGL.cpp)
//=========================================================================//
void GLFontCreate()
{
    HFONT   font;
    HFONT   oldfont;

	int H = 10;
	//if (WinH>=600) H = 12;
	//if (WinH>=768) H = 14;
	//if (WinH>=1024) H = 16;

    pFont = glGenLists(255);

    font = CreateFont(  -H,0,
                        0,0,
                        FW_NORMAL,//FW_BOLD,
                        FALSE,FALSE,FALSE,
                        ANSI_CHARSET,//RUSSIAN_CHARSET,
                        OUT_TT_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        ANTIALIASED_QUALITY,
                        FF_DONTCARE|DEFAULT_PITCH,
                        "Verdana");

    oldfont = (HFONT)SelectObject(DeviceContext, font);

    wglUseFontBitmaps(DeviceContext,0,255, pFont);

    SelectObject(DeviceContext,oldfont);
    DeleteObject(font);
}

void GLDeleteFont()
{
	if (pFont==0) return;
	glDeleteLists(pFont,255);
	pFont = 0;
}

void GLAddToBuffer(GLVertex &vtx1, GLVertex &vtx2, GLVertex &vtx3);

DWORD DarkRGBX(DWORD c)
{
    BYTE R = ((c>>0 ) & 255) / 3;
    BYTE G = ((c>>8 ) & 255) / 3;
    BYTE B = ((c>>16) & 255) / 3;
    return (R) + (G<<8) + (B<<16);
}

void GLDrawTriangle(GLVertex &vtx1, GLVertex &vtx2, GLVertex &vtx3)
{
	// -> Draws a Triangle or adds it to the buffer

    if (!NightVision && OptDayNight==2)
    {
        vtx1.color = (((vtx1.color>>24) & 255)<<24) | DarkRGBX(vtx1.color);
        vtx2.color = (((vtx2.color>>24) & 255)<<24) | DarkRGBX(vtx2.color);
        vtx3.color = (((vtx3.color>>24) & 255)<<24) | DarkRGBX(vtx3.color);
    }

	if (VBENABLE)
	{
		if (glIsEnabled(GL_ALPHA_TEST))
		bBufNeedOp = true;
		GLAddToBuffer(vtx1,vtx2,vtx3);
		return;
	}

	glBegin(GL_TRIANGLES);

	glColor4ubv((GLubyte*)&vtx1.color);
	glTexCoord2f(vtx1.tu,vtx1.tv);
	glVertex4f(vtx1.x,vtx1.y,vtx1.z,vtx1.rhw);

	glColor4ubv((GLubyte*)&vtx2.color);
	glTexCoord2f(vtx2.tu,vtx2.tv);
	glVertex4f(vtx2.x,vtx2.y,vtx2.z,vtx2.rhw);

	glColor4ubv((GLubyte*)&vtx3.color);
	glTexCoord2f(vtx3.tu,vtx3.tv);
	glVertex4f(vtx3.x,vtx3.y,vtx3.z,vtx3.rhw);

	glEnd();

	dFacesCount++;
}

void GLStartBuffer()
{
	// -> Initialize/Re-Initialize the Vertex Buffer
	pVtxBuf = gVtxBuf;
	VBENABLE = true;
	iVtxCount = 0;
	bBufNeedOp = false;
}

void GLAddToBuffer(GLVertex &vtx1, GLVertex &vtx2, GLVertex &vtx3)
{
	if (!VBENABLE) return;

	*pVtxBuf = vtx1;
	pVtxBuf++;
	*pVtxBuf = vtx2;
	pVtxBuf++;
	*pVtxBuf = vtx3;
	pVtxBuf++;

	iVtxCount+=3;
}

void GLEndBuffer()
{
	// -> Flush the Vertex Buffer to the screen

	int Prims = iVtxCount;
	VBENABLE = false;

	if (bBufNeedOp) glEnable(GL_ALPHA_TEST);

	glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(4, GL_FLOAT, sizeof(GLVertex), &gVtxBuf[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(GLVertex), &gVtxBuf[0].color);
    glTexCoordPointer(2, GL_FLOAT, sizeof(GLVertex), &gVtxBuf[0].tu);

    glDrawArrays(GL_TRIANGLES, 0, Prims);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

	if (bBufNeedOp) glDisable(GL_ALPHA_TEST);

	dFacesCount += Prims;
}

void GLDrawRectangle(int x1, int y1, int x2, int y2, DWORD color)
{
	// -> Draw a filled rectangle
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_QUADS);

	glColor4ubv((GLubyte*)&color);

	glVertex4f((float)x1,(float)y1, 1, 1);
	glVertex4f((float)x2+1,(float)y1, 1, 1);
	glVertex4f((float)x2+1,(float)y2+1, 1, 1);
	glVertex4f((float)x1,(float)y2+1, 1, 1);

	glEnd();
}

void GLDrawLine(GLVertex& vtx1, GLVertex& vtx2)
{
	// -> Draws a Line
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_LINES);

	glColor4ubv((GLubyte*)&vtx1.color);
	glVertex4f(vtx1.x,vtx1.y,vtx1.z,vtx1.rhw);

	glColor4ubv((GLubyte*)&vtx2.color);
	glVertex4f(vtx2.x,vtx2.y,vtx2.z,vtx2.rhw);

	glEnd();
}

void GLDrawCircle(int x,int y,int r, DWORD color)
{
	// -> Draws the outline of a circle
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_LINE_LOOP);
	float xx = float(x);
	float yy = float(y);

	glColor4ubv((GLubyte*)&color);

	for (int i=0; i<16; i++)
	{
		float p = float(i/16.0f);
		float rx = sinf( ((360*p)*3.14f/180) ) *cosf(0)* (float)r;
		float ry = cosf( ((360*p)*3.14f/180) ) *cosf(0)* (float)r;
		glVertex4f((float)x+rx,(float)y+ry,1.f,1.f);
	}

	glEnd();
}

void GLTextOut(int x,int y, char *text,DWORD color)
{
	// -> Outputs text to the OpenGL Scene
	if (text == NULL) return;
    if (text == "") return;

    glBindTexture(GL_TEXTURE_2D, 0);

	//Shadow
	glColor4ub(0,0,0,0xF0);
    glRasterPos3f(float(x+1), float(y+(10/2)+5+1), 0.9999f);

    glPushAttrib(GL_LIST_BIT);
    glListBase(pFont);
    glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();

	//Actual text
    glColor4ubv((GLubyte*)&color);
    glRasterPos3f(float(x), float(y+(10/2)+5), 1.0f);

    glPushAttrib(GL_LIST_BIT);
    glListBase(pFont);
    glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void GLClearBuffers(void)
{
	glClearColor(SkyR/255.0f, SkyG/255.0f, SkyB/255.0f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,WinW,WinH,0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,WinW,WinH);

	if (OptDayNight==2 && !NightVision)
	{
	    glDepthRange(0.0f,1.0f);
	    float fogColor[] = {0,0,0,1};
	    glFogi(GL_FOG_MODE, GL_LINEAR);
	    glFogfv(GL_FOG_COLOR,fogColor);
	    //glFogf(GL_FOG_DENSITY, -0.35f);
	    glFogf(GL_FOG_START, 0.015f);
        glFogf(GL_FOG_END, 0.f);
        glEnable(GL_FOG);
	}
}


int GLCreateSprite(bool Alpha, TPicture &pic)
{
	//Process Alpha Transparency
	if (Alpha)
	for(int x=0; x<pic.W*pic.H; x++)
	{
		WORD w = *(pic.lpImage + x);
		int R = ((w>>10) & 31);
		int G = ((w>>5 ) & 31);
		int B = ((w>>0 ) & 31);
		int A = 1;
		if (Alpha && R<1 && G<1 && B<1) A = 0;
		*(pic.lpImage + x) = (B) + (G<<5) + (R<<10) + (A<<15);
	}

	if (pic.W==3) pic.cW = 4;
    if (pic.W>4  && pic.W<=8 ) pic.cW = 8;
    if (pic.W>8  && pic.W<=16) pic.cW = 16;
    if (pic.W>16 && pic.W<=32) pic.cW = 32;
    if (pic.W>32 && pic.W<=64) pic.cW = 64;
    if (pic.W>64 && pic.W<=128) pic.cW = 128;
    if (pic.W>128 && pic.W<=256) pic.cW = 256;
    if (pic.W>256 && pic.W<=512) pic.cW = 512;
    if (pic.W>512 && pic.W<=1024) pic.cW = 1024;
    if (pic.H==3) pic.cH = 4;
    if (pic.H>4  && pic.H<=8 ) pic.cH = 8;
    if (pic.H>8  && pic.H<=16) pic.cH = 16;
    if (pic.H>16 && pic.H<=32) pic.cH = 32;
    if (pic.H>32 && pic.H<=64) pic.cH = 64;
    if (pic.H>64 && pic.H<=128) pic.cH = 128;
    if (pic.H>128 && pic.H<=256) pic.cH = 256;
    if (pic.H>256 && pic.H<=512) pic.cH = 512;
    if (pic.H>512 && pic.H<=1024) pic.cH = 1024;

	if (pic.cW>pic.cH) pic.cH = pic.cW;
	if (pic.cH>pic.cW) pic.cW = pic.cH;

	sprintf(gLog,"  WxH: %dx%d\n  CWxCH: %dx%d\n",pic.W,pic.H,pic.cW,pic.cH);
	PrintLog(gLog);

	WORD *Raster;
    if (pic.W!=pic.cW || pic.H!=pic.cH)
    {
        Raster = new WORD[pic.cW*pic.cH];

        for (unsigned int y=0; y<(unsigned int)pic.H; y++)
        {
            unsigned int pos1 = 0+y*pic.W;
            unsigned int pos2 = 0+y*pic.cW;

            memcpy((Raster + pos2), (pic.lpImage + pos1), pic.W*2);
        }
    }
    else
    Raster = pic.lpImage;

	//Bind the texture and apply it's parameters
	glBindTexture(GL_TEXTURE_2D, pTextures[glTextureCount]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Transfer the texture from CPU memory to GPU (High Performance) memory
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5_A1,pic.cW,pic.cH,0,GL_BGRA,GL_UNSIGNED_SHORT_1_5_5_5_REV,Raster);

	if (pic.W!=pic.cW || pic.H!=pic.cH) delete [] Raster;

	//Make sure the texture was created
	if (!glIsTexture(pTextures[glTextureCount])) return 0;

	sprintf(gLog,"  ID: %d\n",glTextureCount);
	PrintLog(gLog);

	//Up the counter and return the index we used
	glTextureCount++;
	return glTextureCount-1;
}

int GLCreateTexture(bool alpha, int w, int h, WORD* tptr)
{
	for(int x=0; x<w*h; x++)
	{
		WORD w = *(tptr + x);
		int R = ((w>>10) & 31);
		int G = ((w>>5 ) & 31);
		int B = ((w>>0 ) & 31);
		int A = 1;
		if (alpha && R<1 && G<1 && B<1) A = 0;
		*(tptr + x) = (B) + (G<<5) + (R<<10) + (A<<15);
	}

	glBindTexture(GL_TEXTURE_2D, pTextures[glTextureCount]);

	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5_A1,w,h,0,GL_BGRA,GL_UNSIGNED_SHORT_1_5_5_5_REV,tptr);

	WORD *PrevMipMap,*MipMap;
	PrevMipMap = new WORD[w*w];
	MipMap = new WORD[w*w];
	int MipW = w/2;

	GenerateMipMap((WORD*)tptr, PrevMipMap, MipW);

	for(int x=0; x<MipW*MipW; x++)
	{
		WORD w = PrevMipMap[x];
		int R = ((w>>10) & 31);
		int G = ((w>>5 ) & 31);
		int B = ((w>>0 ) & 31);
		int A = 1;
		if (alpha && R<1 && G<1 && B<1) A = 0;
		PrevMipMap[x] = (B) + (G<<5) + (R<<10) + (A<<15);
	}

	glTexImage2D(GL_TEXTURE_2D,1,GL_RGB5_A1,MipW,MipW,0,GL_BGRA,GL_UNSIGNED_SHORT_1_5_5_5_REV, PrevMipMap);

	for (int i=1; i<32; i++)
	{
		MipW /= 2;	//Half the resolution

		GenerateMipMap(PrevMipMap, MipMap, MipW);

		for(int x=0; x<MipW*MipW; x++)
		{
			WORD w = MipMap[x];
			int R = ((w>>10) & 31);
			int G = ((w>>5 ) & 31);
			int B = ((w>>0 ) & 31);
			int A = 1;
			if (alpha && R<1 && G<1 && B<1) A = 0;
			MipMap[x] = (B) + (G<<5) + (R<<10) + (A<<15);
		}

		glTexImage2D(GL_TEXTURE_2D,i+1,GL_RGB5_A1,MipW,MipW,0,GL_BGRA,GL_UNSIGNED_SHORT_1_5_5_5_REV, MipMap);

		memcpy(PrevMipMap,MipMap,MipW*MipW*2);

		if (MipW<2) break;
	}
	delete [] PrevMipMap;
	delete [] MipMap;

	if (!glIsTexture(pTextures[glTextureCount])) return -1;

	glTextureCount ++;
	return glTextureCount-1;
}


void ClipVector(CLIPPLANE& C, int vn)
{
  int ClipRes = 0;
  float s,s1,s2;
  int vleft  = (vn-1); if (vleft <0) vleft=vused-1;
  int vright = (vn+1); if (vright>=vused) vright=0;

  MulVectorsScal(cp[vn].ev.v, C.nv, s); /*s=SGN(s-0.01f);*/
  if (s>=0) return;

  MulVectorsScal(cp[vleft ].ev.v, C.nv, s1); /* s1=SGN(s1+0.01f); */ //s1+=0.0001f;
  MulVectorsScal(cp[vright].ev.v, C.nv, s2); /* s2=SGN(s2+0.01f); */ //s2+=0.0001f;

  if (s1>0) {
   ClipRes+=1;
   /*
   CalcHitPoint(C,cp[vn].ev.v,
                  cp[vleft].ev.v, hleft.ev.v);

   float ll = VectorLength(SubVectors(cp[vleft].ev.v, cp[vn].ev.v));
   float lc = VectorLength(SubVectors(hleft.ev.v, cp[vn].ev.v));
   lc = lc / ll;
   */
   float lc = -s / (s1-s);
   hleft.ev.v.x = cp[vn].ev.v.x + ((cp[vleft].ev.v.x - cp[vn].ev.v.x) * lc);
   hleft.ev.v.y = cp[vn].ev.v.y + ((cp[vleft].ev.v.y - cp[vn].ev.v.y) * lc);
   hleft.ev.v.z = cp[vn].ev.v.z + ((cp[vleft].ev.v.z - cp[vn].ev.v.z) * lc);

   hleft.tx = cp[vn].tx + ((cp[vleft].tx - cp[vn].tx) * lc);
   hleft.ty = cp[vn].ty + ((cp[vleft].ty - cp[vn].ty) * lc);
   hleft.ev.Light = cp[vn].ev.Light + (int)((cp[vleft].ev.Light - cp[vn].ev.Light) * lc);
   hleft.ev.ALPHA = cp[vn].ev.ALPHA + (int)((cp[vleft].ev.ALPHA - cp[vn].ev.ALPHA) * lc);
   hleft.ev.Fog   = cp[vn].ev.Fog   +      ((cp[vleft].ev.Fog   - cp[vn].ev.Fog  ) * lc);
  }

  if (s2>0) {
   ClipRes+=2;
   /*
   CalcHitPoint(C,cp[vn].ev.v,
                  cp[vright].ev.v, hright.ev.v);

   float ll = VectorLength(SubVectors(cp[vright].ev.v, cp[vn].ev.v));
   float lc = VectorLength(SubVectors(hright.ev.v, cp[vn].ev.v));
   lc = lc / ll;*/
   float lc = -s / (s2-s);
   hright.ev.v.x = cp[vn].ev.v.x + ((cp[vright].ev.v.x - cp[vn].ev.v.x) * lc);
   hright.ev.v.y = cp[vn].ev.v.y + ((cp[vright].ev.v.y - cp[vn].ev.v.y) * lc);
   hright.ev.v.z = cp[vn].ev.v.z + ((cp[vright].ev.v.z - cp[vn].ev.v.z) * lc);

   hright.tx = cp[vn].tx + ((cp[vright].tx - cp[vn].tx) * lc);
   hright.ty = cp[vn].ty + ((cp[vright].ty - cp[vn].ty) * lc);
   hright.ev.Light = cp[vn].ev.Light + (int)((cp[vright].ev.Light - cp[vn].ev.Light) * lc);
   hright.ev.ALPHA = cp[vn].ev.ALPHA + (int)((cp[vright].ev.ALPHA - cp[vn].ev.ALPHA) * lc);
   hright.ev.Fog   = cp[vn].ev.Fog   +      ((cp[vright].ev.Fog   - cp[vn].ev.Fog  ) * lc);
  }

  if (ClipRes == 0) {
      u--; vused--;
      cp[vn] = cp[vn+1];
      cp[vn+1] = cp[vn+2];
      cp[vn+2] = cp[vn+3];
      cp[vn+3] = cp[vn+4];
      cp[vn+4] = cp[vn+5];
      cp[vn+5] = cp[vn+6];
      //memcpy(&cp[vn], &cp[vn+1], (15-vn)*sizeof(ClipPoint));
  }
  if (ClipRes == 1) {cp[vn] = hleft; }
  if (ClipRes == 2) {cp[vn] = hright;}
  if (ClipRes == 3) {
    u++; vused++;
    //memcpy(&cp[vn+1], &cp[vn], (15-vn)*sizeof(ClipPoint));
    cp[vn+6] = cp[vn+5];
    cp[vn+5] = cp[vn+4];
    cp[vn+4] = cp[vn+3];
    cp[vn+3] = cp[vn+2];
    cp[vn+2] = cp[vn+1];
    cp[vn+1] = cp[vn];

    cp[vn] = hleft;
    cp[vn+1] = hright;
    }
}


void _fillMappingClip(BOOL SECOND)
{
	if (ReverseOn)
    if (SECOND) {
     switch (TDirection) {
      case 0:
       cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
       cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
	   cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
       break;
      case 1:
       cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
       cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
       cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
       break;
      case 2:
       cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
       cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
       break;
      case 3:
       cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
       cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
       break;
     }
    } else {
     switch (TDirection) {
      case 0:
       cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
       cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
       cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
       break;
      case 1:
       cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
       cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
       cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
       break;
      case 2:
       cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
       cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
       cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
       break;
      case 3:
       cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
       cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
       break;
     }
    }
   else
    if (SECOND) {
     switch (TDirection) {
      case 0:
       cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
       cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
       break;
      case 1:
       cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
       cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
       cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
       break;
      case 2:
       cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
       cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
       cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
       break;
      case 3:
       cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
       cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
       break;
     }
    } else {
     switch (TDirection) {
      case 0:
       cp[0].tx = TCMIN;   cp[0].ty = TCMIN;
       cp[1].tx = TCMAX;   cp[1].ty = TCMIN;
       cp[2].tx = TCMAX;   cp[2].ty = TCMAX;
       break;
      case 1:
       cp[0].tx = TCMIN;   cp[0].ty = TCMAX;
       cp[1].tx = TCMIN;   cp[1].ty = TCMIN;
       cp[2].tx = TCMAX;   cp[2].ty = TCMIN;
       break;
      case 2:
       cp[0].tx = TCMAX;   cp[0].ty = TCMAX;
       cp[1].tx = TCMIN;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMIN;
       break;
      case 3:
       cp[0].tx = TCMAX;   cp[0].ty = TCMIN;
       cp[1].tx = TCMAX;   cp[1].ty = TCMAX;
       cp[2].tx = TCMIN;   cp[2].ty = TCMAX;
       break;
     }
    }
}

void _fillMapping(BOOL SECOND)
{
	if (ReverseOn)
    if (SECOND) {
     switch (TDirection) {
      case 0:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
       break;
      case 1:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
       break;
      case 2:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
       break;
      case 3:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
       break;
     }
    } else {
     switch (TDirection) {
      case 0:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
       break;
      case 1:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
       break;
      case 2:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
       break;
      case 3:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
       break;
     }
    }
   else
    if (SECOND) {
     switch (TDirection) {
      case 0:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
       break;
      case 1:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
       break;
      case 2:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
       break;
      case 3:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
       break;
     }
    } else {
     switch (TDirection) {
      case 0:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMAX;
       break;
      case 1:
       scrp[0].tx = TCMIN;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMIN;
       scrp[2].tx = TCMAX;   scrp[2].ty = TCMIN;
       break;
      case 2:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMAX;
       scrp[1].tx = TCMIN;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMIN;
       break;
      case 3:
       scrp[0].tx = TCMAX;   scrp[0].ty = TCMIN;
       scrp[1].tx = TCMAX;   scrp[1].ty = TCMAX;
       scrp[2].tx = TCMIN;   scrp[2].ty = TCMAX;
       break;
     }
    }
}


void DrawTPlane(BOOL SECONT)
{
	int n;

    MulVectorsVect(SubVectors(ev[1].v, ev[0].v), SubVectors(ev[2].v, ev[0].v), nv);
    if (nv.x*ev[0].v.x  +  nv.y*ev[0].v.y  +  nv.z*ev[0].v.z<0) return;

	Mask1=0x007F;
	for (n=0; n<3; n++)
	{
		if (ev[n].DFlags & 128) return;
		Mask1=Mask1 & ev[n].DFlags;
	}
	if (Mask1>0) return;

	_fillMapping(SECONT);

	int alpha1 = 255;
	int alpha2 = 255;
	int alpha3 = 255;

	if (zs > (ctViewR-8)<<8)
	{
		int zz;
		zz = (int)VectorLength(ev[0].v) - 256 * (ctViewR-4);
		if (zz > 0) alpha1 = max(0, 255 - zz / 3); else alpha1 = 255;

		zz = (int)VectorLength(ev[1].v) - 256 * (ctViewR-4);
		if (zz > 0) alpha2 = max(0, 255 - zz / 3); else alpha2 = 255;

		zz = (int)VectorLength(ev[2].v) - 256 * (ctViewR-4);
		if (zz > 0) alpha3 = max(0, 255 - zz / 3); else alpha3 = 255;
	}

	int v=0;
    gvtx[v].x       = (float)ev[0].scrx;
    gvtx[v].y       = (float)ev[0].scry;
    gvtx[v].z       = -(_ZSCALE / ev[0].v.z);
    gvtx[v].rhw      = 1;//gvtx[v].z * _AZSCALE;
    gvtx[v].color    = (int)(ev[0].Light) * 0x00010101 | alpha1<<24;
    gvtx[v].tu       = (float)(scrp[0].tx);// / (128.f*65536.f);
    gvtx[v].tv       = (float)(scrp[0].ty);// / (128.f*65536.f);
    v++;

	gvtx[v].x       = (float)ev[1].scrx;
    gvtx[v].y       = (float)ev[1].scry;
    gvtx[v].z       = -(_ZSCALE / ev[1].v.z);
    gvtx[v].rhw      = 1;//gvtx[v].z * _AZSCALE;
    gvtx[v].color    = (int)(ev[1].Light) * 0x00010101 | alpha2<<24;
    gvtx[v].tu       = (float)(scrp[1].tx);// / (128.f*65536.f);
    gvtx[v].tv       = (float)(scrp[1].ty);// / (128.f*65536.f);
    v++;

	gvtx[v].x       = (float)ev[2].scrx;
    gvtx[v].y       = (float)ev[2].scry;
    gvtx[v].z       = -(_ZSCALE / ev[2].v.z);
    gvtx[v].rhw      = 1;//gvtx[v].z * _AZSCALE;
    gvtx[v].color    = (int)(ev[2].Light) * 0x00010101 | alpha3<<24;
    gvtx[v].tu       = (float)(scrp[2].tx);// / (128.f*65536.f);
    gvtx[v].tv       = (float)(scrp[2].ty);// / (128.f*65536.f);
    v++;

	GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
}

void DrawTPlaneClip(BOOL SECONT)
{
	if (!WATERREVERSE)
	{
		MulVectorsVect(SubVectors(ev[1].v, ev[0].v), SubVectors(ev[2].v, ev[0].v), nv);
		if (nv.x*ev[0].v.x  +  nv.y*ev[0].v.y  +  nv.z*ev[0].v.z<0) return;
	}

	cp[0].ev = ev[0]; cp[1].ev = ev[1]; cp[2].ev = ev[2];

	// == Process the Triangle Texture mapping == //
	_fillMappingClip(SECONT);

	vused = 3;


	for (u=0; u<vused; u++) cp[u].ev.v.z+=16.0f;
	for (u=0; u<vused; u++) ClipVector(ClipZ,u);
	for (u=0; u<vused; u++) cp[u].ev.v.z-=16.0f;
	if (vused<3) return;

	for (u=0; u<vused; u++) ClipVector(ClipA,u); if (vused<3) return;
	for (u=0; u<vused; u++) ClipVector(ClipB,u); if (vused<3) return;
	for (u=0; u<vused; u++) ClipVector(ClipC,u); if (vused<3) return;
	for (u=0; u<vused; u++) ClipVector(ClipD,u); if (vused<3) return;

	for (u=0; u<vused; u++)
	{
		cp[u].ev.scrx = int(VideoCXf - cp[u].ev.v.x / cp[u].ev.v.z * CameraW);
		cp[u].ev.scry = int(VideoCYf + cp[u].ev.v.y / cp[u].ev.v.z * CameraH);
	}

	//Triangle start

	for (u=0; u<vused-2; u++)
	{
		int v=0;
		gvtx[v].x       = (float)cp[0].ev.scrx;
		gvtx[v].y       = (float)cp[0].ev.scry;
		gvtx[v].z       = -(_ZSCALE / cp[0].ev.v.z);
		gvtx[v].rhw      = 1.0f;//gvtx[v].z * _AZSCALE;
		gvtx[v].color    = (int)(cp[0].ev.Light) * 0x00010101 | ((int)cp[0].ev.ALPHA<<24);
		gvtx[v].tu       = (float)(cp[0].tx);
		gvtx[v].tv       = (float)(cp[0].ty);
		v++;

		gvtx[v].x       = (float)cp[u+1].ev.scrx;
		gvtx[v].y       = (float)cp[u+1].ev.scry;
		gvtx[v].z       = -(_ZSCALE / cp[u+1].ev.v.z);
		gvtx[v].rhw      = 1.0f;//gvtx[v].z * _AZSCALE;
		gvtx[v].color    = (int)(cp[u+1].ev.Light) * 0x00010101 | ((int)cp[u+1].ev.ALPHA<<24);
		gvtx[v].tu       = (float)(cp[u+1].tx);
		gvtx[v].tv       = (float)(cp[u+1].ty);
		v++;

		gvtx[v].x       = (float)cp[u+2].ev.scrx;
		gvtx[v].y       = (float)cp[u+2].ev.scry;
		gvtx[v].z       = -(_ZSCALE / cp[u+2].ev.v.z);
		gvtx[v].rhw      = 1.0f;//gvtx[v].z * _AZSCALE;
		gvtx[v].color    = (int)(cp[u+2].ev.Light) * 0x00010101 | ((int)cp[u+2].ev.ALPHA<<24);
		gvtx[v].tu       = (float)(cp[u+2].tx);
		gvtx[v].tv       = (float)(cp[u+2].ty);
		v++;

		GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
	}
}

void DrawTPlaneW(BOOL SECONT)
{
	int n;

	Mask1=0x007F;
	for (n=0; n<3; n++)
	{
		if (ev[n].DFlags & 128) return;
		Mask1=Mask1 & ev[n].DFlags;
	}

	if (Mask1>0) return;


	_fillMapping(SECONT);


	if (!UNDERWATER)
	if (zs > (ctViewR-8)<<8)
	{
		float zz;
		zz = VectorLength(ev[0].v) - 256 * (ctViewR-4);
		if (zz > 0) ev[0].ALPHA = (short)max(0.f,255.f - zz / 3.f);

		zz = VectorLength(ev[1].v) - 256 * (ctViewR-4);
		if (zz > 0) ev[1].ALPHA = (short)max(0.f, 255.f - zz / 3.f);

		zz = VectorLength(ev[2].v) - 256 * (ctViewR-4);
		if (zz > 0) ev[2].ALPHA = (short)max(0,255.f - zz / 3.f);
	}

    gvtx[0].x       = (float)ev[0].scrx;
    gvtx[0].y       = (float)ev[0].scry;
    gvtx[0].z       = -(_ZSCALE / ev[0].v.z);
    gvtx[0].rhw      = 1;//lpVertexG->sz * _AZSCALE;
    gvtx[0].color    = (int)(ev[0].Light) * 0x00010101 | ev[0].ALPHA<<24;
	gvtx[0].tu       = (float)(scrp[0].tx);
    gvtx[0].tv       = (float)(scrp[0].ty);

	gvtx[1].x       = (float)ev[1].scrx;
    gvtx[1].y       = (float)ev[1].scry;
    gvtx[1].z       = -(_ZSCALE / ev[1].v.z);
    gvtx[1].rhw      = 1;//lpVertexG->sz * _AZSCALE;
    gvtx[1].color    = (int)(ev[1].Light) * 0x00010101 | ev[1].ALPHA<<24;
	gvtx[1].tu       = (float)(scrp[1].tx);
    gvtx[1].tv       = (float)(scrp[1].ty);

	gvtx[2].x       = (float)ev[2].scrx;
    gvtx[2].y       = (float)ev[2].scry;
    gvtx[2].z       = -(_ZSCALE / ev[2].v.z);
    gvtx[2].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
    gvtx[2].color    = (int)(ev[2].Light) * 0x00010101 | ev[2].ALPHA<<24;
	gvtx[2].tu       = (float)(scrp[2].tx);
    gvtx[2].tv       = (float)(scrp[2].ty);

    GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
}


void ProcessMap(int x, int y, int r)
{
	if (x>=ctMapSize-1 || y>=ctMapSize-1 || x<0 || y<0) return;

	float BackR = BackViewR;

	if (OMap[y][x]!=255) BackR+=MObjects[OMap[y][x]].info.BoundR;

	ev[0] = VMap[y-CCY+128][x-CCX+128];
	if (ev[0].v.z>BackR) return;

	int t1 = TMap1[y][x];
	ReverseOn = (FMap[y][x] & fmReverse);
	TDirection = (FMap[y][x] & 3);

	x = x - CCX + 128;
	y = y - CCY + 128;

	ev[1] = VMap[y][x+1];

	if (ReverseOn) ev[2] = VMap[y+1][x];
    else ev[2] = VMap[y+1][x+1];

	float xx = (ev[0].v.x + VMap[y+1][x+1].v.x) / 2;
	float yy = (ev[0].v.y + VMap[y+1][x+1].v.y) / 2;
	float zz = (ev[0].v.z + VMap[y+1][x+1].v.z) / 2;

	if ( fabs(xx*FOVK) > -zz + BackR) return;


	zs = (int)sqrtf( xx*xx + zz*zz + yy*yy);

	glBindTexture(GL_TEXTURE_2D, pTextures[Textures[t1]->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (r>8)	DrawTPlane(FALSE);
	else		DrawTPlaneClip(FALSE);//DrawTPlaneClip(FALSE);

	if (ReverseOn)	{ ev[0] = ev[2]; ev[2] = VMap[y+1][x+1]; }
    else			{ ev[1] = ev[2]; ev[2] = VMap[y+1][x];   }

	if (r>8)	DrawTPlane(TRUE);
    else		DrawTPlaneClip(TRUE);//DrawTPlaneClip(TRUE);

	x = x + CCX - 128;
	y = y + CCY - 128;

	if (OMap[y][x]==255) return;

	if (zz<BackR) RenderObject(x, y);
}

void ProcessMap2(int x, int y, int r)
{
	if (x>=ctMapSize-1 || y>=ctMapSize-1 || x<0 || y<0) return;

	ev[0] = VMap[y-CCY+128][x-CCX+128];
	if (ev[0].v.z>BackViewR) return;

	int t1 = TMap2[y][x];
	TDirection = ((FMap[y][x]>>8) & 3);
	ReverseOn = FALSE;

	x = x - CCX + 128;
	y = y - CCY + 128;

	ev[1] = VMap[y][x+2];
	if (ReverseOn) ev[2] = VMap[y+2][x];
             else ev[2] = VMap[y+2][x+2];

	float xx = (ev[0].v.x + VMap[y+2][x+2].v.x) / 2;
	float yy = (ev[0].v.y + VMap[y+2][x+2].v.y) / 2;
	float zz = (ev[0].v.z + VMap[y+2][x+2].v.z) / 2;

	if ( fabsf(xx*FOVK) > -zz + BackViewR) return;

	zs = (int)sqrtf( xx*xx + zz*zz + yy*yy);
	if (zs>ctViewR*256) return;

	glBindTexture(GL_TEXTURE_2D, pTextures[Textures[t1]->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	DrawTPlane(FALSE);

	if (ReverseOn) { ev[0] = ev[2]; ev[2] = VMap[y+2][x+2]; }
             else { ev[1] = ev[2]; ev[2] = VMap[y+2][x];   }

	DrawTPlane(TRUE);

	glDisable(GL_BLEND);


	x = x + CCX - 128;
	y = y + CCY - 128;

	RenderObject(x  , y);
	RenderObject(x+1, y);
	RenderObject(x  , y+1);
	RenderObject(x+1, y+1);
}

void ProcessMapW(int x, int y, int r)
{
   if (!( (FMap[y  ][x  ] & fmWaterA) &&
	      (FMap[y  ][x+1] & fmWaterA) &&
		  (FMap[y+1][x  ] & fmWaterA) &&
		  (FMap[y+1][x+1] & fmWaterA) )) return;


   WATERREVERSE = TRUE;
   int t1 = WaterList[ WMap[y][x] ].tindex;

   ev[0] = VMap2[y-CCY+128][x-CCX+128];
   if (ev[0].v.z>BackViewR) return;

   ReverseOn = FALSE;
   TDirection = 0;

   x = x - CCX + 128;
   y = y - CCY + 128;
   ev[1] = VMap2[y][x+1];
   ev[2] = VMap2[y+1][x+1];

   float xx = (ev[0].v.x + VMap2[y+1][x+1].v.x) / 2;
   float yy = (ev[0].v.y + VMap2[y+1][x+1].v.y) / 2;
   float zz = (ev[0].v.z + VMap2[y+1][x+1].v.z) / 2;

	if ( fabsf(xx*FOVK) > -zz + BackViewR) return;

	zs = (int)sqrtf( xx*xx + zz*zz + yy*yy);
	if (zs > ctViewR*256) return;

	//if (!LOWRESTX)
	glBindTexture(GL_TEXTURE_2D, pTextures[Textures[t1]->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (r>8)	DrawTPlaneW(FALSE);
    else		DrawTPlaneClip(FALSE);

	ev[1] = ev[2]; ev[2] = VMap2[y+1][x];

	if (r>8)	DrawTPlaneW(TRUE);
    else		DrawTPlaneClip(TRUE);

	WATERREVERSE = FALSE;
}

void ProcessMapW2(int x, int y, int r)
{
	if (!( (FMap[y  ][x  ] & fmWaterA) &&
	      (FMap[y  ][x+2] & fmWaterA) &&
		  (FMap[y+2][x  ] & fmWaterA) &&
		  (FMap[y+2][x+2] & fmWaterA) )) return;

	int t1 = WaterList[ WMap[y][x] ].tindex;

	ev[0] = VMap2[y-CCY+128][x-CCX+128];
	if (ev[0].v.z>BackViewR) return;

	//WATERREVERSE = TRUE;
	ReverseOn = FALSE;
	TDirection = 0;

	x = x - CCX + 128;
	y = y - CCY + 128;
	ev[1] = VMap2[y][x+2];
	ev[2] = VMap2[y+2][x+2];

	float xx = (ev[0].v.x + VMap2[y+2][x+2].v.x) / 2;
	float yy = (ev[0].v.y + VMap2[y+2][x+2].v.y) / 2;
	float zz = (ev[0].v.z + VMap2[y+2][x+2].v.z) / 2;

	if ( fabs(xx*FOVK) > -zz + BackViewR) return;

	zs = (int)sqrtf( xx*xx + zz*zz + yy*yy);
	if (zs > ctViewR*256) return;


	glBindTexture(GL_TEXTURE_2D, pTextures[Textures[t1]->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	DrawTPlaneW(FALSE);
	ev[1] = ev[2]; ev[2] = VMap2[y+2][x];

	DrawTPlaneW(TRUE);
}

void RenderHealthBar()
{
	//if (MyHealth >= 100000) return;
	//if (MyHealth == 000000) return;


	int L = WinW / 4;
	int x0 = WinW - (WinW / 20) - L;
	int y0 = WinH / 40;
	int G = min( (MyHealth * 240 / 100000), 160);
	int R = min( ( (100000 - MyHealth) * 240 / 100000), 160);


	int L0 = (L * MyHealth) / 100000;
	int H = WinH / 200;

	for (int y=0; y<4; y++)
	{
		gvtx[0].x       = (float)x0-1;
		gvtx[0].y       = (float)y0+y;
		gvtx[0].z       = 0.9999f;
		gvtx[0].rhw      = 1.f;
		gvtx[0].color    = 0xF0000000;

		gvtx[1].x       = (float)x0+L+1;
		gvtx[1].y       = (float)y0+y;
		gvtx[1].z       = 0.9999f;
		gvtx[1].rhw      = 1.f;
		gvtx[1].color    = 0xF0000000;

		GLDrawLine(gvtx[0],gvtx[1]);
	}

	for (int y=1; y<3; y++)
	{
		gvtx[0].x       = (float)x0;
		gvtx[0].y       = (float)y0+y;
		gvtx[0].z       = 0.99999f;
		gvtx[0].rhw      = 1.f;
		gvtx[0].color    = 0xF0000000 + (G<<8) + (R<<0);

		gvtx[1].x       = (float)x0+L0;
		gvtx[1].y       = (float)y0+y;
		gvtx[1].z       = 0.99999f;
		gvtx[1].rhw      = 1.f;
		gvtx[1].color    = 0xF0000000 + (G<<8) + (R<<0);

		GLDrawLine(gvtx[0],gvtx[1]);
	}
}

void RenderFSRect(DWORD Color)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	gvtx[0].x       = 0.f;
    gvtx[0].y       = 0.f;
    gvtx[0].z       = 0.0;
    gvtx[0].rhw      = 1.f;
    gvtx[0].color    = Color;
    gvtx[0].tu       = 0;
    gvtx[0].tv       = 0;

	gvtx[1].x       = (float)WinW;
    gvtx[1].y       = 0.f;
    gvtx[1].z       = 0.0f;
    gvtx[1].rhw      = 1.f;
    gvtx[1].color    = Color;
    gvtx[1].tu       = 0;
    gvtx[1].tv       = 0;

	gvtx[2].x       = (float)WinW;
    gvtx[2].y       = (float)WinH;
    gvtx[2].z       = (float)0.0f;
    gvtx[2].rhw      = 1.f;
    gvtx[2].color    = Color;
    gvtx[2].tu       = 0;
    gvtx[2].tv       = 0;

	gvtx[3].x       = 0.f;
    gvtx[3].y       = (float)WinH;
    gvtx[3].z       = 0.0f;
    gvtx[3].rhw      = 1.f;
    gvtx[3].color    = Color;
    gvtx[3].tu       = 0;
    gvtx[3].tv       = 0;

	glBindTexture(GL_TEXTURE_2D, 0);

	GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
	GLDrawTriangle(gvtx[2],gvtx[3],gvtx[0]);

	glDisable(GL_BLEND);
}


void RenderModel(TModel* _mptr, float x0, float y0, float z0, int light, int VT, float al, float bt)
{
   int f;

   if (fabs(y0) > -(z0-256*6)) return;

   mptr = _mptr;

   float ca = (float)cos(al);
   float sa = (float)sin(al);

   float cb = (float)cos(bt);
   float sb = (float)sin(bt);

   float minx = 10241024;
   float maxx =-10241024;
   float miny = 10241024;
   float maxy =-10241024;

   BOOL FOGACTIVE = (FOGON && (FogYBase>0));

   int alphamask = (255-GlassL)<<24;
   int ml = light;

   TPoint3d p;
   for (int s=0; s<mptr->VCount; s++) {
    p = mptr->gVertex[s];

	if (FOGACTIVE) {
	 vFogT[s] = 255-(int)(FogYBase + p.y * FogYGrad);
	 if (vFogT[s]<5  ) vFogT[s] = 5;
	 if (vFogT[s]>255) vFogT[s]=255;
	 vFogT[s]<<=24;
	} else vFogT[s] = 255<<24;



    rVertex[s].x = (p.x * ca + p.z * sa) + x0;

    float vz = p.z * ca - p.x * sa;

    rVertex[s].y = (p.y * cb - vz * sb)  + y0;
    rVertex[s].z = (vz  * cb + p.y * sb) + z0;

    if (rVertex[s].z<-32) {
     gScrpf[s].x = VideoCXf - (rVertex[s].x / rVertex[s].z * CameraW);
     gScrpf[s].y = VideoCYf + (rVertex[s].y / rVertex[s].z * CameraH);
	} else gScrpf[s].x=-102400;

     if (gScrpf[s].x > maxx) maxx = gScrpf[s].x;
     if (gScrpf[s].x < minx) minx = gScrpf[s].x;
     if (gScrpf[s].y > maxy) maxy = gScrpf[s].y;
     if (gScrpf[s].y < miny) miny = gScrpf[s].y;
   }

   if (minx == 10241024) return;
   if (minx>WinW || maxx<0 || miny>WinH || maxy<0) return;


   BuildTreeNoSortf();

   float d = (float) sqrt(x0*x0 + y0*y0 + z0*z0);
   if (LOWRESTX) d = 14*256;

	glBindTexture(GL_TEXTURE_2D, pTextures[mptr->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   int PrevOpacity = 0;
   int NewOpacity = 0;
   int PrevTransparent = 0;
   int NewTransparent = 0;

   int fproc1 = 0;
   int fproc2 = 0;
   f = Current;
   BOOL CKEY = FALSE;
	while( f!=-1 )
	{
		TFace *fptr = & mptr->gFace[f];
		f = mptr->gFace[f].Next;

		if (minx<0)
		if (gScrpf[fptr->v1].x <0    && gScrpf[fptr->v2].x<0    && gScrpf[fptr->v3].x<0) continue;
		if (maxx>WinW)
		if (gScrpf[fptr->v1].x >WinW && gScrpf[fptr->v2].x>WinW && gScrpf[fptr->v3].x>WinW) continue;

		if (fptr->Flags & (sfOpacity | sfTransparent)) fproc2++; else fproc1++;

		int _ml = ml + mptr->VLight[VT][fptr->v1];
		int v=0;
		gvtx[v].x       = (float)gScrpf[fptr->v1].x;
		gvtx[v].y       = (float)gScrpf[fptr->v1].y;
		gvtx[v].z       = -(_ZSCALE / rVertex[fptr->v1].z);
		gvtx[v].rhw      = 1.f;
		gvtx[v].color    = _ml * 0x00010101 | alphamask;
		gvtx[v].tu       = float(fptr->tax / 256.0f);
		gvtx[v].tv       = float(fptr->tay / 256.0f);
		v++;

		_ml = ml + mptr->VLight[VT][fptr->v2];
		gvtx[v].x       = (float)gScrpf[fptr->v2].x;
		gvtx[v].y       = (float)gScrpf[fptr->v2].y;
		gvtx[v].z       = -(_ZSCALE / rVertex[fptr->v2].z);
		gvtx[v].rhw      = 1.f;
		gvtx[v].color    = _ml * 0x00010101 | alphamask;
		gvtx[v].tu       = (float)(fptr->tbx/256.0f);
		gvtx[v].tv       = (float)(fptr->tby/256.0f);
		v++;

		_ml = ml + mptr->VLight[VT][fptr->v3];
		gvtx[v].x       = (float)gScrpf[fptr->v3].x;
		gvtx[v].y       = (float)gScrpf[fptr->v3].y;
		gvtx[v].z       = -(_ZSCALE / rVertex[fptr->v3].z);
		gvtx[v].rhw      = 1.f;
		gvtx[v].color    = _ml * 0x00010101 | alphamask;
		gvtx[v].tu       = (float)(fptr->tcx/256.0f);
		gvtx[v].tv       = (float)(fptr->tcy/256.0f);
		v++;

		if (fptr->Flags & (sfOpacity | sfTransparent)) glEnable(GL_ALPHA_TEST);

		GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

		if (fptr->Flags & (sfOpacity | sfTransparent)) glDisable(GL_ALPHA_TEST);

		if (fproc1+fproc2>256)
		{
			fproc1 = fproc2 = 0;
		}
	}
}


void RenderModelClip(TModel* _mptr, float x0, float y0, float z0, int light, int VT, float al, float bt)
{
	int f,CMASK;
	mptr = _mptr;

	float ca = (float)cos(al);
	float sa = (float)sin(al);

	float cb = (float)cos(bt);
	float sb = (float)sin(bt);


	int flight = (int)light;
	DWORD almask;
	DWORD alphamask = (255-GlassL)<<24;


	BOOL BL = FALSE;
	BOOL FOGACTIVE = (FOGON && (FogYBase>0));

	for (int s=0; s<mptr->VCount; s++) {

	if (FOGACTIVE)
	{
		vFogT[s] = 255-(int)(FogYBase + mptr->gVertex[s].y * FogYGrad);
		if (vFogT[s]<5  ) vFogT[s] = 5;
		if (vFogT[s]>255) vFogT[s]=255;
	}
	else vFogT[s] = 255;


	rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
    float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
    rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
    rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
    if (rVertex[s].z<0) BL=TRUE;

    if (rVertex[s].z>-256) { gScrp[s].x = 0xFFFFFF; gScrp[s].y = 0xFF; }
    else {
     int f = 0;
     int sx =  VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
     int sy =  VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);

     if (sx>=WinEX) f+=1;
     if (sx<=0    ) f+=2;

     if (sy>=WinEY) f+=4;
     if (sy<=0    ) f+=8;

     gScrp[s].y = f;
    }

	}

	if (!BL) return;


	glBindTexture(GL_TEXTURE_2D, pTextures[mptr->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (mptr->Index < 0) glBindTexture(GL_TEXTURE_2D, 0);
	if (!glIsTexture(pTextures[mptr->Index])) glBindTexture(GL_TEXTURE_2D, 0);

	BuildTreeClipNoSort();

	f = Current;
	int fproc1 = 0;
	int fproc2 = 0;
	BOOL CKEY = FALSE;

	while( f!=-1 )
	{
		vused = 3;
		TFace *fptr = &mptr->gFace[f];

		CMASK = 0;

		CMASK|=gScrp[fptr->v1].y;
		CMASK|=gScrp[fptr->v2].y;
		CMASK|=gScrp[fptr->v3].y;


		cp[0].ev.v = rVertex[fptr->v1]; cp[0].tx = (float)fptr->tax;  cp[0].ty = (float)fptr->tay; cp[0].ev.Fog = (float)vFogT[fptr->v1]; cp[0].ev.Light = (int)mptr->VLight[VT][fptr->v1];
		cp[1].ev.v = rVertex[fptr->v2]; cp[1].tx = (float)fptr->tbx;  cp[1].ty = (float)fptr->tby; cp[1].ev.Fog = (float)vFogT[fptr->v2]; cp[1].ev.Light = (int)mptr->VLight[VT][fptr->v2];
		cp[2].ev.v = rVertex[fptr->v3]; cp[2].tx = (float)fptr->tcx;  cp[2].ty = (float)fptr->tcy; cp[2].ev.Fog = (float)vFogT[fptr->v3]; cp[2].ev.Light = (int)mptr->VLight[VT][fptr->v3];

		{
		for (u=0; u<vused; u++) cp[u].ev.v.z+= 8.0f;
		for (u=0; u<vused; u++) ClipVector(ClipZ,u);
		for (u=0; u<vused; u++) cp[u].ev.v.z-= 8.0f;
		if (vused<3) goto LNEXT;
		}

		if (CMASK & 1) for (u=0; u<vused; u++) ClipVector(ClipA,u); if (vused<3) goto LNEXT;
		if (CMASK & 2) for (u=0; u<vused; u++) ClipVector(ClipC,u); if (vused<3) goto LNEXT;
		if (CMASK & 4) for (u=0; u<vused; u++) ClipVector(ClipB,u); if (vused<3) goto LNEXT;
		if (CMASK & 8) for (u=0; u<vused; u++) ClipVector(ClipD,u); if (vused<3) goto LNEXT;
		almask = 0xFF000000;
		if (fptr->Flags & sfTransparent) almask = 0x70000000;
		if (almask > alphamask) almask = alphamask;

		for (u=0; u<vused-2; u++)
		{
			int v=0;
			int _flight = flight + cp[0].ev.Light;
	   		gvtx[v].x       = VideoCXf - cp[0].ev.v.x / cp[0].ev.v.z * CameraW;
			gvtx[v].y       = VideoCYf + cp[0].ev.v.y / cp[0].ev.v.z * CameraH;
			gvtx[v].z       = -(_ZSCALE / cp[0].ev.v.z);
			gvtx[v].rhw      = 1.0f;
			gvtx[v].color    = _flight * 0x00010101 | almask;
			gvtx[v].tu       = (float)(cp[0].tx/256.0f);
			gvtx[v].tv       = (float)(cp[0].ty/256.0f);
			v++;

			_flight = flight + cp[u+1].ev.Light;
	   		gvtx[v].x       = VideoCXf - cp[u+1].ev.v.x / cp[u+1].ev.v.z * CameraW;
			gvtx[v].y       = VideoCYf + cp[u+1].ev.v.y / cp[u+1].ev.v.z * CameraH;
			gvtx[v].z       = -(_ZSCALE / cp[u+1].ev.v.z);
			gvtx[v].rhw      = 1.0f;
			gvtx[v].color    = _flight * 0x00010101 | almask;
			gvtx[v].tu       = (float)(cp[u+1].tx/256.0f);
			gvtx[v].tv       = (float)(cp[u+1].ty/256.0f);
			v++;

			_flight = flight + cp[u+2].ev.Light;
	   		gvtx[v].x       = VideoCXf - cp[u+2].ev.v.x / cp[u+2].ev.v.z * CameraW;
			gvtx[v].y       = VideoCYf + cp[u+2].ev.v.y / cp[u+2].ev.v.z * CameraH;
			gvtx[v].z       = -(_ZSCALE / cp[u+2].ev.v.z);
			gvtx[v].rhw      = 1.f;//gvtx[v].z * _AZSCALE;//1.0f;
			gvtx[v].color    = _flight * 0x00010101 | almask;
			gvtx[v].tu       = (float)(cp[u+2].tx/256.0f);
			gvtx[v].tv       = (float)(cp[u+2].ty/256.0f);
			v++;

			if (fptr->Flags & (sfOpacity | sfTransparent))	glEnable(GL_ALPHA_TEST);

			GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

			if (fptr->Flags & (sfOpacity | sfTransparent)) glDisable(GL_ALPHA_TEST);
			if (fptr->Flags & (sfOpacity | sfTransparent)) fproc2++; else fproc1++;
		}
LNEXT:
		f = mptr->gFace[f].Next;

		if (fproc1+fproc2>256)
		{
			fproc1 = fproc2 = 0;
		}
	}
}

void BuildTreeNoSortf()
{
    Vector2df v[3];
	Current = -1;
    int LastFace = -1;
    TFace* fptr;
    int sg;

	for (int f=0; f<mptr->FCount; f++)
	{
        fptr = &mptr->gFace[f];
  		v[0] = gScrpf[fptr->v1];
        v[1] = gScrpf[fptr->v2];
        v[2] = gScrpf[fptr->v3];

        if (v[0].x <-102300) continue;
        if (v[1].x <-102300) continue;
        if (v[2].x <-102300) continue;

        if (fptr->Flags & (sfDarkBack+sfNeedVC)) {
           sg = (int)((v[1].x-v[0].x)*(v[2].y-v[1].y) - (v[1].y-v[0].y)*(v[2].x-v[1].x));
           if (sg<0) continue;
        }

		fptr->Next=-1;
        if (Current==-1) { Current=f; LastFace = f; } else
        { mptr->gFace[LastFace].Next=f; LastFace=f; }

	}
}

int BuildTreeClipNoSort()
{
	Current = -1;
    int fc = 0;
    int LastFace = -1;
    TFace* fptr;

	for (int f=0; f<mptr->FCount; f++)
	{
        fptr = &mptr->gFace[f];

        if (fptr->Flags & (sfDarkBack + sfNeedVC) )
		{
			MulVectorsVect(SubVectors(rVertex[fptr->v2], rVertex[fptr->v1]), SubVectors(rVertex[fptr->v3], rVertex[fptr->v1]), nv);
			if (nv.x*rVertex[fptr->v1].x  +  nv.y*rVertex[fptr->v1].y  +  nv.z*rVertex[fptr->v1].z<0) continue;
        }

        fc++;
        fptr->Next=-1;
        if (Current==-1)	{ Current=f; LastFace = f; }
		else				{ mptr->gFace[LastFace].Next=f; LastFace=f; }

	}
    return fc;
}


void CalcFogLevel_Gradient(Vector3d v)
{
  FogYBase =  CalcFogLevel(v);
  if (FogYBase>0) {
   v.y+=800;
   FogYGrad = (CalcFogLevel(v) - FogYBase) / 800.f;
  } else FogYGrad=0;
}


void RenderObject(int x, int y)
{
	if (OMap[y][x]==255) return;
	if (!MODELS) return;
	if (ORLCount>2000) return;
	ORList[ORLCount].x = x;
	ORList[ORLCount].y = y;
	ORLCount++;
}


void _RenderObject(int x, int y)
{
	int ob = OMap[y][x];

	if (!MObjects[ob].model)
	{
		sprintf(logt,"Incorrect model at [%d][%d]!", x, y);
		DoHalt(logt);
	}

	int FI = (FMap[y][x] >> 2) & 3;
	float fi = CameraAlpha + (float)(FI * 2.f*pi / 4.f);


	int mlight;
	if (MObjects[ob].info.flags & ofDEFLIGHT) mlight = MObjects[ob].info.DefLight;
	else if (MObjects[ob].info.flags & ofGRNDLIGHT)
	{
		mlight = 128;
		CalcModelGroundLight(MObjects[ob].model, (float)(x*256+128), (float)(y*256+128), FI);
		FI = 0;
	}
	else mlight = -(RandomMap[y & 31][x & 31] >> 5) + (LMap[y][x]>>1) + 96;


    if (mlight >192) mlight =192;
	if (mlight < 64) mlight = 64;

	  v[0].x = x*256+128 - CameraX;
      v[0].z = y*256+128 - CameraZ;
      v[0].y = (float)(HMapO[y][x]) * ctHScale - CameraY;

	  float zs = VectorLength(v[0]);

	  CalcFogLevel_Gradient(v[0]);

	  v[0] = RotateVector(v[0]);
	  GlassL = 0;

      if (zs > 256 * (ctViewR-8))
	   GlassL=min(255,(int)(zs - 256 * (ctViewR-8)) / 4);

	  if (GlassL==255) return;

	  if (MObjects[ob].info.flags & ofANIMATED)
	   if (MObjects[ob].info.LastAniTime!=RealTime) {
        MObjects[ob].info.LastAniTime=RealTime;
		CreateMorphedObject(MObjects[ob].model,
		                    MObjects[ob].vtl,
							RealTime % MObjects[ob].vtl.AniTime);
	   }



	if (MObjects[ob].info.flags & ofNOBMP) zs = 0;

	if (zs>ctViewRM*256)
	{
		GLStartBuffer();
		RenderBMPModel(&MObjects[ob].bmpmodel, v[0].x, v[0].y, v[0].z, mlight-16);
		GLEndBuffer();
	}
	else
	if (v[0].z<-256*8)
	{
		GLStartBuffer();
		RenderModel(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, FI, fi, CameraBeta);
		GLEndBuffer();
	}
	else
	{
		GLStartBuffer();
		RenderModelClip(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, FI, fi, CameraBeta);
		GLEndBuffer();
	}
}


//=========================================================================//
// Global Source (All .cpp files)
//=========================================================================//
void Init3DHardware()
{
	// -> Perform 3d hardware startup (OpenGL, DD, D3D,  etc)

	PrintLog("Initializing OpenGL... ");

    // Get the device context (DC)
    DeviceContext = GetDC (hwndMain);
	if (DeviceContext==NULL)
	{
		DoHalt("Failed to create the OpenGL Device Context!");
		return;
	}

    // Set the pixel format for the DC
    ZeroMemory (&pfd, sizeof (pfd));
    pfd.nSize = sizeof (pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat (DeviceContext, &pfd);

	HARD3D = TRUE;
	PrintLog("[Ok]\n");
}

void Activate3DHardware()
{
	// == Global Function == //
	// -> Called by reinit game
	// -> Simply reload/restart hardware (Was shutdown by shutdown3dhardware)

	PrintLog("Activating OpenGL... ");

	if (WinW<640) SetVideoMode(640,480);
	else SetVideoMode(WinW,WinH);

	SetVideoMode(WinW,WinH);
	MoveWindow(hwndMain,0,0,WinW,WinH,TRUE);

	// Go Fullscreen
	if (!Windowed) {
	DEVMODE dmScreenSettings;
    memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
    dmScreenSettings.dmSize=sizeof(dmScreenSettings);
    dmScreenSettings.dmPelsWidth = WinW;
    dmScreenSettings.dmPelsHeight = WinH;
    dmScreenSettings.dmBitsPerPel = 16;
    dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
    ChangeDisplaySettings( &dmScreenSettings,CDS_FULLSCREEN);
	}

	// Set the Pixel Format
	SetPixelFormat (DeviceContext, iFormat, &pfd);

    // Create and enable the render context (RC)
    RenderContext = wglCreateContext( DeviceContext );
    wglMakeCurrent( DeviceContext, RenderContext );

	GLFontCreate();
	glGenTextures(2048,pTextures);

	// OpenGL Hints
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glHint(GL_FOG_HINT,GL_NICEST);

	// Basic OpenGL Properties
	glEnable(GL_TEXTURE_2D);
	//glEnable(GL_AUTO_NORMAL);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.9f);

	// == ReCreate GL Textures == //
	//Terrain Textures
	for (int i=0; i<TextureCount; i++)
	{
		Textures[i]->Index = GLCreateTexture(true, 128, 128, Textures[i]->DataA);
	}
	//Model textures
	for (int i=0; i<ModelCount; i++)
	{
		MObjects[i].model->Index = GLCreateTexture(true, 256, 256, MObjects[i].model->lpTexture);
		MObjects[i].bmpmodel.Index = GLCreateTexture(true, 128, 128, MObjects[i].bmpmodel.lpTexture);
	}
	//Weapon textures
	for (int i=0; i<TotalW; i++)
	{
		if (!Weapon.chinfo[i].mptr) continue;
		Weapon.chinfo[i].mptr->Index = GLCreateTexture(true, 256, 256, Weapon.chinfo[i].mptr->lpTexture);
		Weapon.BulletPic[i].Index = GLCreateSprite(false,Weapon.BulletPic[i]);
	}
	//Character textures
	for (int i=0; i<TotalC; i++)
	{
		if (!ChInfo[i].mptr) continue;
		ChInfo[i].mptr->Index = GLCreateTexture(true, 256, 256, ChInfo[i].mptr->lpTexture);
		DinoInfo[i].CallIcon.Index = GLCreateSprite(false, DinoInfo[i].CallIcon);
	}
	//Ship texture
	ShipModel.mptr->Index = GLCreateTexture(true, 256, 256, ShipModel.mptr->lpTexture);
	//Compass Texture
	CompasModel->Index = GLCreateTexture(true, 256, 256, CompasModel->lpTexture);
	//Binocular Texture
	Binocular->Index = GLCreateTexture(true, 256, 256, Binocular->lpTexture);
	//WindMode Texture
	WindModel.mptr->Index = GLCreateTexture(true, 256, 256, WindModel.mptr->lpTexture);
	//Water Ring Texture
	WCircleModel.mptr->Index = GLCreateTexture(false, 256, 256, WCircleModel.mptr->lpTexture);
	//Environment Effect Texture
	TFX_ENVMAP.Index = GLCreateTexture(false, 256, 256, TFX_ENVMAP.lpImage);
	//Specular Effect Texture
	TFX_SPECULAR.Index = GLCreateTexture(false, 256, 256, TFX_SPECULAR.lpImage);
	//Sky Texture
	glBindTexture(GL_TEXTURE_2D, pTextures[2043]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5_A1,256,256,0,GL_BGRA,GL_UNSIGNED_SHORT_1_5_5_5_REV,SkyPic);
	//Reset Texture Filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Create Sprites for UI
	PausePic.Index = GLCreateSprite(false,PausePic);
	ExitPic.Index = GLCreateSprite(false,ExitPic);
	TrophyExit.Index = GLCreateSprite(false,TrophyExit);
	TrophyPic.Index = GLCreateSprite(false,TrophyPic);
	MapPic.Index = GLCreateSprite(false,MapPic);

	// Clear the buffers
	GLClearBuffers();
	PrintLog("[Ok]\n");
}

void ShutDown3DHardware()
{
	// -> Shutdown hardware

	PrintLog("Shutting down OpenGL... ");

	GLDeleteFont();
	glDeleteTextures(2048,pTextures);

	wglMakeCurrent (NULL, NULL);
    wglDeleteContext (RenderContext);
    ReleaseDC (hwndMain, DeviceContext);

	PrintLog("[Ok]\n");
}


void DrawFogOverlay()
{
	// ==ADP global function: renders fog mode overlay == //
}


void RenderBMPModel(TBMPModel* mptr, float x0, float y0, float z0, int light)
{
	if (fabsf(y0) > -(z0-256*6)) return;

	int minx = 10241024;
	int maxx =-10241024;
	int miny = 10241024;
	int maxy =-10241024;

	BOOL FOGACTIVE = (FOGON && (FogYBase>0));

	for (int s=0; s<4; s++)
	{

		if (FOGACTIVE) {
		vFogT[s] = 255-(int) (FogYBase + mptr->gVertex[s].y * FogYGrad);
		if (vFogT[s]<0  ) vFogT[s] = 0;
		if (vFogT[s]>255) vFogT[s]=255;
		vFogT[s]<<=24;
		} else vFogT[s]=255<<24;

		rVertex[s].x = mptr->gVertex[s].x + x0;
		rVertex[s].y = mptr->gVertex[s].y * cb + y0;
		rVertex[s].z = mptr->gVertex[s].y * sb + z0;

		if (rVertex[s].z<-256) {
		gScrp[s].x = VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
		gScrp[s].y = VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);
		} else return;

		if (gScrp[s].x > maxx) maxx = gScrp[s].x;
		if (gScrp[s].x < minx) minx = gScrp[s].x;
		if (gScrp[s].y > maxy) maxy = gScrp[s].y;
		if (gScrp[s].y < miny) miny = gScrp[s].y;
	}

	if (minx == 10241024) return;
	if (minx>WinW || maxx<0 || miny>WinH || maxy<0) return;

	int argb = light * 0x00010101 + ((255-GlassL)<<24);

	float d = (float) sqrtf(x0*x0 + y0*y0 + z0*z0);

	glBindTexture(GL_TEXTURE_2D, pTextures[mptr->Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glEnable(GL_ALPHA_TEST);

	gvtx[0].x       = (float)gScrp[0].x;
	gvtx[0].y       = (float)gScrp[0].y;
	gvtx[0].z       = -(_ZSCALE / rVertex[0].z);
	gvtx[0].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	gvtx[0].color    = argb;
	gvtx[0].tu       = (float)(0.0f);
	gvtx[0].tv       = (float)(0.0f);

	gvtx[1].x       = (float)gScrp[1].x;
	gvtx[1].y       = (float)gScrp[1].y;
	gvtx[1].z       = -(_ZSCALE / rVertex[1].z);
	gvtx[1].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	gvtx[1].color    = argb;
	gvtx[1].tu       = (float)(0.995f);
	gvtx[1].tv       = (float)(0.0f);

	gvtx[2].x       = (float)gScrp[2].x;
	gvtx[2].y       = (float)gScrp[2].y;
	gvtx[2].z       = -(_ZSCALE / rVertex[2].z);
	gvtx[2].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	gvtx[2].color    = argb;
	gvtx[2].tu       = (float)(0.995f);
	gvtx[2].tv       = (float)(0.995f);

	GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

	//=========//

	gvtx[0].x       = (float)gScrp[0].x;
	gvtx[0].y       = (float)gScrp[0].y;
	gvtx[0].z       = -(_ZSCALE / rVertex[0].z);
	gvtx[0].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	gvtx[0].color    = argb;
	gvtx[0].tu       = (float)(0.0f);
	gvtx[0].tv       = (float)(0.0f);

    gvtx[1].x       = (float)gScrp[2].x;
    gvtx[1].y       = (float)gScrp[2].y;
    gvtx[1].z       = -(_ZSCALE / rVertex[2].z);
    gvtx[1].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
    gvtx[1].color    = argb;
    gvtx[1].tu       = (float)(0.995f);
    gvtx[1].tv       = (float)(0.995f);

	gvtx[2].x       = (float)gScrp[3].x;
	gvtx[2].y       = (float)gScrp[3].y;
	gvtx[2].z       = -(_ZSCALE / rVertex[3].z);
	gvtx[2].rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	gvtx[2].color    = argb;
	gvtx[2].tu       = (float)(0.0f);
	gvtx[2].tv       = (float)(0.995f);

	GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

	glDisable(GL_ALPHA_TEST);
}


void Draw_Text(int x,int y,char* text, unsigned long color)
{
	GLTextOut(x,y,text,color);
}


void ShowVideo()
{
	// == Global Function == //
	// -> Simply draw everything
	// -> Also draw the health and energy bar
	//Render overlays

	glDepthFunc(GL_ALWAYS);
	if (OptDayNight==2 && !NightVision) glDisable(GL_FOG);
	//Draw the underwater haze
	if (UNDERWATER) RenderFSRect(0x80000000 + CurFogColor);

	//Draw the sun glare
	if (OptDayNight!=2)
	if (!UNDERWATER && (SunLight>1.0f) )
	{
		RenderFSRect(0xFFFFC0 + ((int)SunLight<<24));
	}

	if (!UNDERWATER && MyHealth != MyPrevHealth && MyHealth<MyPrevHealth)
		RenderFSRect(0xA00000FF);

	//Draw the sun flare
	//->ToDo: James

	//Draw the health bar
	RenderHealthBar();

	char Str[256];
	sprintf(Str,"Beta: %f\r\nAlpha: %f",CameraBeta,CameraAlpha);
	GLTextOut(WinW/2,3, Str, 0xFFFFFFFF);

	glDepthFunc(GL_LESS);
	if (OptDayNight==2 && !NightVision) glEnable(GL_FOG);

	MyPrevHealth = MyHealth;

	SwapBuffers(DeviceContext);
	GLClearBuffers();
}

void DrawPicture(int x, int y, TPicture &pic)
{
	// == Global Function == //
	// -> Draw a picture on screen

	if (OptDayNight==2 && !NightVision) glDisable(GL_FOG);

	glBindTexture(GL_TEXTURE_2D, pTextures[pic.Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	float fW = float(pic.W);
	float fH = float(pic.H);
	float fcW = float(pic.cW);
	float fcH = float(pic.cH);

	glBegin(GL_QUADS);
	glColor4f(1,1,1,1);

	glTexCoord2f(0,0);
	glVertex4f(float(x),float(y),0.999999f,1);
	glTexCoord2f(fW/fcW,0);
	glVertex4f(float(x+pic.W),float(y),0.999999f,1);
	glTexCoord2f(fW/fcW, fH/fcH);
	glVertex4f(float(x+pic.W),float(y+pic.H),0.999999f,1);
	glTexCoord2f(0, fH/fcH);
	glVertex4f(float(x),float(y+pic.H),0.999999f,1);

	glEnd();

	if (OptDayNight==2 && !NightVision) glEnable(GL_FOG);
}


void ShowControlElements()
{
	// == Global Function == //
	// -> Used to draw some informative text on screen
	// -> Includes drawing weapon list

	char buf[128];

	if (TIMER)
	{
		sprintf(buf,"msc: %d", TimeDt);
		GLTextOut(WinEX-81, 11, buf, 0xFF20A0A0);

		sprintf(buf,"polys: %d", dFacesCount);
		GLTextOut(WinEX-90, 24, buf, 0xFF20A0A0);
	}

	if (MessageList.timeleft)
	{
		if (RealTime>MessageList.timeleft) MessageList.timeleft = 0;
		GLTextOut(10, 10, MessageList.mtext, 0xFF20A0A0);
	}

	if (ExitTime)
	{
		int y = WinH / 3;
		sprintf(buf,"Preparing for evacuation...");
		GLTextOut(VideoCX - GetTextW(hdcCMain, buf)/2, y, buf, 0xFF60C0D0);
		sprintf(buf,"%d seconds left.", 1 + ExitTime / 1000);
		GLTextOut(VideoCX - GetTextW(hdcCMain, buf)/2, y + 18, buf, 0xFF60C0D0);
	}
}


void RenderModelsList()
{
	// == Global Function == //
	// -> Loop through objects in view list/array and render them
	for (int o=0; o<ORLCount; o++) _RenderObject(ORList[o].x, ORList[o].y);
	ORLCount=0;
}


void RenderGround()
{
	// == Global Function == //
	// -> Render the ground

	for (r=ctViewR; r>=ctViewR1; r-=2)
	{
		for (int x=r; x>0; x-=2)
		{
			ProcessMap2(CCX-x, CCY+r, r);
			ProcessMap2(CCX+x, CCY+r, r);
			ProcessMap2(CCX-x, CCY-r, r);
			ProcessMap2(CCX+x, CCY-r, r);
		}

		ProcessMap2(CCX, CCY-r, r);
		ProcessMap2(CCX, CCY+r, r);

		for (int y=r-2; y>0; y-=2)
		{
			ProcessMap2(CCX+r, CCY-y, r);
			ProcessMap2(CCX+r, CCY+y, r);
			ProcessMap2(CCX-r, CCY+y, r);
			ProcessMap2(CCX-r, CCY-y, r);
		}
		ProcessMap2(CCX-r, CCY, r);
		ProcessMap2(CCX+r, CCY, r);

	}

	r = ctViewR1-1;
	for (int x=r; x>-r; x--)
	{
		ProcessMap(CCX+r, CCY+x, r);
		ProcessMap(CCX+x, CCY+r, r);
	}

	for (r=ctViewR1-2; r>0; r--)
	{
		for (int x=r; x>0; x--)
		{
			ProcessMap(CCX-x, CCY+r, r);
			ProcessMap(CCX+x, CCY+r, r);
			ProcessMap(CCX-x, CCY-r, r);
			ProcessMap(CCX+x, CCY-r, r);
		}

		ProcessMap(CCX, CCY-r, r);
		ProcessMap(CCX, CCY+r, r);

		for (int y=r-1; y>0; y--)
		{
		ProcessMap(CCX+r, CCY-y, r);
		ProcessMap(CCX+r, CCY+y, r);
		ProcessMap(CCX-r, CCY+y, r);
		ProcessMap(CCX-r, CCY-y, r);
		}
		ProcessMap(CCX-r, CCY, r);
		ProcessMap(CCX+r, CCY, r);
	}

	ProcessMap(CCX, CCY, 0);
}

void RenderWCircles()
{
	TWCircle *wptr;
	Vector3d rpos;

	for (int c=0; c<WCCount; c++)
	{
		wptr = &WCircles[c];
		rpos.x = wptr->pos.x - CameraX;
		rpos.y = wptr->pos.y - CameraY;
		rpos.z = wptr->pos.z - CameraZ;

		float r = (float)max( fabs(rpos.x), fabs(rpos.z) );
		int ri = -1 + (int)(r / 256.f + 0.4f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		rpos = RotateVector(rpos);

		if ( rpos.z > BackViewR) continue;
		if ( fabsf(rpos.x) > -rpos.z + BackViewR ) continue;
		if ( fabsf(rpos.y) > -rpos.z + BackViewR ) continue;

		GlassL = 255 - (2000-wptr->FTime) / 38;

		CreateMorphedModel(WCircleModel.mptr, &WCircleModel.Animation[0], (int)(wptr->FTime), wptr->scale);

		if ( fabsf(rpos.z) + fabs(rpos.x) < 1000)
		RenderModelClip(WCircleModel.mptr,rpos.x, rpos.y, rpos.z+1.5f, 250, 0, 0, CameraBeta);
		else
		RenderModel(WCircleModel.mptr,rpos.x, rpos.y, rpos.z+1.5f, 250, 0, 0, CameraBeta);
	}
	GlassL = 0;
}

void RenderWater()
{
	// == Global Function == //
	// -> Render the water that is visible (perhaps use a variable named "BOOL NEEDWATER")

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (int r=ctViewR; r>=ctViewR1; r-=2)
	{
		for (int x=r; x>0; x-=2)
		{
			ProcessMapW2(CCX-x, CCY+r, r);
			ProcessMapW2(CCX+x, CCY+r, r);
			ProcessMapW2(CCX-x, CCY-r, r);
			ProcessMapW2(CCX+x, CCY-r, r);
		}

		ProcessMapW2(CCX, CCY-r, r);
		ProcessMapW2(CCX, CCY+r, r);

		for (int y=r-2; y>0; y-=2)
		{
			ProcessMapW2(CCX+r, CCY-y, r);
			ProcessMapW2(CCX+r, CCY+y, r);
			ProcessMapW2(CCX-r, CCY+y, r);
			ProcessMapW2(CCX-r, CCY-y, r);
		}

		ProcessMapW2(CCX-r, CCY, r);
		ProcessMapW2(CCX+r, CCY, r);
	}

	for (int y=-ctViewR1+2; y<ctViewR1; y++)
    for (int x=-ctViewR1+2; x<ctViewR1; x++)
	{
		ProcessMapW(CCX+x, CCY+y, max(abs(x), abs(y)));
	}

	FogYBase = 0;
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	RenderWCircles();
	glDisable(GL_BLEND);
}

void RenderNearModel(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	//== Global Function == //
	// -> Render a 3d model up close/Not 3d space. used to render Binocs, wind compass, compass, etc
	// -> z0 simply changes the size of the object. x and y are 2d space locations on screen
	// -> al and bt are alpha and beta radians.

	BOOL b = LOWRESTX;
	Vector3d v;
	v.x = 0; v.y =-128; v.z = 0;

	CalcFogLevel_Gradient(v);
	FogYGrad = 0;

	LOWRESTX = FALSE;
	//GLStartBuffer();
	RenderModelClip(_mptr, x0, y0, z0, light, 0, al, bt);
	//GLEndBuffer();
	LOWRESTX = b;
}

void RenderCircle(float cx, float cy, float z, float _R, DWORD RGBA, DWORD RGBA2)
{
	// == HARDWARE 3D ONLY == //
	// -> Render a circle particle
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	z = -(_ZSCALE / z);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv((GLubyte*)&RGBA);

	glVertex4f(cx,cy,z,1.0f);

	for (int i=0; i<=16; i++)
	{
		//glColor4ub(0,0,0,0);
		glColor4ubv((GLubyte*)&RGBA2);

		float p = float(i/16.0f);
		float rx = sinf( ((360*p)*3.14f/180) ) *cosf(0)* (float)_R;
		float ry = cosf( ((360*p)*3.14f/180) ) *cosf(0)* (float)_R;
		glVertex4f((float)cx+rx,(float)cy+ry,z,1.f);
	}

	glEnd();

	glDisable(GL_BLEND);
}

void RenderElements()
{
	// == Global Function == //
	// -> Render the particles/decals (Snow, dirt, blood, etc)
	int fproc1 = 0;

	for (int eg = 0; eg<ElCount; eg++)
	{
		for (int e = 0; e<Elements[eg].ECount; e++)
		{
			Vector3d rpos;
			TElement *el = &Elements[eg].EList[e];
			rpos.x = el->pos.x - CameraX;
			rpos.y = el->pos.y - CameraY;
			rpos.z = el->pos.z - CameraZ;
			float r = el->R;

			rpos = RotateVector(rpos);
            if (rpos.z > -64) continue;
            if ( fabs(rpos.x) > -rpos.z) continue;
            if ( fabs(rpos.y) > -rpos.z) continue;

			float sx = VideoCXf - (CameraW * rpos.x / rpos.z);
			float sy = VideoCYf + (CameraH * rpos.y / rpos.z);

			RenderCircle(sx, sy, rpos.z, -r*CameraW*0.64f / rpos.z, Elements[eg].RGBA, Elements[eg].RGBA2);

			fproc1+=8;
			if (fproc1>256)
			{
				fproc1 = 0;
			}
		}

	}


	//if (fproc1) /*d3dFlushBuffer(fproc1, 0)*/;


    fproc1 = 0;
	for (int b=0; b<BloodTrail.Count; b++)
	{
		Vector3d rpos = BloodTrail.Trail[b].pos;
		DWORD A1 = (0xE0 * BloodTrail.Trail[b].LTime / 20000); if (A1>0xE0) A1=0xE0;
		DWORD A2 = (0x20 * BloodTrail.Trail[b].LTime / 20000); if (A2>0x20) A2=0x20;
        rpos.x = rpos.x - CameraX;
		rpos.y = rpos.y - CameraY;
	    rpos.z = rpos.z - CameraZ;

		rpos = RotateVector(rpos);
        if (rpos.z > -64) continue;
        if ( fabs(rpos.x) > -rpos.z) continue;
        if ( fabs(rpos.y) > -rpos.z) continue;

		//if (!fproc1) /*d3dStartBuffer()*/;

        float sx = VideoCXf - (CameraW * rpos.x / rpos.z);
	    float sy = VideoCYf + (CameraH * rpos.y / rpos.z);
		RenderCircle(sx, sy, rpos.z, -12*CameraW*0.64f / rpos.z, (A1<<24)+0x000070, (A2<<24)+0x000030);
		fproc1+=8;

		if (fproc1>256)
		{
			/*d3dFlushBuffer(fproc1, 0);*/
			fproc1 = 0;
		}
	}

	/*if (fproc1) d3dFlushBuffer(fproc1, 0);*/

    if (SNOW)
    for (int s=0; s<SnCount; s++)
	{
		Vector3d rpos = Snow[s].pos;


        rpos.x = rpos.x - CameraX;
		rpos.y = rpos.y - CameraY;
	    rpos.z = rpos.z - CameraZ;


		rpos = RotateVector(rpos);
        if (rpos.z > -64) continue;
        if ( fabs(rpos.x) > -rpos.z) continue;
        if ( fabs(rpos.y) > -rpos.z) continue;

        float sx = VideoCXf - CameraW * rpos.x / rpos.z;
	    float sy = VideoCYf + CameraH * rpos.y / rpos.z;

		DWORD A1 = 0xFF;
		DWORD A2 = 0x30;
		if (Snow[s].ftime) {
			A1 = A1 * (DWORD)(2000-Snow[s].ftime) / 2000;
			A2 = A2 * (DWORD)(2000-Snow[s].ftime) / 2000;
		}

        /*if (!fproc1) d3dStartBuffer();*/

		if (rpos.z>-1624)
		{
			RenderCircle(sx, sy, rpos.z, -8*CameraW*0.64f / rpos.z, (A1<<24)+conv_xGx(0xF0F0F0), (A2<<24)+conv_xGx(0xB0B0B0));
			fproc1+=8;
        }
		else
		{
			//RenderPoint(sx, sy, rpos.z, -8*CameraW*0.64f / rpos.z, (A1<<24)+conv_xGx(0xF0F0F0), (A2<<24)+conv_xGx(0xB0B0B0));
			fproc1++;
        }

		if (fproc1>256)
		{
			/*d3dFlushBuffer(fproc1, 0);*/
			fproc1 = 0;
		}
	}

    /*if (fproc1) d3dFlushBuffer(fproc1, 0);*/

    GlassL = 0;
}


void RenderCharacterPost(TCharacter *cptr)
{
	// -> Render the Characters (AI) in the game world
	CreateChMorphedModel(cptr);

	float zs = (float)sqrtf( cptr->rpos.x*cptr->rpos.x  +  cptr->rpos.y*cptr->rpos.y  +  cptr->rpos.z*cptr->rpos.z);
	if (zs > ctViewR*256) return;

	GlassL = 0;

	if (zs > 256 * (ctViewR-4)) GlassL = (int)min(255, (zs/4 - 64*(ctViewR-4)));

	waterclip = FALSE;


	if ( cptr->rpos.z >-256*10)
    RenderModelClip(cptr->pinfo->mptr,
					cptr->rpos.x,
					cptr->rpos.y,
					cptr->rpos.z,
					210,
					0,
					-cptr->alpha + pi / 2 + CameraAlpha,
					CameraBeta );
	else
    RenderModel(	cptr->pinfo->mptr,
					cptr->rpos.x,
					cptr->rpos.y,
					cptr->rpos.z,
					210,
					0,
					-cptr->alpha + pi / 2 + CameraAlpha,
					CameraBeta );


	if (!SHADOWS3D) return;
	if (zs > 256 * (ctViewR-8)) return;

	int Al = 0x60;

	if (cptr->Health==0)
	{
		int at = cptr->pinfo->Animation[cptr->Phase].AniTime;
		if (Tranq) return;
		if (cptr->FTime==at-1) return;
		Al = Al * (at-cptr->FTime) / at;  }
   		if (cptr->AI==0) Al = 0x50;

		GlassL = (Al<<24) + 0x00222222;
		/* -> OpenGl doesnt support shadows right now,
		   -> ToDo: Write Shadow Map functionality
		RenderShadowClip(	cptr->pinfo->mptr,
							cptr->pos.x,
							cptr->pos.y,
							cptr->pos.z,
							cptr->rpos.x,
							cptr->rpos.y,
							cptr->rpos.z,
							pi/2-cptr->alpha,
							CameraAlpha,
							CameraBeta );
		*/
}

void RenderShipPost()
{
	if (Ship.State==-1) return;
	GlassL = 0;
	zs = (int)VectorLength(Ship.rpos);
	if (zs > 256 * (ctViewR)) return;

	if (zs > 256 * (ctViewR-4))
	GlassL = min(255,(int)(zs - 256 * (ctViewR-4)) / 4);

	CreateMorphedModel(ShipModel.mptr, &ShipModel.Animation[0], Ship.FTime, 1.0);

	if ( fabs(Ship.rpos.z) < 4000)
	{
		RenderModelClip(ShipModel.mptr,Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 210, 0, -Ship.alpha -pi/2 + CameraAlpha, CameraBeta);
	}
	else
	{
		RenderModel(ShipModel.mptr,Ship.rpos.x, Ship.rpos.y, Ship.rpos.z, 210, 0, -Ship.alpha -pi/2+ CameraAlpha, CameraBeta);
	}
}

void RenderSupplyShipPost()
{

}

void RenderAmmoBag()
{

}


void Render3DHardwarePosts()
{
	// -> Render models, ships, and ammo bags
	TCharacter *cptr;
	for (int c=0; c<ChCount; c++)
	{
		cptr = &Characters[c];
		cptr->rpos.x = cptr->pos.x - CameraX;
		cptr->rpos.y = cptr->pos.y - CameraY;
		cptr->rpos.z = cptr->pos.z - CameraZ;


		float r = (float)max( fabs(cptr->rpos.x), fabs(cptr->rpos.z) );
		int ri = -1 + (int)(r / 256.f + 0.5f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		if (FOGON) CalcFogLevel_Gradient(cptr->rpos);

		cptr->rpos = RotateVector(cptr->rpos);

		float br = BackViewR + DinoInfo[cptr->CType].Radius;
		if (cptr->rpos.z > br) continue;
		if ( fabs(cptr->rpos.x) > -cptr->rpos.z + br ) continue;
		if ( fabs(cptr->rpos.y) > -cptr->rpos.z + br ) continue;

		RenderCharacterPost(cptr);
	}



   Ship.rpos.x = Ship.pos.x - CameraX;
   Ship.rpos.y = Ship.pos.y - CameraY;
   Ship.rpos.z = Ship.pos.z - CameraZ;
   float r = (float)max( fabs(Ship.rpos.x), fabs(Ship.rpos.z) );

   int ri = -1 + (int)(r / 256.f + 0.2f);
   if (ri < 0) ri = 0;
   if (ri < ctViewR) {
	  if (FOGON)
	   CalcFogLevel_Gradient(Ship.rpos);

      Ship.rpos = RotateVector(Ship.rpos);
      if (Ship.rpos.z > BackViewR) goto NOSHIP;
      if ( fabs(Ship.rpos.x) > -Ship.rpos.z + BackViewR ) goto NOSHIP;

      RenderShipPost();
   }
NOSHIP: ;


   if (!SupplyShip.State) goto NOSHIP2;
   SupplyShip.rpos.x = SupplyShip.pos.x - CameraX;
   SupplyShip.rpos.y = SupplyShip.pos.y - CameraY;
   SupplyShip.rpos.z = SupplyShip.pos.z - CameraZ;
   r = (float)max( fabs(SupplyShip.rpos.x), fabs(SupplyShip.rpos.z) );

   ri = -1 + (int)(r / 256.f + 0.2f);
   if (ri < 0) ri = 0;
   if (ri < ctViewR) {
	  if (FOGON)
	   CalcFogLevel_Gradient(SupplyShip.rpos);

      SupplyShip.rpos = RotateVector(SupplyShip.rpos);
      if (SupplyShip.rpos.z > BackViewR) goto NOSHIP2;
      if ( fabs(SupplyShip.rpos.x) > -SupplyShip.rpos.z + BackViewR ) goto NOSHIP2;

      RenderSupplyShipPost();
   }
NOSHIP2: ;

   if (!AmmoBag.State) goto NOBAG;
   AmmoBag.rpos.x = AmmoBag.pos.x - CameraX;
   AmmoBag.rpos.y = AmmoBag.pos.y - CameraY;
   AmmoBag.rpos.z = AmmoBag.pos.z - CameraZ;
   r = (float)max( fabs(AmmoBag.rpos.x), fabs(AmmoBag.rpos.z) );

   ri = -1 + (int)(r / 256.f + 0.2f);
   if (ri < 0) ri = 0;
   if (ri < ctViewR) {
	  if (FOGON)
	   CalcFogLevel_Gradient(AmmoBag.rpos);

      AmmoBag.rpos = RotateVector(AmmoBag.rpos);
      if (AmmoBag.rpos.z > BackViewR) goto NOBAG;
      if ( fabs(AmmoBag.rpos.x) > -AmmoBag.rpos.z + BackViewR ) goto NOBAG;

      RenderAmmoBag();
   }
NOBAG: ;

   SunLight *= GetTraceK(SunScrX, SunScrY);
}

void RotateVVector(Vector3d& v)
{
   float x = v.x * ca - v.z * sa;
   float y = v.y;
   float z = v.z * ca + v.x * sa;

   float xx = x;
   float xy = y * cb + z * sb;
   float xz = z * cb - y * sb;

   v.x = xx; v.y = xy; v.z = xz;
}

void RenderSun(float x,float y,float z)
{
	SunScrX = VideoCX + (int)(x / (-z) * CameraW);
    SunScrY = VideoCY - (int)(y / (-z) * CameraH);
	GetSkyK(SunScrX, SunScrY);

	float d = (float)sqrt(x*x + y*y);
	if (d<2048) {
		SunLight = (220.f- d*220.f/2048.f);
		if (SunLight>140) SunLight = 140;
		SunLight*=SkyTraceK;
	}


	if (d>812.f) d = 812.f;
	d = (2048.f + d) / 3048.f;
	d+=(1.f-SkyTraceK)/2.f;
	if (OptDayNight==2)  d=1.5;
    //RenderModelSun(SunModel,  x*d, y*d, z*d, (int)(200.f* SkyTraceK));
}

void RenderSkyPlane(void)
{
	glBindTexture(GL_TEXTURE_2D, pTextures[2043]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Vector3d v,vbase;
   Vector3d tx,ty,nv;
   float p,q, qx, qy, qz, px, py, pz, rx, ry, rz, ddx, ddy;
   float lastdt = 0.f;

	nv.x = 512; nv.y = 4024; nv.z=0;

	cb = (float)cos(CameraBeta);
	sb = (float)sin(CameraBeta);
	SKYDTime = RealTime & ((1<<17) - 1);

	float sh = - CameraY;

	if (MapMinY==10241024) MapMinY=0;
	sh = (float)((int)MapMinY)*ctHScale - CameraY;

	if (sh<-2024) sh=-2024;

	v.x = 0;
	v.z = ctViewR*256.f;
	v.y = sh;

	vbase.x = v.x;
	vbase.y = v.y * cb + v.z * sb;
	vbase.z = v.z * cb - v.y * sb;
	if (vbase.z < 128) vbase.z = 128;
	int scry = VideoCY - (int)(vbase.y / vbase.z * CameraH);

	if (scry<0) return;
	if (scry>WinEY+1) scry = WinEY+1;

	cb = (float)cos(CameraBeta-0.15);
	sb = (float)sin(CameraBeta-0.15);

	v.x = 0;
	v.z = 2*256.f*256.f;
	v.y = 512;
	vbase.x = v.x;
	vbase.y = v.y * cb + v.z * sb;
	vbase.z = v.z * cb - v.y * sb;
	if (vbase.z < 128) vbase.z = 128;
	int _scry = VideoCY - (int)(vbase.y / vbase.z * CameraH);
	if (scry > _scry) scry = _scry;


	tx.x=0.002f;  tx.y=0;     tx.z=0;
	ty.x=0.0f;    ty.y=0;     ty.z=0.002f;
	nv.x=0;       nv.y=-1.f;  nv.z=0;

	RotateVVector(tx);
	RotateVVector(ty);
	RotateVVector(nv);

	sh = 4*512*16;
	vbase.x = -CameraX;
	vbase.y = sh;
	vbase.z = +CameraZ;
	RotateVVector(vbase);

//============= calc render params =================//
	p = nv.x * vbase.x + nv.y * vbase.y + nv.z * vbase.z;
	ddx = vbase.x * tx.x  +  vbase.y * tx.y  +  vbase.z * tx.z;
	ddy = vbase.x * ty.x  +  vbase.y * ty.y  +  vbase.z * ty.z;

	qx = CameraH * nv.x;   qy = CameraW * nv.y;   qz = CameraW*CameraH  * nv.z;
	px = p*CameraH*tx.x;   py = p*CameraW*tx.y;   pz = p*CameraW*CameraH* tx.z;
	rx = p*CameraH*ty.x;   ry = p*CameraW*ty.y;   rz = p*CameraW*CameraH* ty.z;

	px=px - ddx*qx;  py=py - ddx*qy;   pz=pz - ddx*qz;
	rx=rx - ddy*qx;  ry=ry - ddy*qy;   rz=rz - ddy*qz;

	float za = CameraW * CameraH * p / (qy * VideoCY + qz);
	float zb = CameraW * CameraH * p / (qy * (VideoCY-scry/2.f) + qz);
	float zc = CameraW * CameraH * p / (qy * (VideoCY-scry) + qz);

	float _za = fabsf(za) - 100200.f; if (_za<0) _za=0.f;
	float _zb = fabsf(zb) - 100200.f; if (_zb<0) _zb=0.f;
	float _zc = fabsf(zc) - 100200.f; if (_zc<0) _zc=0.f;

	int alpha = (int)(255*40240 / (40240+_za));
	int alphb = (int)(255*40240 / (40240+_zb));
	int alphc = (int)(255*40240 / (40240+_zc));

	int sx1 = - VideoCX;
	int sx2 = + VideoCX;

	float qx1 = qx * sx1 + qz;
	float qx2 = qx * sx2 + qz;
	float qyy;

	float dtt = (float)(SKYDTime) / 512.f;

    float sky=0;
	float sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	float fxa = (px * sx1 + py * sy + pz) / q;
	float fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	float fxb = (px * sx2 + py * sy + pz) / q;
	float fyb = (rx * sx2 + ry * sy + rz) / q;

	gvtx[0].x       = 0.f;
    gvtx[0].y       = (float)sky;
    gvtx[0].z       = 0.0f;//-8.f / za;
    gvtx[0].rhw      = 1;//128.f / za;
	gvtx[0].color    = 0xFFFFFFFF;// | alpha<<24;
    gvtx[0].tu       = (fxa + dtt) / 256.f;//(fxa + dtt) / 256.f;
    gvtx[0].tv       = (fya - dtt) / 256.f;//(fya - dtt) / 256.f;

	gvtx[1].x       = (float)WinW;
    gvtx[1].y       = (float)sky;
    gvtx[1].z       = 0.0f;//-8.f / za;
    gvtx[1].rhw      = 1;//128.f / za;
	gvtx[1].color    = 0xFFFFFFFF;// | alpha<<24;
    gvtx[1].tu       = (fxb + dtt) / 256.f;//(fxb + dtt) / 256.f;
    gvtx[1].tv       = (fyb - dtt) / 256.f;//(fyb - dtt) / 256.f;


	sky=scry/2.f;
	sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	fxa = (px * sx1 + py * sy + pz) / q;
	fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	fxb = (px * sx2 + py * sy + pz) / q;
	fyb = (rx * sx2 + ry * sy + rz) / q;

	gvtx[2].x       = 0.f;
    gvtx[2].y       = (float)sky;
    gvtx[2].z       = 0.0f;//-8.f / zb;
    gvtx[2].rhw      = 1;//128.f / zb;
	gvtx[2].color    = 0xFFFFFFFF;// | alphb<<24;
	gvtx[2].tu       = (fxa + dtt) / 256.f;//(fxa + dtt) / 256.f;
    gvtx[2].tv       = (fya - dtt) / 256.f;//(fya - dtt) / 256.f;

	gvtx[3].x       = (float)WinW;
    gvtx[3].y       = (float)sky;
    gvtx[3].z       = 0.0f;//-8.f / zb;
    gvtx[3].rhw      = 1;//128.f / zb;
	gvtx[3].color    = 0x00FFFFFF | alphb<<24;
	gvtx[3].tu       = (fxb + dtt) / 256.f;//(fxb + dtt) / 256.f;
    gvtx[3].tv       = (fyb - dtt) / 256.f;


	sky=(float)scry;
	sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	fxa = (px * sx1 + py * sy + pz) / q;
	fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	fxb = (px * sx2 + py * sy + pz) / q;
	fyb = (rx * sx2 + ry * sy + rz) / q;

	gvtx[4].x       = 0.0f;
    gvtx[4].y       = (float)sky;
    gvtx[4].z       = 0.0f;//-8.f / zb;
    gvtx[4].rhw      = 1;//128.f / zc;
	gvtx[4].color    = 0xFFFFFFFF;// | alphc<<24;
    gvtx[4].tu       = (fxa + dtt) / 256.f;//(fxa + dtt) / 256.f;
    gvtx[4].tv       = (fya - dtt) / 256.f;//(fya - dtt) / 256.f;

	gvtx[5].x       = (float)WinW;
    gvtx[5].y       = (float)sky;
    gvtx[5].z       = 0.0f;
    gvtx[5].rhw      = 1;
	gvtx[5].color    = 0x80FFFFFF;// | alphc<<24;
	gvtx[5].tu       = (fxb + dtt) / 256.f;
    gvtx[5].tv       = (fyb - dtt) / 256.f;//(fyb - dtt) / 256.f;

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
	GLDrawTriangle(gvtx[1],gvtx[2],gvtx[3]);
	GLDrawTriangle(gvtx[2],gvtx[3],gvtx[4]);
	GLDrawTriangle(gvtx[3],gvtx[4],gvtx[5]);

	glDisable(GL_BLEND);

	nv = RotateVector(Sun3dPos);
	SunLight = 0;
	if (nv.z < -2024) RenderSun(nv.x, nv.y, nv.z);
}

void Render_Cross(int x, int y)
{
	// -> Render a crosshair on the screen
}

void Render_Text(int x, int y,char *text, unsigned long color)
{
	// -> Render text
	GLTextOut(x,y,text,color);
}

void Render_LifeInfo(int li)
{
	// -> Render the targeted animal's info
	int x,y;
	int ctype = Characters[li].CType;
	char t[32];

    x = VideoCX + WinW / 64;
	y = VideoCY + (int)(WinH / 6.8);

    GLTextOut(x, y, DinoInfo[ctype].Name, 0xFF00b000);

	if (OptSys) sprintf(t,"Weight: %3.2ft ", DinoInfo[ctype].Mass * Characters[li].scale * Characters[li].scale / 0.907);
	else        sprintf(t,"Weight: %3.2fT ", DinoInfo[ctype].Mass * Characters[li].scale * Characters[li].scale);

	GLTextOut(x, y+16, t, 0xFF00b000);

	int R  = (int)(VectorLength( SubVectors(Characters[li].pos, PlayerPos) )*3 / 64.f);
	if (OptSys) sprintf(t,"Distance: %dft ", R);
	else        sprintf(t,"Distance: %dm  ", R/3);

	GLTextOut(x, y+32, t, 0xFF00b000);
}

void DrawTrophyText(int x0, int y0)
{
	int x;
	int tc = TrophyBody;

	int   dtype = TrophyRoom.Body[tc].ctype;
	int   time  = TrophyRoom.Body[tc].time;
	int   date  = TrophyRoom.Body[tc].date;
	int   wep   = TrophyRoom.Body[tc].weapon;
	int   score = TrophyRoom.Body[tc].score;
	float scale = TrophyRoom.Body[tc].scale;
	float range = TrophyRoom.Body[tc].range;
	char t[32];

	x0+=14; y0+=18;
    x = x0;
	GLTextOut(x, y0   , "Name: ", 0x00BFBFBF);  x+=GetTextW(DeviceContext,"Name: ");
	if (CHEATED)
	{
		GLTextOut(x, y0   , "[CHEATED] ", 0x000000F0);  x+=GetTextW(DeviceContext,"[CHEATED] ");
	}
    GLTextOut(x, y0   , DinoInfo[dtype].Name, 0x0000BFBF);

	x = x0;
	GLTextOut(x, y0+16, "Weight: ", 0x00BFBFBF);  x+=GetTextW(DeviceContext,"Weight: ");

	if (OptSys)	sprintf(t,"%3.2ft ", DinoInfo[dtype].Mass * scale * scale / 0.907);
	else		sprintf(t,"%3.2fT ", DinoInfo[dtype].Mass * scale * scale);

    GLTextOut(x, y0+16, t, 0x0000BFBF);    x+=GetTextW(DeviceContext,t);
    GLTextOut(x, y0+16, "Length: ", 0x00BFBFBF); x+=GetTextW(DeviceContext,"Length: ");

	if (OptSys) sprintf(t,"%3.2fft", DinoInfo[dtype].Length * scale / 0.3);
	else		sprintf(t,"%3.2fm", DinoInfo[dtype].Length * scale);

	GLTextOut(x, y0+16, t, 0x0000BFBF);

	x = x0;
	GLTextOut(x, y0+32, "Weapon: ", 0x00BFBFBF);  x+=GetTextW(DeviceContext,"Weapon: ");

	sprintf(t,"%s    ", WeapInfo[wep].Name);
	GLTextOut(x, y0+32, t, 0x0000BFBF);   x+=GetTextW(hdcMain,t);
    GLTextOut(x, y0+32, "Score: ", 0x00BFBFBF);   x+=GetTextW(DeviceContext,"Score: ");

	sprintf(t,"%d", score);
	GLTextOut(x, y0+32, t, 0x0000BFBF);


	x = x0;
	GLTextOut(x, y0+48, "Range of kill: ", 0x00BFBFBF);  x+=GetTextW(DeviceContext,"Range of kill: ");
	if (OptSys) sprintf(t,"%3.1fft", range / 0.3);
	else        sprintf(t,"%3.1fm", range);
    GLTextOut(x, y0+48, t, 0x0000BFBF);


	x = x0;
	GLTextOut(x, y0+64, "Date: ", 0x00BFBFBF);  x+=GetTextW(DeviceContext,"Date: ");

	if (OptSys)	sprintf(t,"%d.%d.%d   ", ((date>>10) & 255), (date & 255), date>>20);
	else		sprintf(t,"%d.%d.%d   ", (date & 255), ((date>>10) & 255), date>>20);

    GLTextOut(x, y0+64, t, 0x0000BFBF);   x+=GetTextW(DeviceContext,t);
    GLTextOut(x, y0+64, "Time: ", 0x00BFBFBF);   x+=GetTextW(DeviceContext,"Time: ");
	sprintf(t,"%d:%02d", ((time>>10) & 255), (time & 255));
	GLTextOut(x, y0+64, t, 0x0000BFBF);
}

void RenderModelClipEnvMap(TModel* _mptr, float x0, float y0, float z0, float al, float bt)
{
   int f,CMASK;
   mptr = _mptr;

   float ca = (float)cos(al);
   float sa = (float)sin(al);
   float cb = (float)cos(bt);
   float sb = (float)sin(bt);

   DWORD PHCOLOR = 0xFFFFFFFF;

   BOOL BL = FALSE;


	for (int s=0; s<mptr->VCount; s++)
	{

	rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
    float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
    rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
    rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
    if (rVertex[s].z<0) BL=TRUE;

    if (rVertex[s].z>-256) { gScrp[s].x = 0xFFFFFF; gScrp[s].y = 0xFF; }
    else {
     int f = 0;
     int sx =  VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
     int sy =  VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);

     if (sx>=WinEX) f+=1;
     if (sx<=0    ) f+=2;

     if (sy>=WinEY) f+=4;
     if (sy<=0    ) f+=8;

     gScrp[s].y = f;
    }

   }


   //d3dSetTexture(TFX_ENVMAP.lpImage, TFX_ENVMAP.W, TFX_ENVMAP.W);
	glBindTexture(GL_TEXTURE_2D,pTextures[TFX_ENVMAP.Index]);
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	BuildTreeClipNoSort();

	f = Current;
	int fproc1 = 0;

	while( f!=-1 )
	{

		vused = 3;
		TFace *fptr = &mptr->gFace[f];
		if (!(fptr->Flags & sfEnvMap)) goto LNEXT;


		CMASK = 0;

		CMASK|=gScrp[fptr->v1].y;
		CMASK|=gScrp[fptr->v2].y;
		CMASK|=gScrp[fptr->v3].y;


		cp[0].ev.v = rVertex[fptr->v1]; cp[0].tx = PhongMapping[fptr->v1].x/256.f;  cp[0].ty = PhongMapping[fptr->v1].y/256.f;
		cp[1].ev.v = rVertex[fptr->v2]; cp[1].tx = PhongMapping[fptr->v2].x/256.f;  cp[1].ty = PhongMapping[fptr->v2].y/256.f;
		cp[2].ev.v = rVertex[fptr->v3]; cp[2].tx = PhongMapping[fptr->v3].x/256.f;  cp[2].ty = PhongMapping[fptr->v3].y/256.f;



		{
			for (u=0; u<vused; u++) cp[u].ev.v.z+= 8.0f;
			for (u=0; u<vused; u++) ClipVector(ClipZ,u);
			for (u=0; u<vused; u++) cp[u].ev.v.z-= 8.0f;
			if (vused<3) goto LNEXT;
		}

		if (CMASK & 1) for (u=0; u<vused; u++) ClipVector(ClipA,u); if (vused<3) goto LNEXT;
		if (CMASK & 2) for (u=0; u<vused; u++) ClipVector(ClipC,u); if (vused<3) goto LNEXT;
		if (CMASK & 4) for (u=0; u<vused; u++) ClipVector(ClipB,u); if (vused<3) goto LNEXT;
		if (CMASK & 8) for (u=0; u<vused; u++) ClipVector(ClipD,u); if (vused<3) goto LNEXT;

		for (u=0; u<vused-2; u++)
		{
			int v=0;
	   		 gvtx[v].x       = VideoCXf - cp[0].ev.v.x / cp[0].ev.v.z * CameraW;
			 gvtx[v].y       = VideoCYf + cp[0].ev.v.y / cp[0].ev.v.z * CameraH;
			 gvtx[v].z       = -(_ZSCALE / cp[0].ev.v.z);
			 gvtx[v].rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			 gvtx[v].color    = PHCOLOR;
			 gvtx[v].tu       = (float)(cp[0].tx);
			 gvtx[v].tv       = (float)(cp[0].ty);
			 v++;

	   		 gvtx[v].x       = VideoCXf - cp[u+1].ev.v.x / cp[u+1].ev.v.z * CameraW;
			 gvtx[v].y       = VideoCYf + cp[u+1].ev.v.y / cp[u+1].ev.v.z * CameraH;
			 gvtx[v].z       = -(_ZSCALE / cp[u+1].ev.v.z);
			 gvtx[v].rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			 gvtx[v].color    = PHCOLOR;
			 gvtx[v].tu       = (float)(cp[u+1].tx);
			 gvtx[v].tv       = (float)(cp[u+1].ty);
			 v++;

	   		gvtx[v].x       = VideoCXf - cp[u+2].ev.v.x / cp[u+2].ev.v.z * CameraW;
			gvtx[v].y       = VideoCYf + cp[u+2].ev.v.y / cp[u+2].ev.v.z * CameraH;
			gvtx[v].z       = -(_ZSCALE / cp[u+2].ev.v.z);
			gvtx[v].rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			gvtx[v].color    = PHCOLOR;
			gvtx[v].tu       = (float)(cp[u+2].tx);
			gvtx[v].tv       = (float)(cp[u+2].ty);
			v++;

			GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

			fproc1++;
		}
LNEXT:
		f = mptr->gFace[f].Next;
		if (fproc1>256)
		{
			fproc1 = 0;
		}
	}

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void RenderModelClipPhongMap(TModel* _mptr, float x0, float y0, float z0, float al, float bt)
{
   int f,CMASK;
   mptr = _mptr;

   float ca = (float)cos(al);
   float sa = (float)sin(al);
   float cb = (float)cos(bt);
   float sb = (float)sin(bt);

   int   rv = SkyR +64; if (rv>255) rv = 255;
   int   gv = SkyG +64; if (gv>255) gv = 255;
   int   bv = SkyB +64; if (bv>255) bv = 255;
   DWORD PHCOLOR = 0xFF000000 + (rv<<16) + (gv<<8) + bv;

   BOOL BL = FALSE;

   for (int s=0; s<mptr->VCount; s++) {

	rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
    float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
    rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
    rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
    if (rVertex[s].z<0) BL=TRUE;

    if (rVertex[s].z>-256) { gScrp[s].x = 0xFFFFFF; gScrp[s].y = 0xFF; }
    else {
     int f = 0;
     int sx =  VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
     int sy =  VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);

     if (sx>=WinEX) f+=1;
     if (sx<=0    ) f+=2;

     if (sy>=WinEY) f+=4;
     if (sy<=0    ) f+=8;

     gScrp[s].y = f;
    }

   }

	glBindTexture(GL_TEXTURE_2D,pTextures[TFX_SPECULAR.Index]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	BuildTreeClipNoSort();

	f = Current;
	int fproc1 = 0;


	while( f!=-1 )
	{
		vused = 3;
		TFace *fptr = &mptr->gFace[f];
		if (!(fptr->Flags & sfPhong)) goto LNEXT;

		CMASK = 0;

		CMASK|=gScrp[fptr->v1].y;
		CMASK|=gScrp[fptr->v2].y;
		CMASK|=gScrp[fptr->v3].y;

		cp[0].ev.v = rVertex[fptr->v1]; cp[0].tx = PhongMapping[fptr->v1].x/256.f;  cp[0].ty = PhongMapping[fptr->v1].y/256.f;
		cp[1].ev.v = rVertex[fptr->v2]; cp[1].tx = PhongMapping[fptr->v2].x/256.f;  cp[1].ty = PhongMapping[fptr->v2].y/256.f;
		cp[2].ev.v = rVertex[fptr->v3]; cp[2].tx = PhongMapping[fptr->v3].x/256.f;  cp[2].ty = PhongMapping[fptr->v3].y/256.f;

		{
			for (u=0; u<vused; u++) cp[u].ev.v.z+= 8.0f;
			for (u=0; u<vused; u++) ClipVector(ClipZ,u);
			for (u=0; u<vused; u++) cp[u].ev.v.z-= 8.0f;
			if (vused<3) goto LNEXT;
		}

		if (CMASK & 1) for (u=0; u<vused; u++) ClipVector(ClipA,u); if (vused<3) goto LNEXT;
		if (CMASK & 2) for (u=0; u<vused; u++) ClipVector(ClipC,u); if (vused<3) goto LNEXT;
		if (CMASK & 4) for (u=0; u<vused; u++) ClipVector(ClipB,u); if (vused<3) goto LNEXT;
		if (CMASK & 8) for (u=0; u<vused; u++) ClipVector(ClipD,u); if (vused<3) goto LNEXT;

		for (u=0; u<vused-2; u++)
		{
			int v=0;
	   	 gvtx[v].x       = VideoCXf - cp[0].ev.v.x / cp[0].ev.v.z * CameraW;
         gvtx[v].y       = VideoCYf + cp[0].ev.v.y / cp[0].ev.v.z * CameraH;
         gvtx[v].z       = -(_ZSCALE / cp[0].ev.v.z);
         gvtx[v].rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
         gvtx[v].color    = PHCOLOR;
         gvtx[v].tu       = (float)(cp[0].tx);
         gvtx[v].tv       = (float)(cp[0].ty);
         v++;

	   	 gvtx[v].x       = VideoCXf - cp[u+1].ev.v.x / cp[u+1].ev.v.z * CameraW;
         gvtx[v].y       = VideoCYf + cp[u+1].ev.v.y / cp[u+1].ev.v.z * CameraH;
         gvtx[v].z       = -(_ZSCALE / cp[u+1].ev.v.z);
         gvtx[v].rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
         gvtx[v].color    = PHCOLOR;
         gvtx[v].tu       = (float)(cp[u+1].tx);
         gvtx[v].tv       = (float)(cp[u+1].ty);
         v++;

	   	 gvtx[v].x       = VideoCX - cp[u+2].ev.v.x / cp[u+2].ev.v.z * CameraW;
         gvtx[v].y       = VideoCY + cp[u+2].ev.v.y / cp[u+2].ev.v.z * CameraH;
         gvtx[v].z       = -(_ZSCALE / cp[u+2].ev.v.z);
         gvtx[v].rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
         gvtx[v].color    = PHCOLOR;
		 gvtx[v].tu       = (float)(cp[u+2].tx);
         gvtx[v].tv       = (float)(cp[u+2].ty);
         v++;

			GLDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

			fproc1++;
		}
LNEXT:
		f = mptr->gFace[f].Next;
		if (fproc1>256)
		{
			fproc1 = 0;
		}
	}

	glDisable(GL_BLEND);
}

void CopyHARDToDIB(void)
{
	// -> Copy the buffer to a BMP canvas
	if (WinW>1024) return;
	glReadBuffer(GL_FRONT);
	glReadPixels(0,0,WinW,WinH,GL_BGR,GL_UNSIGNED_SHORT_5_6_5_REV,lpVideoBuf);
	glReadBuffer(GL_BACK);
}

void DrawHMap()
{
	// -> Render the player map
	int xx,yy;

	// == Draw the map picture
	DrawPicture(VideoCX-MapPic.W/2, VideoCY - MapPic.H/2-6, MapPic);

	glDepthFunc(GL_ALWAYS);

	// == Draw the player
	xx = VideoCX - 128 + (CCX>>2);
	yy = VideoCY - 128 + (CCY>>2);
	if (yy<0 || yy>=WinH) goto endmap;
	if (xx<0 || xx>=WinW) goto endmap;
	GLDrawRectangle(xx-1,yy-1,xx+1,yy+1,0xFF00FF00);
	GLDrawCircle(int(xx), int(yy), int(ctViewR / 4.0f), 0xFF00FF00);
	//GLDrawLine(xx,yy,xx+R,yy+R,0xFF00FF00);

	// == Draw the huntable creatures
	if (RadarMode)
	for (int c=0; c<ChCount; c++)
	{
		if (Characters[c].AI<10) continue;
		if (! (TargetDino & (1<<Characters[c].AI)) ) continue;
		if (!Characters[c].Health) continue;
		xx = VideoCX - 128 + (int)Characters[c].pos.x / 1024;
		yy = VideoCY - 128 + (int)Characters[c].pos.z / 1024;
		if (yy<=0 || yy>=WinH) goto endmap;
		if (xx<=0 || xx>=WinW) goto endmap;
		GLDrawRectangle(xx-1,yy-1, xx+1, yy+1, 0xFF0000FF);
	}

	// == Draw the dropship
	xx = VideoCX - 128 + (int)Ship.pos.x / 1024;
	yy = VideoCY - 128 + (int)Ship.pos.z / 1024;
	if (xx>=1 && yy>=1)
	{
		GLDrawRectangle(xx-1,yy-1, xx+1, yy+1, 0xFF0080FF);
	}

	glDepthFunc(GL_LEQUAL);

endmap: return;
}


void Hardware_ZBuffer(BOOL bl)
{
	// == Global Function == //
	// -> Toggle zbuffer
	// -> Used by DrawPostObjects so that all objects rendered appear close?
	if (!bl)
	{
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
	}
	else
	{
		glDepthFunc(GL_LEQUAL);
	}
}

float GetSkyK(int x, int y)
{
	return 1.0f;
}

float GetTraceK(int x, int y)
{
	return 1.0f;
}


#endif //_gl
