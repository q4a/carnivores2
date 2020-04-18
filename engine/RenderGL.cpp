
// -- Base libraries
#include "IniFile.h"
#include "gl_extra.h"
#include "Platform.h"
#include <stdio.h>

// -- STL
#include <vector>
using namespace std;


/* NOTE: Put any OpenGL todo items here
TODO: [OGL] Remove all deprecated glBegin/glEnd/glVertex/glColor/glTexcoord function calls
TODO: [OGL] Optimise states, create a state variable cache (struct gl_cache_s{};)
TODO: [OGL] Rewrite rendering system and terrain caching from scratch, use GL Shaders
*/


///////////////////////////////////////////////////////////////////
// Methods

void TPicture::Allocate()
{
	if ( m_data ) return;
	if ( m_texid ) return;

	m_data = malloc( m_width * m_height * ( m_bpp/8 ) );
	glGenTextures( 1, &m_texid );

	if ( m_texid == 0 )
	DoHalt( "TPicture::Allocate()\n\tFailed to create a GL texture!\n" );
}

void TPicture::Release()
{
	if ( m_data ) free( m_data );
	m_data = 0;
	if ( m_texid ) glDeleteTextures( 1, &m_texid );
	m_texid = 0;
}

void CTexture::Allocate()
{
	if ( m_data ) return;
	if ( m_texid ) return;

	m_data = malloc( m_width * m_height * ( m_bpp/8 ) );
	glGenTextures( 1, &m_texid );

	if ( m_texid == 0 )
	DoHalt( "CTexture::Allocate()\n\tFailed to create a GL texture!\n" );
}

void CTexture::Release()
{
	if ( m_data ) free( m_data );
	m_data = 0;
	if ( m_texid ) glDeleteTextures( 1, &m_texid );
	m_texid = 0;
}



uint32_t ABGR_to_ARGB( uint32_t c )
{
	int B = ( (c>>0 ) & 255 );
	int G = ( (c>>8 ) & 255 );
	int R = ( (c>>16) & 255 );
	int A = ( (c>>24) & 255 );
	return (A<<24) | (B<<16) | (G<<8) | (R<<0);
}

uint32_t Color_ARGB( BYTE R, BYTE G, BYTE B, BYTE A )
{
	return (A<<24) | (B<<16) | (G<<8) | (R<<0);
}


// == Definitions == //

//#define ZSCALE		-32767.0f
#define ZSCALE		-65535.0f
#define _AZSCALE	(1.f /  ZSCALE);
#define ShowErrorMessage( text ) MessageBox( hwndMain, text, "OpenGL Error", MB_OK|MB_ICONWARNING|MB_SYSTEMMODAL )

// == Types & Structures == //


// == Global Variables == //
VideoOptions* VidOpt = 0;
// Get rid of this:
int			MyPrevHealth = MAX_HEALTH;
uint32_t		TerrainTime,WaterTime,CharacterTime,ObjectTime;


float		SkyTrans = 0.0f;
bool		NEEDWATER; // Does the player see water?
bool		VBENABLE;

#define		MAX_RENDER_OBJECTS	2048
vec2i	ORList[ MAX_RENDER_OBJECTS ]; // List of objects that the player can see
int			ORLCount = 0;
int			zs =0;

// Useless:
uint32_t		Counter = 0;
float		SunLight;
float		TraceK,SkyTraceK,FogYGrad,FogYBase;;
int			SunScrX, SunScrY;
int			SkySumR, SkySumG, SkySumB;
int			LowHardMemory;
int			lsw;
float		vFogT[1024];
char		gLog[512];
GLint		gGLSkyTexture = 0;
GLint		glDefaultTexture = 0;


// == Function Declarations == //
void ClipVector(CLIPPLANE& C, int vn);
int BuildTreeClipNoSort();
void BuildTreeNoSortf();
void RenderObject(int x, int y);
float GetSkyK(int x, int y);
float GetTraceK(int x, int y);
void RenderShadowClip(TModel* _mptr, float xm0, float ym0, float zm0, float x0, float y0, float z0, float cal, float al, float bt);

//=========================================================================//
// Internal Source (RenderGL.cpp)
//=========================================================================//
uint32_t DarkRGBX(uint32_t c)
{
    BYTE R = ((c>>0 ) & 255) / 3;
    BYTE G = ((c>>8 ) & 255) / 3;
    BYTE B = ((c>>16) & 255) / 3;
    return (R) + (G<<8) + (B<<16);
}


//======================================================================
// Clip the vertex to screen co-ordinates
void ClipVector(CLIPPLANE& C, int vn)
{
    int ClipRes = 0;
    float s=0,s1=0,s2=0;
    int vleft  = (vn-1); if (vleft <0) vleft=vused-1;
    int vright = (vn+1); if (vright>=vused) vright=0;

    MulVectorsScal(cp[vn].ev.v, C.nv, s); //s=SGN(s-0.01f);
    if (s>=0) return;

    MulVectorsScal(cp[vleft ].ev.v, C.nv, s1);  //s1=SGN(s1+0.01f);  //s1+=0.0001f;
    MulVectorsScal(cp[vright].ev.v, C.nv, s2);  //s2=SGN(s2+0.01f);  //s2+=0.0001f;

    if (s1>0)
    {
        ClipRes+=1;
        float lc = -s / (s1-s);
        hleft.ev.v.x = cp[vn].ev.v.x + ((cp[vleft].ev.v.x - cp[vn].ev.v.x) * lc);
        hleft.ev.v.y = cp[vn].ev.v.y + ((cp[vleft].ev.v.y - cp[vn].ev.v.y) * lc);
        hleft.ev.v.z = cp[vn].ev.v.z + ((cp[vleft].ev.v.z - cp[vn].ev.v.z) * lc);

        hleft.tx = cp[vn].tx + ((cp[vleft].tx - cp[vn].tx) * (lc));
        hleft.ty = cp[vn].ty + ((cp[vleft].ty - cp[vn].ty) * (lc));
        hleft.ev.Light = cp[vn].ev.Light + (int)((cp[vleft].ev.Light - cp[vn].ev.Light) * lc);
        if ( hleft.ev.Light > 192 ) hleft.ev.Light = 192;
        hleft.ev.ALPHA = cp[vn].ev.ALPHA + (int)((cp[vleft].ev.ALPHA - cp[vn].ev.ALPHA) * lc);
        hleft.ev.Fog   = cp[vn].ev.Fog   +      ((cp[vleft].ev.Fog   - cp[vn].ev.Fog  ) * lc);
    }

    if (s2>0)
    {
        ClipRes+=2;

        float lc = -s / (s2-s);
		hright.ev.v.x = cp[vn].ev.v.x + ((cp[vright].ev.v.x - cp[vn].ev.v.x) * lc);
        hright.ev.v.y = cp[vn].ev.v.y + ((cp[vright].ev.v.y - cp[vn].ev.v.y) * lc);
        hright.ev.v.z = cp[vn].ev.v.z + ((cp[vright].ev.v.z - cp[vn].ev.v.z) * lc);

        hright.tx = cp[vn].tx + ((cp[vright].tx - cp[vn].tx) * lc);
        hright.ty = cp[vn].ty + ((cp[vright].ty - cp[vn].ty) * lc);
        hright.ev.Light = cp[vn].ev.Light + (int)((cp[vright].ev.Light - cp[vn].ev.Light) * lc);
        if ( hright.ev.Light > 192 ) hright.ev.Light = 192;
        hright.ev.ALPHA = cp[vn].ev.ALPHA + (int)((cp[vright].ev.ALPHA - cp[vn].ev.ALPHA) * lc);
        hright.ev.Fog   = cp[vn].ev.Fog   +      ((cp[vright].ev.Fog   - cp[vn].ev.Fog  ) * lc);
    }

    if (ClipRes == 0)
    {
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
    if (ClipRes == 3)
    {
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


void _fillMappingClip(bool SECOND)
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

void _fillMapping(bool SECOND)
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


void DrawTPlane(bool SECONT)
{
	int n;
    MulVectorsVect(SubVectors(ev[1].v, ev[0].v), SubVectors(ev[2].v, ev[0].v), nv);
    if ( (nv.x*ev[0].v.x  +  nv.y*ev[0].v.y  +  nv.z*ev[0].v.z) < 0 ) return;

	Mask1=0x007F;
	for (n=0; n<3; n++)
	{
		if (ev[n].DFlags & 128) return; // 128 means dont render
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
		if (zz > 0) alpha1 = af_max(0, 255 - zz / 3); else alpha1 = 255;

		zz = (int)VectorLength(ev[1].v) - 256 * (ctViewR-4);
		if (zz > 0) alpha2 = af_max(0, 255 - zz / 3); else alpha2 = 255;

		zz = (int)VectorLength(ev[2].v) - 256 * (ctViewR-4);
		if (zz > 0) alpha3 = af_max(0, 255 - zz / 3); else alpha3 = 255;

		if ( alpha1 != 255 || alpha2 != 255 || alpha3 != 255 )
		{
			oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
			oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );
		}
	}

	if ( CurrentTexture != gStateGLTextureBind )
	{
		if (lpVertex) oglEndBuffer();
		oglStartBuffer();

		oglSetTexture( CurrentTexture );
	}

	if (!lpVertex) oglStartBuffer();

	if ( pGLVerticesCount >= 2048 )
	{
		oglEndBuffer( );
		oglStartBuffer();
	}

    lpVertex->x       = (float)ev[0].scrx;
    lpVertex->y       = (float)ev[0].scry;
    lpVertex->z       = 1.0f - (ev[0].v.z / ZSCALE);
    lpVertex->rhw      = 1;//gvtx[v].z * _AZSCALE;
    lpVertex->color    = (int)(ev[0].Light) * 0x00010101 | alpha1<<24;
	lpVertex->fog		= af_min(1.f, ev[0].Fog / 255.f);
    lpVertex->tu       = (float)(scrp[0].tx);// / (128.f*65536.f);
    lpVertex->tv       = (float)(scrp[0].ty);// / (128.f*65536.f);

    //lpVertex->color = rgba_t( 255-255*lpVertex->z, 0,0, 255 ).as32();

    lpVertex++;

	lpVertex->x       = (float)ev[1].scrx;
    lpVertex->y       = (float)ev[1].scry;
    lpVertex->z       = 1.0f - (ev[1].v.z / ZSCALE);
    lpVertex->rhw      = 1;//lpVertex->.z * _AZSCALE;
    lpVertex->color    = (int)(ev[1].Light) * 0x00010101 | alpha2<<24;
	lpVertex->fog		= af_min(1.f, ev[1].Fog / 255.f);
    lpVertex->tu       = (float)(scrp[1].tx);// / (128.f*65536.f);
    lpVertex->tv       = (float)(scrp[1].ty);// / (128.f*65536.f);

    //lpVertex->color = rgba_t( 255-255*lpVertex->z, 0,0, 255 ).as32();

    lpVertex++;

	lpVertex->x       = (float)ev[2].scrx;
    lpVertex->y       = (float)ev[2].scry;
    lpVertex->z       = 1.0f - (ev[2].v.z / ZSCALE);
    lpVertex->rhw      = 1;//lpVertex->.z * _AZSCALE;
    lpVertex->color    = (int)(ev[2].Light) * 0x00010101 | alpha3<<24;
	lpVertex->fog		= af_min(1.f, ev[2].Fog / 255.f);
    lpVertex->tu       = (float)(scrp[2].tx);// / (128.f*65536.f);
    lpVertex->tv       = (float)(scrp[2].ty);// / (128.f*65536.f);

    //lpVertex->color = rgba_t( 255-255*lpVertex->z, 0,0, 255 ).as32();

	lpVertex++;

	pGLVerticesCount += 3;
}

void DrawTPlaneClip(bool SECONT)
{
	if (!g->WATERREVERSE)
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

	// Clip volume (sides)
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

	if ( CurrentTexture != gStateGLTextureBind )
	{
		if (lpVertex) oglEndBuffer();
		oglStartBuffer();

		oglSetTexture( CurrentTexture );
	}

	if (!lpVertex) oglStartBuffer();

	if ( pGLVerticesCount >= 2048 )
	{
		if (lpVertex) oglEndBuffer( );
		oglStartBuffer();
	}

	for (u=0; u<vused-2; u++)
	{

		lpVertex->x			= (float)cp[0].ev.scrx * 1.f;
		lpVertex->y			= (float)cp[0].ev.scry * 1.f;
		lpVertex->z			= 1.0f - ( cp[0].ev.v.z / ZSCALE );
		lpVertex->rhw		= 1;
		lpVertex->color		= (int)(cp[0].ev.Light) * 0x00010101 | ((int)cp[0].ev.ALPHA<<24);
		lpVertex->fog		= min(1.f, cp[0].ev.Fog / 255.f);;
		lpVertex->tu		= (float)(cp[0].tx);
		lpVertex->tv		= (float)(cp[0].ty);
		lpVertex++;

		lpVertex->x       = (float)cp[u+1].ev.scrx * 1.f;
		lpVertex->y       = (float)cp[u+1].ev.scry * 1.f;
		lpVertex->z       = 1.0f - ( cp[u+1].ev.v.z / ZSCALE );
		lpVertex->rhw      = 1;
		lpVertex->color    = (int)(cp[u+1].ev.Light) * 0x00010101 | ((int)cp[u+1].ev.ALPHA<<24);
		lpVertex->fog		= min(1.f, cp[u+1].ev.Fog / 255.f);;
		lpVertex->tu       = (float)(cp[u+1].tx);
		lpVertex->tv       = (float)(cp[u+1].ty);
		lpVertex++;

		lpVertex->x       = (float)cp[u+2].ev.scrx * 1.f;
		lpVertex->y       = (float)cp[u+2].ev.scry * 1.f;
		lpVertex->z       = 1.0f - ( cp[u+2].ev.v.z / ZSCALE );
		lpVertex->rhw      = 1;
		lpVertex->color    = (int)(cp[u+2].ev.Light) * 0x00010101 | ((int)cp[u+2].ev.ALPHA<<24);
		lpVertex->fog		= min(1.f, cp[u+2].ev.Fog / 255.f);;
		lpVertex->tu       = (float)(cp[u+2].tx);
		lpVertex->tv       = (float)(cp[u+2].ty);
		lpVertex++;

		pGLVerticesCount+=3;
	}
}

void DrawTPlaneW(bool SECONT)
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


	if (!g->UNDERWATER)
	if (zs > (ctViewR-8)<<8)
	{
		float zz;
		zz = VectorLength(ev[0].v) - 256 * (ctViewR-4);
		if (zz > 0) ev[0].ALPHA = (short)max(0.f, 255.f - zz / 3.f);

		zz = VectorLength(ev[1].v) - 256 * (ctViewR-4);
		if (zz > 0) ev[1].ALPHA = (short)max(0.f, 255.f - zz / 3.f);

		zz = VectorLength(ev[2].v) - 256 * (ctViewR-4);
		if (zz > 0) ev[2].ALPHA = (short)max(0.f, 255.f - zz / 3.f);
	}

	if ( CurrentTexture != gStateGLTextureBind )
	{
		if (lpVertex) oglEndBuffer();
		oglStartBuffer();

		oglSetTexture( CurrentTexture );
	}

	if (!lpVertex) oglStartBuffer();

	if ( pGLVerticesCount >= 2048 )
	{
		if (lpVertex) oglEndBuffer( );
		oglStartBuffer();
	}

    lpVertex->x       = (float)ev[0].scrx;
    lpVertex->y       = (float)ev[0].scry;
    lpVertex->z       = 1.0f - ( ev[0].v.z / ZSCALE );
    lpVertex->rhw      = 1;//lpVertexG->sz * _AZSCALE;
    lpVertex->color    = (int)(ev[0].Light) * 0x00010101 | ev[0].ALPHA<<24;
	lpVertex->fog		= 0;
	lpVertex->tu       = (float)(scrp[0].tx);
    lpVertex->tv       = (float)(scrp[0].ty);
	lpVertex++;

	lpVertex->x       = (float)ev[1].scrx;
    lpVertex->y       = (float)ev[1].scry;
    lpVertex->z       = 1.0f - ( ev[1].v.z / ZSCALE );
    lpVertex->rhw      = 1;//lpVertexG->sz * _AZSCALE;
    lpVertex->color    = (int)(ev[1].Light) * 0x00010101 | ev[1].ALPHA<<24;
	lpVertex->fog		= 0;
	lpVertex->tu       = (float)(scrp[1].tx);
    lpVertex->tv       = (float)(scrp[1].ty);
	lpVertex++;

	lpVertex->x       = (float)ev[2].scrx;
    lpVertex->y       = (float)ev[2].scry;
    lpVertex->z       = 1.0f - ( ev[2].v.z / ZSCALE );
    lpVertex->rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
    lpVertex->color    = (int)(ev[2].Light) * 0x00010101 | ev[2].ALPHA<<24;
	lpVertex->fog		= 0;
	lpVertex->tu       = (float)(scrp[2].tx);
    lpVertex->tv       = (float)(scrp[2].ty);
	lpVertex++;

	pGLVerticesCount+=3;

    //oglDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
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

	if ( fabsf(xx*FOVK) > -zz + BackR) return;


	zs = (int)sqrtf( xx*xx + zz*zz + yy*yy);

	CurrentTexture = Textures[t1]->m_texid;

	oglSetRenderState( GL_BLEND, false );

	if (r>3)	DrawTPlane(false);
	else		DrawTPlaneClip(false);
	//DrawTPlane( false );

	if (ReverseOn)	{ ev[0] = ev[2]; ev[2] = VMap[y+1][x+1]; }
    else			{ ev[1] = ev[2]; ev[2] = VMap[y+1][x];   }

	if (r>3)	DrawTPlane(true);
    else		DrawTPlaneClip(true);
	//DrawTPlane( true );

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
	ReverseOn = false;

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

	CurrentTexture = Textures[t1]->m_texid;

	DrawTPlane(false);

	if (ReverseOn) { ev[0] = ev[2]; ev[2] = VMap[y+2][x+2]; }
             else { ev[1] = ev[2]; ev[2] = VMap[y+2][x];   }

	DrawTPlane(true);

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


   g->WATERREVERSE = true;
   int t1 = WaterList[ WMap[y][x] ].tindex;

   ev[0] = VMap2[y-CCY+128][x-CCX+128];
   if (ev[0].v.z>BackViewR) return;

   ReverseOn = false;
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

	CurrentTexture = Textures[t1]->m_texid;

	//DrawTPlaneW(false);
	if (r>3)	DrawTPlaneW(false);
    else		DrawTPlaneClip(false);

	ev[1] = ev[2]; ev[2] = VMap2[y+1][x];

	//DrawTPlaneW(true);
	if (r>3)	DrawTPlaneW(true);
    else		DrawTPlaneClip(true);

	g->WATERREVERSE = false;
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

	//g->WATERREVERSE = true;
	ReverseOn = false;
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

	CurrentTexture = Textures[t1]->m_texid;

	DrawTPlaneW(false);
	ev[1] = ev[2]; ev[2] = VMap2[y+2][x];

	DrawTPlaneW(true);

}

void RenderHealthBar()
{
	if (MyHealth >= 100000) return;
	if (MyHealth == 000000) return;


    int L = WinW / 4;
	int x0 = WinW - (WinW / 20) - L;
	int y0 = WinH / 40;
	int G = min( (MyHealth * 240 / 100000), 160);
	int R = min( ( (100000 - MyHealth) * 240 / 100000), 160);


	int L0 = (L * MyHealth) / 100000;
	int H = (float(WinH) / 480.0f) * 4;

	for (int y=0; y<H; y++)
	{
		oglDrawLine((float)x0-1,(float)y0+y,(float)x0+L+1,(float)y0+y,0xF0000000);
	}

	for (int y=1; y<H-1; y++)
	{
        oglDrawLine((float)x0,(float)y0+y,(float)x0+L0,(float)y0+y,0xF0000000 + (G<<8) + (R<<0));
	}
}

void RenderFSRect(uint32_t Color)
{
	oglSetRenderState( GL_BLEND, GL_TRUE );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

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

	oglSetTexture( 0 );

	oglDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);
	oglDrawTriangle(gvtx[2],gvtx[3],gvtx[0]);

	oglSetRenderState( GL_BLEND, GL_FALSE );
}


void RenderModel(TModel* _mptr, float x0, float y0, float z0, int light, int VT, float al, float bt)
{
   int f;

   if (fabsf(y0) > -(z0-256*6)) return;

   mptr = _mptr;

   float ca = (float)cosf(al);
   float sa = (float)sinf(al);

   float cb = (float)cosf(bt);
   float sb = (float)sinf(bt);

   float minx = 10241024;
   float maxx =-10241024;
   float miny = 10241024;
   float maxy =-10241024;

   bool FOGACTIVE = (g->FOGON && (FogYBase>0));

   int alphamask = (255-GlassL)<<24;
   int ml = light;

    TPoint3d p;
    for (int s=0; s<mptr->VCount; s++)
    {
        p = mptr->gVertex[s];

        if (FOGACTIVE)
        {
            vFogT[s] = (float)255-(int)(FogYBase + p.y * FogYGrad);
            if (vFogT[s]<5.0f  ) vFogT[s] = 5.0f;
            if (vFogT[s]>255.0f) vFogT[s] = 255.0f;
            //vFogT[s]<<=24;
        }
        else vFogT[s] = 255.0f;//<<24;

        rVertex[s].x = (p.x * ca + p.z * sa) + x0;

        float vz = p.z * ca - p.x * sa;

        rVertex[s].y = (p.y * cb - vz * sb)  + y0;
        rVertex[s].z = (vz  * cb + p.y * sb) + z0; //float(ctViewR-8);

        if (rVertex[s].z<-32)
        {
			//ml = 0;
            gScrpf[s].x = VideoCXf - (rVertex[s].x / rVertex[s].z * CameraW);
            gScrpf[s].y = VideoCYf + (rVertex[s].y / rVertex[s].z * CameraH);
        }
		else
		{
			gScrpf[s].x=-102400;
		}

        if (gScrpf[s].x > maxx) maxx = gScrpf[s].x;
        if (gScrpf[s].x < minx) minx = gScrpf[s].x;
        if (gScrpf[s].y > maxy) maxy = gScrpf[s].y;
        if (gScrpf[s].y < miny) miny = gScrpf[s].y;
    }

   if (minx == 10241024) return;
   if (minx>WinW || maxx<0 || miny>WinH || maxy<0) return;


	BuildTreeNoSortf();

	//float d = (float) sqrtf(x0*x0 + y0*y0 + z0*z0);
	//if (LOWRESTX) d = 14*256;

	if ( !oglIsTextureBound( mptr->m_texture.m_texid ) )
	{
		if (lpVertex && g_VertBufferBMP ) oglEndBuffer( );
		oglSetTexture( mptr->m_texture.m_texid );
	}
	// These seem to be unused
	//int PrevOpacity = 0;
	//int NewOpacity = 0;
	//int PrevTransparent = 0;
	//int NewTransparent = 0;

	if ( FOGACTIVE )
	{
		oglSetRenderState( GL_FOG, GL_TRUE );
		float fogColor[] = {
			((CurFogColor>>0 ) & 255)/255.0f,
			((CurFogColor>>8 ) & 255)/255.0f,
			((CurFogColor>>16) & 255)/255.0f,
			1
		};
		glFogfv(GL_FOG_COLOR, fogColor);
	}
	oglStartBuffer();

	int fproc1 = 0;
	int fproc2 = 0;
	f = Current;
	bool CKEY = false;
	while( f!=-1 )
	{
		TFacef *fptr = & mptr->gFace[f];
		f = mptr->gFace[f].Next;

		if (minx<0)
		if (gScrpf[fptr->v1].x <0    && gScrpf[fptr->v2].x<0    && gScrpf[fptr->v3].x<0) continue;
		if (maxx>WinW)
		if (gScrpf[fptr->v1].x >WinW && gScrpf[fptr->v2].x>WinW && gScrpf[fptr->v3].x>WinW) continue;

		if (fptr->Flags & (sfOpacity)) fproc2++; else fproc1++;

		int _ml = ml + mptr->VLight[VT][fptr->v1];
		lpVertex->x       = (float)gScrpf[fptr->v1].x;
		lpVertex->y       = (float)gScrpf[fptr->v1].y;
		lpVertex->z       = 1.0f - (rVertex[fptr->v1].z / ZSCALE);
		lpVertex->rhw      = 1;
		lpVertex->color    = _ml * 0x00010101 | alphamask;
		lpVertex->fog		= (float) 1.0f - (vFogT[fptr->v1] / 255.0f);
		lpVertex->tu       = float(fptr->tax / 256.0f);
		lpVertex->tv       = float(fptr->tay / 256.0f);
		lpVertex++;

		_ml = ml + mptr->VLight[VT][fptr->v2];
		lpVertex->x			= (float)gScrpf[fptr->v2].x;
		lpVertex->y			= (float)gScrpf[fptr->v2].y;
		lpVertex->z			= 1.0f - ( rVertex[fptr->v2].z / ZSCALE );
		lpVertex->rhw		= 1.0f;
		lpVertex->color		= _ml * 0x00010101 | alphamask;
		lpVertex->fog		= (float) 1.0f - (vFogT[fptr->v2] / 255.0f);
		lpVertex->tu		= (float)(fptr->tbx/256.0f);
		lpVertex->tv		= (float)(fptr->tby/256.0f);
		lpVertex++;

		_ml = ml + mptr->VLight[VT][fptr->v3];
		lpVertex->x       = (float)gScrpf[fptr->v3].x;
		lpVertex->y       = (float)gScrpf[fptr->v3].y;
		lpVertex->z       = 1.0f - ( rVertex[fptr->v3].z / ZSCALE );
		lpVertex->rhw      = 1.f;
		lpVertex->color    = _ml * 0x00010101 | alphamask;
		lpVertex->fog		= (float) 1.0f - (vFogT[fptr->v3] / 255.0f);
		lpVertex->tu       = (float)(fptr->tcx/256.0f);
		lpVertex->tv       = (float)(fptr->tcy/256.0f);
		lpVertex++;

		if (fproc1+fproc2>=2048)
		{
			oglFlushBuffer( fproc1, fproc2 );
			oglStartBuffer( );
			fproc1 = fproc2 = 0;
		}
	}

	oglFlushBuffer( fproc1, fproc2 );
}


void RenderModelClip(TModel* _mptr, float x0, float y0, float z0, int light, int VT, float al, float bt)
{
	int f,CMASK;
	mptr = _mptr;

	float ca = (float)cosf(al);
	float sa = (float)sinf(al);

	float cb = (float)cosf(bt);
	float sb = (float)sinf(bt);


	int flight = (int)light;
	uint32_t almask;
	uint32_t alphamask = (255-GlassL)<<24;


	bool BL = false;
	bool FOGACTIVE = (g->FOGON && (FogYBase>0));

	for (int s=0; s<mptr->VCount; s++) {

	if (FOGACTIVE)
	{
		oglSetRenderState( GL_FOG, GL_TRUE );
		vFogT[s] = 255-(int)(FogYBase + mptr->gVertex[s].y * FogYGrad);
		if (vFogT[s]<5  ) vFogT[s] = 5.0f;
		if (vFogT[s]>255) vFogT[s]=255.0f;
	}
	else vFogT[s] = 255.0f;


	rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
    float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
    rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
    rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
    if (rVertex[s].z<0) BL=true;

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

	if (lpVertex) oglEndBuffer();

	oglSetTexture( mptr->m_texture.m_texid );

	BuildTreeClipNoSort();

	oglStartBuffer();

	f = Current;
	int fproc1 = 0;
	int fproc2 = 0;
	bool CKEY = false;

	while( f!=-1 )
	{
		vused = 3;
		TFacef *fptr = &mptr->gFace[f];

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
		if (fptr->Flags & sfTransparent) { almask = 0x70000000; }; //oglSetRenderState( GL_BLEND, GL_TRUE ); }
		if (almask > alphamask) almask = alphamask;

		for (u=0; u<vused-2; u++)
		{
			float k1,k2,k3;
			k1 = k2 = k3 = 1.0f;

			int _flight = flight + cp[0].ev.Light;
	   		lpVertex->x       = VideoCXf - cp[0].ev.v.x / cp[0].ev.v.z * CameraW;
			lpVertex->y       = VideoCYf + cp[0].ev.v.y / cp[0].ev.v.z * CameraH;
			lpVertex->z       = 1.0f - ( cp[0].ev.v.z / ZSCALE );
			lpVertex->rhw      = 1.0f;
			lpVertex->color    = (_flight * 0x00010101) | almask;
			lpVertex->fog		= (float) 1.0f - (cp[0].ev.Fog / 255.0f);
			lpVertex->tu       = (float)((cp[0].tx) / 256.0f ) * k1;
			lpVertex->tv       = (float)((cp[0].ty) / 256.0f ) * k1;
			lpVertex++;

			_flight = flight + cp[u+1].ev.Light;
	   		lpVertex->x       = VideoCXf - cp[u+1].ev.v.x / cp[u+1].ev.v.z * CameraW;
			lpVertex->y       = VideoCYf + cp[u+1].ev.v.y / cp[u+1].ev.v.z * CameraH;
			lpVertex->z       = 1.0f - ( cp[u+1].ev.v.z / ZSCALE );
			lpVertex->rhw      = 1.0f;
			lpVertex->color    = (_flight * 0x00010101) | almask;
			lpVertex->fog		= (float) 1.0f - (cp[u+1].ev.Fog / 255.0f);
			lpVertex->tu       = (float)(cp[u+1].tx/256.0f) * k2;
			lpVertex->tv       = (float)(cp[u+1].ty/256.0f) * k2;
			lpVertex++;

			_flight = flight + cp[u+2].ev.Light;
	   		lpVertex->x       = VideoCXf - cp[u+2].ev.v.x / cp[u+2].ev.v.z * CameraW;
			lpVertex->y       = VideoCYf + cp[u+2].ev.v.y / cp[u+2].ev.v.z * CameraH;
			lpVertex->z       = 1.0f - ( cp[u+2].ev.v.z / ZSCALE );
			lpVertex->rhw      = 1.f;//gvtx[v].z * _AZSCALE;//1.0f;
			lpVertex->color    = (_flight * 0x00010101) | almask;
			lpVertex->fog		= (float) 1.0f - (cp[u+2].ev.Fog / 255.0f);
			lpVertex->tu       = (float)(cp[u+2].tx/256.0f) * k3;
			lpVertex->tv       = (float)(cp[u+2].ty/256.0f) * k3;
			lpVertex++;


			if ( fptr->Flags & sfDoubleSide )
			 {
				oglSetRenderState( GL_CULL_FACE, false );
			 }

			if (fptr->Flags & (sfOpacity)) fproc2++; else fproc1++;
		}
LNEXT:
		f = mptr->gFace[f].Next;

		if (fproc1+fproc2>=2048)
		{
			oglFlushBuffer( fproc1, fproc2 );
			oglStartBuffer( );
			fproc1 = fproc2 = 0;
		}
	}

	oglFlushBuffer( fproc1, fproc2 );
}

void BuildTreeNoSort()
{
    vec2i v[3];
	Current = -1;
    int LastFace = -1;
    TFacef* fptr;
    int sg;

	for (int f=0; f<mptr->FCount; f++)
	{
        fptr = &mptr->gFace[f];
  		v[0] = gScrp[fptr->v1];
        v[1] = gScrp[fptr->v2];
        v[2] = gScrp[fptr->v3];

        if (v[0].x == 0xFFFFFF) continue;
        if (v[1].x == 0xFFFFFF) continue;
        if (v[2].x == 0xFFFFFF) continue;

        if (fptr->Flags & (sfDarkBack+sfNeedVC)) {
           sg = (v[1].x-v[0].x)*(v[2].y-v[1].y) - (v[1].y-v[0].y)*(v[2].x-v[1].x);
           if (sg<0) continue;
        }

		fptr->Next=-1;
        if (Current==-1) { Current=f; LastFace = f; } else
        { mptr->gFace[LastFace].Next=f; LastFace=f; }

	}
}

void BuildTreeNoSortf()
{
    Vector2df v[3];
	Current = -1;
    int LastFace = -1;
    TFacef* fptr;
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
    TFacef* fptr;

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


void CalcFogLevel_Gradient(vec3 v)
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
	if (!g->MODELS) return;
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
		RenderBMPModel(&MObjects[ob].bmpmodel, v[0].x, v[0].y, v[0].z, mlight-16);
	}
	else
	if (v[0].z<-256*8)
	{
		RenderModel(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, FI, fi, CameraBeta);
	}
	else
	{
		//RenderModel(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, FI, fi, CameraBeta);
		RenderModelClip(MObjects[ob].model, v[0].x, v[0].y, v[0].z, mlight, FI, fi, CameraBeta);
	}
}


//=========================================================================//
// Global Source (All .cpp files)
//=========================================================================//
void Init3DHardware()
{
	// -> Perform 3d hardware startup (OpenGL, DD, D3D,  etc)
	HARD3D = true;
	PrintLog( "\nvoid Init3DHardware()\n{\n" );

	GLenum err = glewInit();
	if ( err != GL_NO_ERROR )
	{
		PrintLog( "\tFailed to initialise GLEW!\n" );
		HARD3D = false;
	}

	VidOpt = GlobalVideoOptions::SharedVariable();

	IniFile* ini = new IniFile();
	ini->File( "VIDEO.INI" );

	VidOpt->mAnisotropyLevels =		ini->ReadKeyFloat( "OpenGL", "Anisotropy", 1.0f );
	VidOpt->mAntialiasingLevels =	ini->ReadKeyInt( "OpenGL", "Antialiasing", 1 );
	VidOpt->mWidth =				ini->ReadKeyInt( "OpenGL", "ScreenW", WinW );
	VidOpt->mHeight =				ini->ReadKeyInt( "OpenGL", "ScreenH", WinH );
	VidOpt->mBackBuffer =			ini->ReadKeyInt( "OpenGL", "ColorBits", 16 );
	VidOpt->mZBuffer =				ini->ReadKeyInt( "OpenGL", "DepthBits", 16 );
	VidOpt->mVSync =				ini->ReadKeyInt( "OpenGL", "VSync", false )?true:false;

	delete ini;

	WinW = VidOpt->mWidth;
	WinH = VidOpt->mHeight;

	// -- Perform a Direct3D-esque capabilities report
	oglGetCaps();

	// -- Create the font object
	oglCreateFont();

	// -- Initialize states
	LastTexture = 0;
	CurrentTexture = 0;
	glTextureCount = 0;

	// OpenGL Hints
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_FOG_HINT, GL_NICEST);

	// -- Basic OpenGL Properties
	oglSetRenderState( GL_TEXTURE_2D, true );
	oglSetRenderState( GL_COLOR_MATERIAL, true );
	oglSetRenderState( GL_DEPTH_FUNC, GL_LESS );
	glDepthRange( 0.0f, 1.0f );
	// -- Enable and set alpha testing
	oglSetRenderState( GL_ALPHA_TEST, true );
	glAlphaFunc( GL_GEQUAL, 0.35f );
	// -- Disable and set blending
	oglSetRenderState( GL_BLEND, false );
	oglSetRenderState( GL_BLEND_SRC, GL_SRC_ALPHA );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );
	// -- Enable and set culling
	oglSetRenderState( GL_CULL_FACE, false );
	oglSetRenderState( GL_CULL_FACE_MODE, GL_FRONT );
	// -- Set Texture Filter Defaults
	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	// == Fog == //
	oglSetRenderState( GL_FOG, GL_TRUE );
	float fogColor[] = {1,0,0,1};
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_START, 0.0f);
    glFogf(GL_FOG_END, 1.0f);
	glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
	if (glFogCoordf) glFogCoordf( 0.0f );

	// -- Enable the client states
	glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );
	if ( GLEW_EXT_fog_coord ) glEnableClientState( GL_FOG_COORDINATE_ARRAY_EXT );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	// -- Set the data pointers
	glVertexPointer(4, GL_FLOAT, sizeof(GLVertex), &pGLVertices[0].x);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(GLVertex), &pGLVertices[0].color);
	if ( GLEW_EXT_fog_coord ) glFogCoordPointer( GL_FLOAT, sizeof(GLVertex), &pGLVertices[0].fog );
	glTexCoordPointer(2, GL_FLOAT, sizeof(GLVertex), &pGLVertices[0].tu);

	lpScreenBuf = malloc( sizeof(uint16_t) * ( WinW * WinH ) );

	PrintLog( "\treturn true;\n" );
	PrintLog( "}\n" );
}

void SetWireframe( bool b )
{
	if ( b )
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Activate3DHardware()
{
	// == Global Function == //
	// -> Called by reinit game
	// -> Simply reload/restart hardware (Was shutdown by shutdown3dhardware)
	if ( HARD3D == false ) return;

	PrintLog("\nvoid Restart3DHardware()\n{\n");

	VidOpt = GlobalVideoOptions::SharedVariable();

	if (WinW<640) SetVideoMode(640,480);
	else SetVideoMode(WinW,WinH);

	sprintf( logt, "\tResolution: %dx%d\n", WinW,WinH );
	PrintLog( logt );
	glfwSetWindowSize( GWindow, WinW, WinH );

	// Go Fullscreen
	if ( Windowed==false )
	{
		// GLFW_FULLSCREEN

		bFullscreen = true;
	}
	else
	{
		// GLFW_WINDOW

		PrintLog( "\tWindowed Mode: " );

		int s_w, s_h;
		GetScreenSize( &s_w, &s_h );
		glfwSetWindowPos( GWindow, (s_w-WinW)/2, (s_h-WinH)/2 );

		bFullscreen = false;

		PrintLog( "Ok\n" );
	}

	// -- Enable / Disable VSync
	glfwSwapInterval( VidOpt->mVSync );

	oglSetRenderState( GL_TEXTURE_MAX_ANISOTROPY_EXT, VidOpt->mAnisotropyLevels );

	// -- Enable MSAA
#if defined( AF_PLATFORM_LINUX )
	if ( GLXEW_ARB_multisample )
#elif defined( AF_PLATFORM_WINDOWS )
	if ( WGLEW_ARB_multisample )
#endif
	{
		if ( VidOpt->mAntialiasingLevels > 0 )
			oglSetRenderState( GL_MULTISAMPLE, GL_TRUE );
		else
			oglSetRenderState( GL_MULTISAMPLE, GL_FALSE );
	}

	if ( GLEW_EXT_clip_volume_hint )
	{
		glHint( GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_NICEST );
		PrintLog( "\tDisabling OpenGL Clip Volume\r\n" );
	}

	// == Re-Create GL Textures == //

	//Sky Texture
	oglCreateTexture( false, false, &SkyTexture);

	//Create Sprites for UI
	oglCreateSprite(false,PausePic);
	oglCreateSprite(false,ExitPic);
	oglCreateSprite(false,TrophyExit);
	oglCreateSprite(false,TrophyPic);
	oglCreateSprite(false,MapPic);
	oglCreateSprite(false,RadarPic);

	glClearDepth(1.0f);

	oglInit();

	// Clear the buffers
	oglClearBuffers();
	PrintLog("}\n");
}

void ShutDown3DHardware()
{
	// -> Shutdown hardware
	if ( HARD3D == false ) return;

	oglDeinit();

	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	if ( GLEW_EXT_fog_coord ) glDisableClientState( GL_FOG_COORDINATE_ARRAY_EXT );
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

	if ( lpScreenBuf )
	{
		free( lpScreenBuf );
		lpScreenBuf = 0;
	}

	PrintLog( "void ShutDown3DHardware()\n{\n" );

	//if ( bFullscreen )
	//ChangeDisplaySettings( 0, 0 );

	oglDeleteFont();
	glDeleteTextures(2048,pTextures);
	for (int i=0; i<2048; i++)
	{
		pTextures[i] = 0;
	}

	HARD3D == false;

	PrintLog("}\n");
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

	bool FOGACTIVE = (g->FOGON && (FogYBase>0));

	for (int s=0; s<4; s++)
	{

		if (FOGACTIVE) {
		vFogT[s] = 255-(int) (FogYBase + mptr->gVertex[s].y * FogYGrad);
		if (vFogT[s]<0  ) vFogT[s] = 0;
		if (vFogT[s]>255) vFogT[s]=255;
		//vFogT[s]<<=24;
		} else vFogT[s]=255.0f;

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

	//float d = (float) sqrtf(x0*x0 + y0*y0 + z0*z0);

	if ( !oglIsTextureBound( mptr->m_texture.m_texid ) )
	{
		if (lpVertex) oglEndBuffer( );
		LastMagFilter = CurrentMagFilter;
		LastMinFilter = CurrentMinFilter;
		oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		oglSetTexture( mptr->m_texture.m_texid );
	}

	if (!lpVertex) oglStartBufferBMP();

	if ( pGLVerticesCount >= 2048 )
	{
		if (lpVertex) oglEndBuffer( );
		oglStartBufferBMP();
	}

	lpVertex->x       = (float)gScrp[0].x;
	lpVertex->y       = (float)gScrp[0].y;
	lpVertex->z       = 1.0f - ( rVertex[0].z / ZSCALE );
	lpVertex->rhw      = 1.0f;//gvtx[0].z * _AZSCALE;
	lpVertex->color    = argb;
	lpVertex->fog		= 1.0f - (vFogT[0] / 255.0f);
	lpVertex->tu       = (float)(0.0f);
	lpVertex->tv       = (float)(0.0f);
	lpVertex++;

	lpVertex->x       = (float)gScrp[1].x;
	lpVertex->y       = (float)gScrp[1].y;
	lpVertex->z       = 1.0f - ( rVertex[1].z / ZSCALE );
	lpVertex->rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	lpVertex->color    = argb;
	lpVertex->fog		= 1.0f - (vFogT[1] / 255.0f);
	lpVertex->tu       = (float)(0.995f);
	lpVertex->tv       = (float)(0.0f);
	lpVertex++;

	lpVertex->x       = (float)gScrp[2].x;
	lpVertex->y       = (float)gScrp[2].y;
	lpVertex->z       = 1.0f - ( rVertex[2].z / ZSCALE );
	lpVertex->rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	lpVertex->color    = argb;
	lpVertex->fog		= 1.0f - (vFogT[2] / 255.0f);
	lpVertex->tu       = (float)(0.995f);
	lpVertex->tv       = (float)(0.995f);
	lpVertex++;

	//=========//

	lpVertex->x       = (float)gScrp[0].x;
	lpVertex->y       = (float)gScrp[0].y;
	lpVertex->z       = 1.0f - ( rVertex[0].z / ZSCALE );
	lpVertex->rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	lpVertex->color    = argb;
	lpVertex->fog		= 1.0f - (vFogT[0] / 255.0f);
	lpVertex->tu       = (float)(0.0f);
	lpVertex->tv       = (float)(0.0f);
	lpVertex++;

    lpVertex->x       = (float)gScrp[2].x;
    lpVertex->y       = (float)gScrp[2].y;
    lpVertex->z       = 1.0f - ( rVertex[2].z / ZSCALE );
    lpVertex->rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
    lpVertex->color    = argb;
	lpVertex->fog		= 1.0f - (vFogT[2] / 255.0f);
    lpVertex->tu       = (float)(0.995f);
    lpVertex->tv       = (float)(0.995f);
	lpVertex++;

	lpVertex->x       = (float)gScrp[3].x;
	lpVertex->y       = (float)gScrp[3].y;
	lpVertex->z       = 1.0f - ( rVertex[3].z / ZSCALE );
	lpVertex->rhw      = 1.0f;//lpVertexG->sz * _AZSCALE;
	lpVertex->color    = argb;
	lpVertex->fog		= 1.0f - (vFogT[3] / 255.0f);
	lpVertex->tu       = (float)(0.0f);
	lpVertex->tv       = (float)(0.995f);
	lpVertex++;

	pGLVerticesCount += 6;

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, LastMinFilter );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, LastMagFilter );
}


void Draw_Text(int x,int y,char* text, unsigned long color)
{
	oglTextOut(x,y,text,color);
}


void ShowVideo()
{
	// == Global Function == //
	// -> Simply draw everything
	// -> Also draw the health and energy bar
	//Render overlays

	oglSetRenderState( GL_DEPTH_TEST, GL_FALSE );
	oglSetRenderState( GL_TEXTURE_2D, GL_FALSE );

	//Draw the underwater haze
	if (g->UNDERWATER) RenderFSRect(0x80000000 + CurFogColor);

	//Draw the sun glare
	if (OptDayNight!=2)
	if (!g->UNDERWATER && (SunLight>1.0f) )
	{
		RenderFSRect(0xC0FFFF + ((int)(SunLight)<<24));
	}

	//Draw the sun flare
	//->ToDo: James

	//Draw the health bar
	RenderHealthBar();

    oglSetRenderState( GL_DEPTH_TEST, GL_TRUE );
    oglSetRenderState( GL_TEXTURE_2D, GL_TRUE );

	MyPrevHealth = MyHealth;

#ifdef AF_DEBUG
	//if ( g->GREMEDY ) glFrameTerminatorGREMEDY(); //<-For OpenGL Debugging.
#endif
	glfwSwapBuffers( GWindow );
	oglClearBuffers();
}

void DrawPicture(int x, int y, TPicture &pic)
{
	// == Global Function == //
	// -> Draw a picture on screen

	oglSetRenderState( GL_DEPTH_TEST, GL_FALSE );
	oglSetRenderState( GL_FOG, GL_FALSE );

	//if (OptDayNight==2 && !NightVision) glDisable(GL_FOG);

	oglSetTexture( pic.m_texid );

	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	float fW = float(pic.m_width);
	float fH = float(pic.m_height);
	float fcW = float(pic.m_cwidth);
	float fcH = float(pic.m_cheight);

	glBegin(GL_QUADS);
	glColor4f(1,1,1,1);
	if ( glFogCoordf ) glFogCoordf( 0.0f );

	glTexCoord2f(0,0);
	glVertex4f(float(x),float(y),0.0f,1);
	glTexCoord2f(fW/fcW,0);
	glVertex4f(float(x+pic.m_width),float(y),0.0f,1);
	glTexCoord2f(fW/fcW, fH/fcH);
	glVertex4f(float(x+pic.m_width),float(y+pic.m_height),0.0f,1);
	glTexCoord2f(0, fH/fcH);
	glVertex4f(float(x),float(y+pic.m_height),0.0f,1);

	glEnd();
	DrawCalls++;

	oglSetRenderState( GL_DEPTH_TEST, GL_TRUE );

	//if (OptDayNight==2 && !NightVision) glEnable(GL_FOG);
}


void ShowControlElements()
{
	// == Global Function == //
	// -> Used to draw some informative text on screen
	// -> Includes drawing weapon list

	char buf[128];

	if ( g->TIMER )
	{
		sprintf( buf, "delta: %dms", TimeDt);
		oglTextOut( WinEX-100, 24, buf, 0xFF20A0A0);

		sprintf( buf, "tris: %d", dFacesCount);
		oglTextOut( WinEX-100, 35, buf, 0xFF20A0A0);

		sprintf( buf, "FPS: %u/60", FramesPerSecond );
		oglTextOut( WinEX-100, 46, buf, 0xFF20A0A0 );

		sprintf( buf, "calls: %u", DrawCalls );
		oglTextOut( WinEX-100, 57, buf, 0xFF20A0A0 );

		sprintf( buf, "texs: %u", TextureBinds );
		oglTextOut( WinEX-100, 68, buf, 0xFF20A0A0 );

		sprintf( buf, "ttime: %u", TerrainTime );
		oglTextOut( WinEX-100, 79, buf, 0xFF20A0A0 );

		sprintf( buf, "wtime: %u", WaterTime );
		oglTextOut( WinEX-100, 90, buf, 0xFF20A0A0 );

		sprintf( buf, "ctime: %u", CharacterTime );
		oglTextOut( WinEX-100, 101, buf, 0xFF20A0A0 );

		sprintf( buf, "otime: %u", ObjectTime );
		oglTextOut( WinEX-100, 112, buf, 0xFF20A0A0 );

		DrawCalls = 0;
		TextureBinds = 0;
		Counter = 0;
	}

	//oglTextOut( 3,3, "Запустите режим отключен", 0xFFFFFFFF );
	/*glPushMatrix();
	glScalef( 0.25f, 0.25f, 0.25f );

	DrawPicture( 3,3, RadarPic );

	glPopMatrix();*/

	// Draw the console
	if ( bDevShow )
	{
		oglSetRenderState( GL_BLEND, GL_TRUE );

		int th = GetTextH( 0, "A");

		oglDrawBox( 0,0, WinW, (th*32)+th, 0x80000000 );

		for (int i=0; i<32; i++)
		{
			if ( MessageList[i].mtext[0] == 0 ) continue;
			//if (RealTime>MessageList[i].mTimeLeft) MessageList[i].mTimeLeft = 0;
			oglTextOut(2, ( th * i ) + th, MessageList[i].mtext, *((uint32_t*)&MessageList[i].m_color ) );
		}

		char devstr[512];
		sprintf( devstr, "~%s%s", cDevConsole, ((int)glfwGetTime() % 1) ? "|" : " " );
		Draw_Text(2, 0, devstr, 0xFFFF00FF );

		oglSetRenderState( GL_BLEND, GL_FALSE );
	}

	/*for (int i=0; i<16; i++)
	if (MessageList[i].mTimeLeft)
	{
		if (RealTime>MessageList[i].mTimeLeft) MessageList[i].mTimeLeft = 0;

		float ratio = (float)( MessageList[i].mTimeLeft - RealTime ) / 5000.0f;
		int alpha = 255 * ratio;
		//alpha = 128;

		oglTextOut(2, WinH - 48 - (GetTextH(0, MessageList[i].mtext) * i ), MessageList[i].mtext, 0x0020A0A0 | (alpha<<24) );
	}*/

	if (ExitTime)
	{
		int y = WinH / 3;
		sprintf(buf,"Preparing for evacuation...");
		oglTextOut(VideoCX - GetTextW(0, buf)/2, y, buf, 0xFF60C0D0);

		sprintf(buf,"%d seconds left.", 1 + ExitTime / 1000);
		oglTextOut(VideoCX - GetTextW(0, buf)/2, y + 18, buf, 0xFF60C0D0);
	}

#ifdef AF_DEBUG
	{
		//sprintf( buf, "%s", "Carnivores [OpenGL] " V_VERSION " Debug" );
		sprintf( buf, "Carnivores [%s] %u.%u.%u r%u %s", V_PLATFORM, Version::MAJOR, Version::MINOR, Version::BUILD, Version::REVISION, Version::STATUS );
		oglTextOut( VideoCX - GetTextW(0, buf)/2, GetTextH(0, buf), buf, 0xFFFFFFFF );
	}
#endif
}


/*
	Renderg->MODELSList()
	Render all the map scenery models
*/
void RenderModelsList()
{
	// == Global Function == //
	// -> Loop through objects in view list/array and render them

	if (lpVertex) oglEndBuffer( );

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	oglSetRenderState( GL_BLEND, GL_FALSE );

	uint32_t start = glfwGetTime()*1000.0;

	for (int o=0; o<ORLCount; o++)
	{
		_RenderObject(ORList[o].x, ORList[o].y);
	}
	ORLCount=0;

	if (lpVertex) oglEndBuffer( );

	ObjectTime = uint32_t(glfwGetTime()*1000.0) - start;

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );
}


void RenderGround()
{
	// == Global Function == //
	// -> Render the ground
	uint32_t start = glfwGetTime() * 1000.0;

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	oglSetRenderState( GL_FOG, GL_TRUE );

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

	oglEndBuffer();

	r = ctViewR1-1;
	for (int x=r; x>-r; x--)
	{
		ProcessMap(CCX+r, CCY+x, r);
		ProcessMap(CCX+x, CCY+r, r);
	}

	oglEndBuffer();

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

	oglEndBuffer();

	ProcessMap(CCX, CCY, 0);

	oglEndBuffer();

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	TerrainTime = (glfwGetTime()*1000.0) - start;
}

void RenderWCircles()
{
	// -> Render the water rings

	TWCircle *wptr;
	vec3 rpos;

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
	// -> Render the water that is visible (perhaps use a variable named "bool NEEDWATER")

	//if (!NEEDWATER) return;

	uint32_t start = glfwGetTime()*1000.0;

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	oglSetRenderState( GL_ALPHA_TEST, false );
	oglSetRenderState( GL_CULL_FACE, false );
	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

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

	if (lpVertex) oglEndBuffer();

	for (int y=-ctViewR1+2; y<ctViewR1; y++)
    for (int x=-ctViewR1+2; x<ctViewR1; x++)
	{
		ProcessMapW(CCX+x, CCY+y, max(abs(x), abs(y)));
	}

	if (lpVertex) oglEndBuffer();

	oglSetRenderState( GL_CULL_FACE, true );

	FogYBase = 0;
	oglSetRenderState( GL_BLEND_DST, GL_ONE );

	RenderWCircles();

	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );
	oglSetRenderState( GL_BLEND, false );
	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	WaterTime = (glfwGetTime()*1000.0) - start;
}


void RenderModelSun(TModel* _mptr, float x0, float y0, float z0, int Alpha)
{
	// -> Render the sun billboard model
    int f;

    mptr = _mptr;

    int minx = 10241024;
    int maxx =-10241024;
    int miny = 10241024;
    int maxy =-10241024;


    for (int s=0; s<mptr->VCount; s++)
    {
        rVertex[s].x = mptr->gVertex[s].x + x0;
        rVertex[s].y = mptr->gVertex[s].y + y0;
        rVertex[s].z = mptr->gVertex[s].z + z0;

        if (rVertex[s].z>-64) gScrp[s].x = 0xFFFFFF;
        else
        {
            gScrp[s].x = VideoCX + (int)(rVertex[s].x / (-rVertex[s].z) * CameraW);
            gScrp[s].y = VideoCY - (int)(rVertex[s].y / (-rVertex[s].z) * CameraH);
        }

        if (gScrp[s].x > maxx) maxx = gScrp[s].x;
        if (gScrp[s].x < minx) minx = gScrp[s].x;
        if (gScrp[s].y > maxy) maxy = gScrp[s].y;
        if (gScrp[s].y < miny) miny = gScrp[s].y;
    }

    if (minx == 10241024) {
        return;
    }
    if (minx>WinW || maxx<0 || miny>WinH || maxy<0) {
        return;
    }

    BuildTreeNoSort();

	oglSetTexture( mptr->m_texture.m_texid );

	oglSetRenderState( GL_FOG, false );
    oglStartBuffer();

    uint32_t alpha = (Alpha<<24) | 0x00FFFFFF;
    int fproc1 = 0;
    f = Current;
    while( f!=-1 )
    {
		TFacef *fptr = & mptr->gFace[f];

		fproc1++;

		lpVertex->x       = (float)gScrp[fptr->v1].x;
		lpVertex->y       = (float)gScrp[fptr->v1].y;
		lpVertex->z       = 0.9999f;//ZSCALE / rVertex[fptr->v1].z;
		lpVertex->rhw      = 1.f;
		lpVertex->color    = alpha;
		lpVertex->tu       = (float)(fptr->tax/256.0f);
		lpVertex->tv       = (float)(fptr->tay/256.0f);
		lpVertex++;

		lpVertex->x       = (float)gScrp[fptr->v2].x;
		lpVertex->y       = (float)gScrp[fptr->v2].y;
		lpVertex->z       = 0.9999f;//ZSCALE / rVertex[fptr->v2].z;
		lpVertex->rhw      = 1.f;
		lpVertex->color    = alpha;
		lpVertex->tu       = (float)(fptr->tbx/256.0f);
		lpVertex->tv       = (float)(fptr->tby/256.0f);
		lpVertex++;

		lpVertex->x       = (float)gScrp[fptr->v3].x;
		lpVertex->y       = (float)gScrp[fptr->v3].y;
		lpVertex->z       = 0.9999f;//ZSCALE / rVertex[fptr->v3].z;
		lpVertex->rhw      = 1.f;
		lpVertex->color    = alpha;
		lpVertex->tu       = (float)(fptr->tcx/256.0f);
		lpVertex->tv       = (float)(fptr->tcy/256.0f);
		lpVertex++;

		f = mptr->gFace[f].Next;
    }

    oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
    oglSetRenderState( GL_BLEND_DST, GL_ONE );

    oglFlushBuffer( fproc1, 0 );

	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );
	oglSetRenderState( GL_FOG, true );
    oglSetRenderState( GL_BLEND, false );
}


void RenderNearModel(TModel* _mptr, float x0, float y0, float z0, int light, float al, float bt)
{
	//== Global Function == //
	// -> Render a 3d model up close/Not 3d space. used to render Binocs, wind compass, compass, etc
	// -> z0 simply changes the size of the object. x and y are 2d space locations on screen
	// -> al and bt are alpha and beta radians.

	bool b = g->LOWRESTX;
	vec3 v;
	v.x = 0; v.y =-128; v.z = 0;

	CalcFogLevel_Gradient(v);
	FogYGrad = 0;

	oglSetRenderState( GL_CULL_FACE, false );

	g->LOWRESTX = false;
	RenderModelClip(_mptr, x0, y0, z0, light, 0, al, bt);
	g->LOWRESTX = b;

	oglSetRenderState( GL_CULL_FACE, true );
}

void RenderCircle(float cx, float cy, float z, float _R, uint32_t RGBA, uint32_t RGBA2)
{
	// == HARDWARE 3D ONLY == //
	// -> Render a circle particle
	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

	z = 1.0f - ( z / ZSCALE );

	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv((GLubyte*)&RGBA);

	glVertex4f(cx,cy,z,1.0f);

	for (int i=0; i<=16; i++)
	{
		glColor4ubv((GLubyte*)&RGBA2);
		if ( glFogCoordf ) glFogCoordf( 0.0f );

		float p = float(i/16.0f);
		float rx = sinf( ((360*p)*3.14f/180) ) *cosf(0)* (float)_R;
		float ry = cosf( ((360*p)*3.14f/180) ) *cosf(0)* (float)_R;
		glVertex4f((float)cx+rx,(float)cy+ry,z,1.f);
	}

	glEnd();
	DrawCalls++;

	oglSetRenderState( GL_BLEND, false );
}

void RenderCircle2(float cx, float cy, float z, float _R, uint32_t RGBA, uint32_t RGBA2)
{
	// == HARDWARE 3D ONLY == //
	// -> Render a circle particle
	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

	glBegin(GL_TRIANGLE_FAN);
	glColor4ubv((GLubyte*)&RGBA);

	glVertex4f(cx,cy,z,1.0f);

	for (int i=0; i<=16; i++)
	{
		glColor4ubv((GLubyte*)&RGBA2);
		if ( glFogCoordf ) glFogCoordf( 0.0f );

		float p = float(i/16.0f);
		float rx = sinf( ((360*p)*3.14f/180) ) *cosf(0)* (float)_R;
		float ry = cosf( ((360*p)*3.14f/180) ) *cosf(0)* (float)_R;
		glVertex4f((float)cx+rx,(float)cy+ry,z,1.f);
	}

	glEnd();
	DrawCalls++;

	oglSetRenderState( GL_BLEND, false );
}

void RenderPoint(float cx, float cy, float z, float _R, uint32_t RGBA, uint32_t RGBA2)
{

}

void RenderElements()
{
	// == Global Function == //
	// -> Render the particles/decals (Snow, dirt, blood, etc)
	if (!g->PARTICLES) return;

	oglSetTexture( 0 );

	oglSetRenderState( GL_CULL_FACE_MODE, GL_BACK );
	oglSetRenderState( GL_TEXTURE_2D, GL_FALSE );

	int fproc1 = 0;

	for (int eg = 0; eg<ElCount; eg++)
	{
		for (int e = 0; e<Elements[eg].ECount; e++)
		{
			vec3 rpos;
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
		vec3 rpos = BloodTrail.Trail[b].pos;
		uint32_t A1 = (0xE0 * BloodTrail.Trail[b].LTime / 20000); if (A1>0xE0) A1=0xE0;
		uint32_t A2 = (0x20 * BloodTrail.Trail[b].LTime / 20000); if (A2>0x20) A2=0x20;
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
		RenderCircle(sx, sy, rpos.z, -12*CameraW*0.64f / rpos.z, (A1<<24)+conv_xGx(0x000070), (A2<<24)+conv_xGx(0x000070));
		fproc1+=8;

		if (fproc1>256)
		{
			/*d3dFlushBuffer(fproc1, 0);*/
			fproc1 = 0;
		}
	}

	/*if (fproc1) d3dFlushBuffer(fproc1, 0);*/

    if (g->SNOW)
    for (int s=0; s<SnCount; s++)
	{
		vec3 rpos = Snow[s].pos;


        rpos.x = rpos.x - CameraX;
		rpos.y = rpos.y - CameraY;
	    rpos.z = rpos.z - CameraZ;


		rpos = RotateVector(rpos);
        if (rpos.z > -64) continue;
        if ( fabs(rpos.x) > -rpos.z) continue;
        if ( fabs(rpos.y) > -rpos.z) continue;

        float sx = VideoCXf - CameraW * rpos.x / rpos.z;
	    float sy = VideoCYf + CameraH * rpos.y / rpos.z;

		uint32_t A1 = 0xFF;
		uint32_t A2 = 0x30;
		if (Snow[s].ftime) {
			A1 = A1 * (uint32_t)(2000-Snow[s].ftime) / 2000;
			A2 = A2 * (uint32_t)(2000-Snow[s].ftime) / 2000;
		}

        /*if (!fproc1) d3dStartBuffer();*/

		if (rpos.z>-1624)
		{
			RenderCircle(sx, sy, rpos.z, -8*CameraW*0.64f / rpos.z, (A1<<24)+conv_xGx(0xF0F0F0), (A2<<24)+conv_xGx(0xB0B0B0));
			fproc1+=8;
        }
		else
		{
			RenderPoint(sx, sy, rpos.z, -8*CameraW*0.64f / rpos.z, (A1<<24)+conv_xGx(0xF0F0F0), (A2<<24)+conv_xGx(0xB0B0B0));
			fproc1++;
        }

		if (fproc1>256)
		{
			/*d3dFlushBuffer(fproc1, 0);*/
			fproc1 = 0;
		}
	}

    /*if (fproc1) d3dFlushBuffer(fproc1, 0);*/

	if (g->RAIN)
    for (int s=0; s<RainCount; s++) {
		vec3 rpos = Rain[s].pos;


        rpos.x = rpos.x - CameraX;
		rpos.y = rpos.y - CameraY;
	    rpos.z = rpos.z - CameraZ;


		rpos = RotateVector(rpos);
        if (rpos.z > -64) continue;
        if ( fabs(rpos.x) > -rpos.z) continue;
        if ( fabs(rpos.y) > -rpos.z) continue;

        float sx = VideoCXf - CameraW * rpos.x / rpos.z;
	    float sy = VideoCYf + CameraH * rpos.y / rpos.z;

		uint32_t A1 = 0xFF;
		uint32_t A2 = 0x30;

		if (Rain[s].ftime)
		{
			A1 = A1 * (uint32_t)(2000-Rain[s].ftime) / 2000;
			A2 = A2 * (uint32_t)(2000-Rain[s].ftime) / 2000;
		}

        //if (!fproc1) d3dStartBuffer();

        if (rpos.z>-1624) //0x3F6060 //0x152020
		{
			RenderCircle(sx, sy, rpos.z, -8*CameraW*0.64f / rpos.z, ((A1/2)<<24)+conv_xGx(0xFF9000), ((A2/2)<<24)+conv_xGx(0xDF6000));
			fproc1+=8;
        }
		else
		{
			RenderPoint(sx, sy, rpos.z, -8*CameraW*0.64f / rpos.z, ((A1/2)<<24)+conv_xGx(0xFF9000), ((A2/2)<<24)+conv_xGx(0xDF6000));
			fproc1++;
        }

		if (fproc1>256)
		{
			//d3dFlushBuffer(fproc1, 0);
			fproc1 = 0;
		}
	}

    /*if (fproc1) d3dFlushBuffer(fproc1, 0);*/

	oglSetRenderState( GL_CULL_FACE_MODE, GL_FRONT );
	oglSetRenderState( GL_TEXTURE_2D, GL_TRUE );

    GlassL = 0;
}


void RenderCharacterPost(TCharacter *cptr)
{
	// -> Render the Characters (AI) in the game world
	CreateChMorphedModel(cptr);

	float zs = (float)sqrtf( cptr->rpos.x*cptr->rpos.x  +  cptr->rpos.y*cptr->rpos.y  +  cptr->rpos.z*cptr->rpos.z);
	if (zs > ctViewR*256) return;

	GlassL = 0;

	if (zs > 256 * (ctViewR-4)) GlassL = (int)min(255.0f, (zs/4 - 64*(ctViewR-4)));

	g->waterclip = false;


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


	if (!g->SHADOWS3D) return;
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
	if (SupplyShip.State==-1) return;
	GlassL = 0;
	zs = (int)VectorLength(SupplyShip.rpos);
	if (zs > 256 * (ctViewR)) return;

	if (zs > 256 * (ctViewR-4))
	GlassL = min(255,(int)(zs - 256 * (ctViewR-4)) / 4);

	//CreateMorphedModel(SShipModel.mptr, &SShipModel.Animation[0], SupplyShip.FTime, 1.0);

	if ( fabsf(SupplyShip.rpos.z) < 4000)
	{
		RenderModelClip(SShipModel.mptr,SupplyShip.rpos.x, SupplyShip.rpos.y, SupplyShip.rpos.z, 210, 0, -SupplyShip.alpha -pi/2 + CameraAlpha, CameraBeta);
	}
	else
	{
		RenderModel(SShipModel.mptr,SupplyShip.rpos.x, SupplyShip.rpos.y, SupplyShip.rpos.z, 210, 0, -SupplyShip.alpha -pi/2+ CameraAlpha, CameraBeta);
	}
}

void RenderAmmoBag()
{
	if (AmmoBag.State==-1) return;
	GlassL = 0;
	zs = (int)VectorLength(AmmoBag.rpos);
	if (zs > 256 * (ctViewR)) return;

	if (zs > 256 * (ctViewR-4))
	GlassL = min(255,(int)(zs - 256 * (ctViewR-4)) / 4);

	CreateMorphedModel(AmmoModel.mptr, &AmmoModel.Animation[0], AmmoBag.FTime, 1.0);

	if ( fabsf(AmmoBag.rpos.z) < 4000)
	{
		RenderModelClip(AmmoModel.mptr,AmmoBag.rpos.x, AmmoBag.rpos.y, AmmoBag.rpos.z, 210, 0, -AmmoBag.alpha -pi/2 + CameraAlpha, CameraBeta);
	}
	else
	{
		RenderModel(AmmoModel.mptr,AmmoBag.rpos.x, AmmoBag.rpos.y, AmmoBag.rpos.z, 210, 0, -AmmoBag.alpha -pi/2+ CameraAlpha, CameraBeta);
	}
}


void Render3DHardwarePosts()
{
	// -> Render g->MODELS, ships, and ammo bags

	uint32_t start = glfwGetTime()*1000.0;

	TCharacter *cptr;
	for (int c=0; c<ChCount; c++)
	{
		cptr = &Characters[c];
		cptr->rpos.x = cptr->pos.x - CameraX;
		cptr->rpos.y = cptr->pos.y - CameraY;
		cptr->rpos.z = cptr->pos.z - CameraZ;


		float r = (float)max( fabsf(cptr->rpos.x), fabsf(cptr->rpos.z) );
		int ri = -1 + (int)(r / 256.f + 0.5f);
		if (ri < 0) ri = 0;
		if (ri > ctViewR) continue;

		if (g->FOGON) CalcFogLevel_Gradient(cptr->rpos);

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
   float r = (float)af_max( fabs(Ship.rpos.x), fabsf(Ship.rpos.z) );

   int ri = -1 + (int)(r / 256.f + 0.2f);
   if (ri < 0) ri = 0;
   if (ri < ctViewR)
   {
	  if (g->FOGON) CalcFogLevel_Gradient(Ship.rpos);

      Ship.rpos = RotateVector(Ship.rpos);
      if (Ship.rpos.z > BackViewR) goto NOSHIP;
      if ( fabsf(Ship.rpos.x) > -Ship.rpos.z + BackViewR ) goto NOSHIP;

      RenderShipPost();
   }
NOSHIP: ;


   if (!SupplyShip.State) goto NOSHIP2;
   SupplyShip.rpos.x = SupplyShip.pos.x - CameraX;
   SupplyShip.rpos.y = SupplyShip.pos.y - CameraY;
   SupplyShip.rpos.z = SupplyShip.pos.z - CameraZ;
   r = (float)af_max( fabsf(SupplyShip.rpos.x), fabsf(SupplyShip.rpos.z) );

   ri = -1 + (int)(r / 256.f + 0.2f);
   if (ri < 0) ri = 0;
   if (ri < ctViewR)
   {
	  if (g->FOGON) CalcFogLevel_Gradient(SupplyShip.rpos);

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
   r = (float)af_max( fabs(AmmoBag.rpos.x), fabs(AmmoBag.rpos.z) );

   ri = -1 + (int)(r / 256.f + 0.2f);
   if (ri < 0) ri = 0;
   if (ri < ctViewR)
   {
	  if (g->FOGON) CalcFogLevel_Gradient(AmmoBag.rpos);

      AmmoBag.rpos = RotateVector(AmmoBag.rpos);
      if (AmmoBag.rpos.z > BackViewR) goto NOBAG;
      if ( fabs(AmmoBag.rpos.x) > -AmmoBag.rpos.z + BackViewR ) goto NOBAG;

      RenderAmmoBag();
   }
NOBAG: ;

   SunLight *= GetTraceK(SunScrX, SunScrY);

   CharacterTime = uint32_t(glfwGetTime()*1000.0) - start;
}

void RotateVVector(vec3& v)
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

	float d = (float)sqrtf(x*x + y*y);
    if (d<2048)
	{
		SunLight = (220.f- d*220.f/2048.f);
		if (SunLight>140) SunLight = 140;
		SunLight*=SkyTraceK;
	}


	if (d>812.f) d = 812.f;
	d = (2048.f + d) / 3048.f;
	d+=(1.f-SkyTraceK)/2.f;
	if (OptDayNight==2)  d=1.5;

    RenderModelSun(SunModel,  x*d, y*d, z*d, (int)(200.f* SkyTraceK));
}

void RenderSkyPlane2()
{
	vec3 v,vbase;
	vec3 tx,ty,nv;
	float p,q, qx, qy, qz, px, py, pz, rx, ry, rz, ddx, ddy;
	float lastdt = 0.f;

	oglSetRenderState( GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	oglSetRenderState( GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	oglSetTexture( SkyTexture.m_texid );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	nv.x = 512; nv.y = 4024; nv.z=0;
	int FogBase = (int)CalcFogLevel(nv);

	cb = (float)cos(CameraBeta);
	sb = (float)sin(CameraBeta);
	SKYDTime = RealTime & ((1<<18) - 1);

	float sh = - CameraY;
	if (MapMinY==10241024) MapMinY=0;
	sh = (float)((int)MapMinY)*ctHScale - CameraY;

	v.x = 0;
	v.z = (ctViewR*4.f)/5.f*256.f;
	v.y = sh;

	vbase.x = v.x;
	vbase.y = v.y * cb + v.z * sb;
	vbase.z = v.z * cb - v.y * sb;

	if (vbase.z < 128) vbase.z = 128;

	int scry = VideoCY - (int)(vbase.y / vbase.z * CameraH);

	if (scry<0) return;
   if (scry>WinEY+1) scry = WinEY+1;

   cb = (float)cosf(CameraBeta-0.15);
   sb = (float)sinf(CameraBeta-0.15);

   tx.x=0.002f; tx.y=0;    tx.z=0;
   ty.x=0.0f;    ty.y=0;    ty.z=0.002f;
   nv.x=0;       nv.y=-1.f; nv.z=0;

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

   int sx1 = - VideoCX;
   int sx2 = + VideoCX;

   float qx1 = qx * sx1 + qz;
   float qx2 = qx * sx2 + qz;
   float qyy;

   glDepthMask( GL_FALSE );
   oglSetRenderState( GL_BLEND, true );
   oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

   //float l = 255.f;
   gvtx[0].x = 0;
   gvtx[1].x = (float)WinEX+1;

   gvtx[0].fog = 0;
   gvtx[1].fog = 0;

   gvtx[0].z = (float)1.f;
   gvtx[1].z = (float)1.f;

   gvtx[0].rhw =  1.0f;
   gvtx[1].rhw =  1.0f;

	glBegin( GL_LINES );
	for (int sky=0; sky<=scry; sky++)
	{
		int sy = VideoCY - sky;
		qyy = qy * sy;

		q = qx1 + qyy;
		float fxa = (px * sx1 + py * sy + pz) / q;
		float fya = (rx * sx1 + ry * sy + rz) / q;

		q = qx2 + qyy;
		float fxb = (px * sx2 + py * sy + pz) / q;
		float fyb = (rx * sx2 + ry * sy + rz) / q;

		float dtt = (float)(SKYDTime) / 512.f;

		float dt = ((float)sqrtf( (fxb-fxa)*(fxb-fxa) + (fyb-fya)*(fyb-fya) ) / 0x40 ) - 5.f;
		if (g->UNDERWATER) dt=6 + dt*3;
		if (dt>10.f) dt = 10.f;
		if (dt<lastdt) dt = lastdt;
		lastdt = dt;

		float Alpha = max(dt*245.f/10.f, (float)FogBase);
		Alpha = dt * 245.f / 10.f;

		gvtx[0].y = (float)sky;
		gvtx[0].tu = (fxa + dtt) * 2.0f;
		gvtx[0].tv = (fya - dtt) * 2.0f;

		gvtx[1].y = (float)sky;
		gvtx[1].tu = (fxb + dtt) * 2.0f;
		gvtx[1].tv = (fyb - dtt) * 2.0f;

		float _tt = 1.0f-(Alpha / 255.0f);
		float R = 1.0f;
		float G = 1.0f;
		float B = 1.0f;

		if ( g->RAIN )
		{
			R = 0.3f;
			G = 0.3f;
			B = 0.3f;
		}

		if ( _tt <= 0.05f ) continue;

		glColor4f( R,G,B, _tt );

		glTexCoord2f( gvtx[0].tu / 256.0f, gvtx[0].tv / 256.0f );
		glVertex4f( gvtx[0].x,gvtx[0].y,gvtx[0].z, 1.0f );
		glTexCoord2f( gvtx[1].tu / 256.0f, gvtx[1].tv / 256.0f );
		glVertex4f( gvtx[1].x,gvtx[1].y,gvtx[1].z, 1.0f );
		dFacesCount++;
	}
	glEnd();
	DrawCalls+=scry;

	cb = (float)cosf(CameraBeta);
	sb = (float)sinf(CameraBeta);

	nv = RotateVector(Sun3dPos);
	SunLight = 0;
	if (nv.z < -2024) RenderSun(nv.x, nv.y, nv.z);

	oglSetRenderState( GL_BLEND, false );
	glDepthMask( GL_TRUE );
}

void RenderSkyPlane(void)
{
	oglSetRenderState( GL_ALPHA_TEST, false );

	if ( !LoDetailSky )
	{
		RenderSkyPlane2();
		return;
	}

	oglSetTexture( SkyTexture.m_texid );
	glTexEnvf( GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	oglSetRenderState(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	oglSetRenderState(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	vec3 v,vbase;
    vec3 tx,ty,nv;
    float p,q, qx, qy, qz, px, py, pz, rx, ry, rz, ddx, ddy;
    //float lastdt = 0.f;

    nv.x = 512; nv.y = 4024; nv.z=0;

	cb = (float)cosf(CameraBeta);
	sb = (float)sinf(CameraBeta);
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

	cb = (float)cosf(CameraBeta-0.15f);
	sb = (float)sinf(CameraBeta-0.15f);

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
	alpha = 255;

	int sx1 = - VideoCX;
	int sx2 = + VideoCX;
	int sx3 = 0;

	float qx1 = qx * sx1 + qz;
	float qx2 = qx * sx2 + qz;
	float qx3 = qx * sx3 + qz;
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
	q = qx3 + qyy;
	float fxc = (px * sx3 + py * sy + pz) / q;
	float fyc = (rx * sx3 + ry * sy + rz) / q;

	oglStartBuffer();

	lpVertex->x       = 0.f;
    lpVertex->y       = (float)sky;
    lpVertex->z       = 1.0f;//-8.f / za;
    lpVertex->rhw      = 1.0f / 1.0f;// 128.f / za;
	lpVertex->color    = 0x00FFFFFF | (alpha<<24);
    lpVertex->tu       = 0;//(fxa + dtt) / 256.f;//(fxa + dtt) / 256.f;
    lpVertex->tv       = 0 - dtt;//(fya - dtt) / 256.f;//(fya - dtt) / 256.f;
	lpVertex++;

	lpVertex->x       = (float)WinW;
    lpVertex->y       = (float)sky;
    lpVertex->z       = 1.0f;//-8.f / za;
    lpVertex->rhw      = 1.0f / 1.0f;//128.f / za;
	lpVertex->color    = 0x00FFFFFF | (alpha<<24);
    lpVertex->tu       = 1;//(fxb + dtt) / 256.f;//(fxb + dtt) / 256.f;
    lpVertex->tv       = 0 - dtt;//(fyb - dtt) / 256.f;//(fyb - dtt) / 256.f;
	lpVertex++;


	sky=scry/2.f;
	sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	fxa = (px * sx1 + py * sy + pz) / q;
	fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	fxb = (px * sx2 + py * sy + pz) / q;
	fyb = (rx * sx2 + ry * sy + rz) / q;

	lpVertex->x       = 0.f;
    lpVertex->y       = (float)sky;
    lpVertex->z       = 1.0f;//-8.f / zb;
    lpVertex->rhw      = 1.0f / 1.0f;//128.f / zb;
	lpVertex->color    = 0x00FFFFFF | (alphb<<24);
	lpVertex->tu       = 0;//(fxa + dtt) / 256.f;//(fxa + dtt) / 256.f;
    lpVertex->tv       = 0.5f - dtt;//(fya - dtt) / 256.f;//(fya - dtt) / 256.f;
	lpVertex++;

	lpVertex->x       = (float)WinW;// * ( (float)(WinH/2) / sky );
    lpVertex->y       = (float)sky;
    lpVertex->z       = 1.0f;//-8.f / zb;
    lpVertex->rhw      = 1.0f / 1.0f;//128.f / zb;
	lpVertex->color    = 0x00FFFFFF | (alphb<<24);
	lpVertex->tu       = 1;//(fxb + dtt) / 256.f;//(fxb + dtt) / 256.f;
    lpVertex->tv       = 0.5f - dtt;//(fyb - dtt) / 256.f;
	lpVertex++;


	sky=(float)scry;
	sy = VideoCY - sky;
	qyy = qy * sy;
	q = qx1 + qyy;
	fxa = (px * sx1 + py * sy + pz) / q;
	fya = (rx * sx1 + ry * sy + rz) / q;
	q = qx2 + qyy;
	fxb = (px * sx2 + py * sy + pz) / q;
	fyb = (rx * sx2 + ry * sy + rz) / q;

	lpVertex->x       = 0.0f;
    lpVertex->y       = (float)sky;
    lpVertex->z       = 1.0f;//-8.f / zb;
    lpVertex->rhw      = 1.0f / 1.0f;//128.f / zc;
	lpVertex->color    = 0x00FFFFFF | (alphc<<24);
    lpVertex->tu       = 0.5f * (-cb);//(fxa + dtt) / 256.f;//(fxa + dtt) / 256.f;
    lpVertex->tv       = 1.0f - dtt;//(fya - dtt) / 256.f;//(fya - dtt) / 256.f;
	lpVertex++;

	lpVertex->x       = (float)WinW;
    lpVertex->y       = (float)sky;
    lpVertex->z       = 1.0f;
    lpVertex->rhw      = 1.0f / 1.0f;
	lpVertex->color    = 0x00FFFFFF | (alphc<<24);
	lpVertex->tu       = 0.5f * cb;//(fxb + dtt) / 256.f;
    lpVertex->tv       = 1.0f - dtt;//(fyb - dtt) / 256.f;//(fyb - dtt) / 256.f;
	lpVertex++;


	oglSetRenderState( GL_BLEND, false );
	oglSetRenderState( GL_DEPTH_TEST, false );
	oglSetRenderState( GL_BLEND_SRC, GL_SRC_ALPHA );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

	oglDrawTriangle(pGLVertices[0],pGLVertices[1],pGLVertices[2]);
	oglDrawTriangle(pGLVertices[1],pGLVertices[2],pGLVertices[3]);
	oglDrawTriangle(pGLVertices[2],pGLVertices[3],pGLVertices[4]);
	oglDrawTriangle(pGLVertices[3],pGLVertices[4],pGLVertices[5]);

	oglSetRenderState( GL_BLEND, false );
	oglSetRenderState( GL_DEPTH_TEST, true );


	nv = RotateVector(Sun3dPos);
	SunLight = 0;
	if (nv.z < -2024) RenderSun(nv.x, nv.y, nv.z);
}

void Render_Cross(int sx, int sy)
{
	// -> Render a crosshair on the screen

	float w = (float)WinW / 12.0f;

	oglSetRenderState( GL_TEXTURE_2D, GL_FALSE );
	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );

	glLineWidth( float(WinW) / 800.0f );

	if ( WeapInfo[CurrentWeapon].CrossHairType == 0 ) //Cross
	{
		if ( !g->OPTICMODE ) w = (w / WeapInfo[CurrentWeapon].Prec) / 4.0f;
		oglDrawLine( sx-w+1, sy+1, sx+w+1, sy+1, 0x80080808 );
		oglDrawLine( sx+1, sy-w+1, sx+1, sy+w+1, 0x80080808 );
		oglDrawLine( sx-w, sy, sx+w, sy, ABGR_to_ARGB(WeapInfo[CurrentWeapon].CrossHairColor) );
		oglDrawLine( sx, sy-w, sx, sy+w, ABGR_to_ARGB(WeapInfo[CurrentWeapon].CrossHairColor) );
	}
	else if ( WeapInfo[CurrentWeapon].CrossHairType == 1 ) //Circle
	{
		if ( !g->OPTICMODE ) w = (w / WeapInfo[CurrentWeapon].Prec) / 8.0f;
		oglDrawCircle( sx, sy, w, ABGR_to_ARGB(WeapInfo[CurrentWeapon].CrossHairColor) );
		oglDrawCircle( sx+1, sy+1, w, 0x80080808 );
	}
	else if ( WeapInfo[CurrentWeapon].CrossHairType == 2 ) //Delta
	{
		if ( !g->OPTICMODE ) w = (w / WeapInfo[CurrentWeapon].Prec) / 1.0f;
		oglDrawLine( sx, sy, sx+w, sy+w, 0x80080808 );
		oglDrawLine( sx, sy, sx-w, sy+w, 0x80080808 );
		oglDrawLine( sx, sy, sx+w, sy+w, ABGR_to_ARGB(WeapInfo[CurrentWeapon].CrossHairColor) );
		oglDrawLine( sx, sy, sx-w, sy+w, ABGR_to_ARGB(WeapInfo[CurrentWeapon].CrossHairColor) );
	}
	else if ( WeapInfo[CurrentWeapon].CrossHairType == 3 ) //Aimdot
	{
		if ( !g->OPTICMODE ) w = (w / WeapInfo[CurrentWeapon].Prec) / 8.0f;
		RenderCircle2( sx, sy, 1.0f, w, 0xFFFF0000, 0xFFFF0000 );
	}

	 glLineWidth( 1.0f );

	oglSetRenderState( GL_BLEND, false );
	oglSetRenderState( GL_TEXTURE_2D, GL_TRUE );
}

void Render_Text(int x, int y,char *text, unsigned long color)
{
	// -> Render text
	oglTextOut(x,y,text,color);
}

void Render_LifeInfo(int li)
{
	// -> Render the targeted animal's info
	int x,y;
	int ctype = Characters[li].CType;
	char t[32];

    x = VideoCX + WinW / 64;
	y = VideoCY + (int)(WinH / 6.8);

    oglTextOut(x, y, DinoInfo[ctype].Name, 0xFF00b000);

	if (OptSys) sprintf(t,"Weight: %3.2ft ", DinoInfo[ctype].Mass * Characters[li].scale * Characters[li].scale / 0.907f);
	else        sprintf(t,"Weight: %3.2fT ", DinoInfo[ctype].Mass * Characters[li].scale * Characters[li].scale);

	oglTextOut(x, y+16, t, 0xFF00b000);

	int R  = (int)(VectorLength( SubVectors(Characters[li].pos, PlayerPos) )*3 / 64.f);
	if (OptSys) sprintf(t,"Distance: %dft ", R);
	else        sprintf(t,"Distance: %dm  ", R/3);

	oglTextOut(x, y+32, t, 0xFF00b000);

	if ( g->DEBUG )
	{
		sprintf( t, "Health: %d/%d", Characters[li].Health, DinoInfo[ctype].Health0 );
	}
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

	oglTextOut(x, y0   , "Name: ", 0xFFBFBFBF);  x+=GetTextW(0,"Name: ");
	if ( !g->CHEATED )
		oglTextOut(x, y0   , DinoInfo[dtype].Name, 0xFF00BFBF);
	else
		oglTextOut(x, y0   , DinoInfo[dtype].Name, 0xFF0000F0);

	x = x0;
	oglTextOut(x, y0+16, "Weight: ", 0xFFBFBFBF);  x+=GetTextW(0,"Weight: ");

	if (OptSys)	sprintf(t,"%3.2ft ", DinoInfo[dtype].Mass * scale * scale / 0.907);
	else		sprintf(t,"%3.2fT ", DinoInfo[dtype].Mass * scale * scale);

    oglTextOut(x, y0+16, t, 0xFF00BFBF);    x+=GetTextW(0,t);
    oglTextOut(x, y0+16, "Length: ", 0xFFBFBFBF); x+=GetTextW(0,"Length: ");

	if (OptSys) sprintf(t,"%3.2fft", DinoInfo[dtype].Length * scale / 0.3);
	else		sprintf(t,"%3.2fm", DinoInfo[dtype].Length * scale);

	oglTextOut(x, y0+16, t, 0xFF00BFBF);

	x = x0;
	oglTextOut(x, y0+32, "Weapon: ", 0xFFBFBFBF);  x+=GetTextW(0,"Weapon: ");

	sprintf(t,"%s    ", WeapInfo[wep].Name);
	oglTextOut(x, y0+32, t, 0xFF00BFBF);   x+=GetTextW(0,t);
    oglTextOut(x, y0+32, "Score: ", 0xFFBFBFBF);   x+=GetTextW(0,"Score: ");

	sprintf(t,"%d", score);
	oglTextOut(x, y0+32, t, 0xFF00BFBF);


	x = x0;
	oglTextOut(x, y0+48, "Range of kill: ", 0xFFBFBFBF);  x+=GetTextW(0,"Range of kill: ");
	if (OptSys) sprintf(t,"%3.1fft", range / 0.3);
	else        sprintf(t,"%3.1fm", range);
    oglTextOut(x, y0+48, t, 0xFF00BFBF);


	x = x0;
	oglTextOut(x, y0+64, "Date: ", 0xFFBFBFBF);  x+=GetTextW(0,"Date: ");

	if (OptSys)	sprintf(t,"%d.%d.%d   ", ((date>>10) & 255), (date & 255), date>>20);
	else		sprintf(t,"%d.%d.%d   ", (date & 255), ((date>>10) & 255), date>>20);

    oglTextOut(x, y0+64, t, 0xFF00BFBF);   x+=GetTextW(0,t);
    oglTextOut(x, y0+64, "Time: ", 0xFFBFBFBF);   x+=GetTextW(0,"Time: ");
	sprintf(t,"%d:%02d", ((time>>10) & 255), (time & 255));
	oglTextOut(x, y0+64, t, 0xFF00BFBF);
}

void RenderModelClipEnvMap(TModel* _mptr, float x0, float y0, float z0, float al, float bt)
{
   int f,CMASK;
   mptr = _mptr;

   float ca = (float)cos(al);
   float sa = (float)sin(al);
   float cb = (float)cos(bt);
   float sb = (float)sin(bt);

   uint32_t PHCOLOR = 0xFFFFFFFF;

   bool BL = false;


	for (int s=0; s<mptr->VCount; s++)
	{

	rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
    float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
    rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
    rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
    if (rVertex[s].z<0) BL=true;

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

	oglSetTexture( TFX_ENVMAP.m_texid );

	BuildTreeClipNoSort();

	oglStartBuffer();

	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_BLEND_DST, GL_ONE );

	f = Current;
	int fproc1 = 0;

	while( f!=-1 )
	{

		vused = 3;
		TFacef *fptr = &mptr->gFace[f];
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
			//int v=0;
	   		 lpVertex->x       = VideoCXf - cp[0].ev.v.x / cp[0].ev.v.z * CameraW;
			 lpVertex->y       = VideoCYf + cp[0].ev.v.y / cp[0].ev.v.z * CameraH;
			 lpVertex->z       = 1.0f - ( cp[0].ev.v.z / ZSCALE );
			 lpVertex->rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			 lpVertex->color    = PHCOLOR;
			 lpVertex->fog		= 0.0f ;
			 lpVertex->tu       = (float)(cp[0].tx);
			 lpVertex->tv       = (float)(cp[0].ty);
			 lpVertex++;

	   		 lpVertex->x       = VideoCXf - cp[u+1].ev.v.x / cp[u+1].ev.v.z * CameraW;
			 lpVertex->y       = VideoCYf + cp[u+1].ev.v.y / cp[u+1].ev.v.z * CameraH;
			 lpVertex->z       = 1.0f - ( cp[u+1].ev.v.z / ZSCALE );
			 lpVertex->rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			 lpVertex->color    = PHCOLOR;
			 lpVertex->fog		= 0.0f;
			 lpVertex->tu       = (float)(cp[u+1].tx);
			 lpVertex->tv       = (float)(cp[u+1].ty);
			 lpVertex++;

	   		lpVertex->x       = VideoCXf - cp[u+2].ev.v.x / cp[u+2].ev.v.z * CameraW;
			lpVertex->y       = VideoCYf + cp[u+2].ev.v.y / cp[u+2].ev.v.z * CameraH;
			lpVertex->z       = 1.0f - ( cp[u+2].ev.v.z / ZSCALE);
			lpVertex->rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			lpVertex->color    = PHCOLOR;
			lpVertex->fog		= 0.0f;
			lpVertex->tu       = (float)(cp[u+2].tx);
			lpVertex->tv       = (float)(cp[u+2].ty);
			lpVertex++;

			//oglDrawTriangle(gvtx[0],gvtx[1],gvtx[2]);

			fproc1++;
		}
LNEXT:
		f = mptr->gFace[f].Next;

		if ( fproc1 >= 2048 )
		{
			oglFlushBuffer(fproc1, 0);
			oglStartBuffer();
			fproc1 = 0;
		}
	}

	oglFlushBuffer(fproc1, 0);

	oglSetRenderState( GL_BLEND, GL_FALSE );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );
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
   uint32_t PHCOLOR = 0xFF000000 + (rv<<16) + (gv<<8) + bv;

   bool BL = false;

   for (int s=0; s<mptr->VCount; s++) {

	rVertex[s].x = (mptr->gVertex[s].x * ca + mptr->gVertex[s].z * sa) /* * mdlScale */ + x0;
    float vz = mptr->gVertex[s].z * ca - mptr->gVertex[s].x * sa;
    rVertex[s].y = (mptr->gVertex[s].y * cb - vz * sb) /* * mdlScale */ + y0;
    rVertex[s].z = (vz * cb + mptr->gVertex[s].y * sb) /* * mdlScale */ + z0;
    if (rVertex[s].z<0) BL=true;

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


	oglSetTexture( TFX_SPECULAR.m_texid );

	BuildTreeClipNoSort();

	oglStartBuffer();

	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_BLEND_DST, GL_ONE );

	f = Current;
	int fproc1 = 0;


	while( f!=-1 )
	{
		vused = 3;
		TFacef *fptr = &mptr->gFace[f];
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
			//int v=0;
	   		lpVertex->x     = VideoCXf - cp[0].ev.v.x / cp[0].ev.v.z * CameraW;
			lpVertex->y     = VideoCYf + cp[0].ev.v.y / cp[0].ev.v.z * CameraH;
			lpVertex->z     = 1.0f - ( cp[0].ev.v.z / ZSCALE );
			lpVertex->rhw   = 1.0f;//lpVertex->sz * _AZSCALE;
			lpVertex->color = PHCOLOR;
			lpVertex->fog	= 0.0f;
			lpVertex->tu    = (float)(cp[0].tx);
			lpVertex->tv    = (float)(cp[0].ty);
			lpVertex++;

	   		lpVertex->x       = VideoCXf - cp[u+1].ev.v.x / cp[u+1].ev.v.z * CameraW;
			lpVertex->y       = VideoCYf + cp[u+1].ev.v.y / cp[u+1].ev.v.z * CameraH;
			lpVertex->z       = 1.0f - ( cp[u+1].ev.v.z / ZSCALE );
			lpVertex->rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			lpVertex->color    = PHCOLOR;
			lpVertex->fog		= 0.0f;
			lpVertex->tu       = (float)(cp[u+1].tx);
			lpVertex->tv       = (float)(cp[u+1].ty);
			lpVertex++;

	   		lpVertex->x       = VideoCX - cp[u+2].ev.v.x / cp[u+2].ev.v.z * CameraW;
			lpVertex->y       = VideoCY + cp[u+2].ev.v.y / cp[u+2].ev.v.z * CameraH;
			lpVertex->z       = 1.0f - ( cp[u+2].ev.v.z / ZSCALE );
			lpVertex->rhw      = 1.0f;//lpVertex->sz * _AZSCALE;
			lpVertex->color    = PHCOLOR;
			lpVertex->fog		= 0.0f;
			lpVertex->tu       = (float)(cp[u+2].tx);
			lpVertex->tv       = (float)(cp[u+2].ty);
			lpVertex++;

			fproc1++;
		}
LNEXT:
		f = mptr->gFace[f].Next;
		if ( fproc1 >= 1024 )
		{
			oglFlushBuffer( fproc1, 0 );
			oglStartBuffer();
			fproc1 = 0;
		}
	}

	oglFlushBuffer( fproc1, 0 );

	oglSetRenderState( GL_BLEND, GL_FALSE );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );
}

void RenderShadowClip(TModel* _mptr, float xm0, float ym0, float zm0, float x0, float y0, float z0, float cal, float al, float bt)
{
	//ToDo//
   int f,CMASK,j;
   mptr = _mptr;

   float cla = (float)cos(cal);
   float sla = (float)sin(cal);

   float ca = (float)cos(al);
   float sa = (float)sin(al);

   float cb = (float)cos(bt);
   float sb = (float)sin(bt);


   bool BL = false;
   for (int s=0; s<mptr->VCount; s++)
   {
    float mrx = mptr->gVertex[s].x * cla + mptr->gVertex[s].z * sla;
    float mrz = mptr->gVertex[s].z * cla - mptr->gVertex[s].x * sla;

    float shx = mrx + mptr->gVertex[s].y * SunShadowK;
    float shz = mrz + mptr->gVertex[s].y * SunShadowK;
    float shy = GetLandH(shx + xm0, shz + zm0) - ym0;

    rVertex[s].x = (shx * ca + shz * sa)   + x0;
    float vz = shz * ca - shx * sa;
    rVertex[s].y = (shy * cb - vz * sb) + y0;
    rVertex[s].z = (vz * cb + shy * sb) + z0;
    if (rVertex[s].z<0) BL=true;

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

	//float d = (float) sqrt(x0*x0 + y0*y0 + z0*z0);
	//if (LOWRESTX) d = 14*256;

	//pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );

	oglSetTexture( 0 );

	BuildTreeClipNoSort();

	oglStartBuffer();
	int fproc1 = 0;

	oglSetRenderState( GL_BLEND, g->TRANSPARENCY );
	oglSetRenderState( GL_BLEND_DST, GL_ONE_MINUS_SRC_ALPHA );

	f = Current;
	while( f!=-1 )
	{
		vused = 3;
		TFacef *fptr = &mptr->gFace[f];

		CMASK = 0;
		CMASK|=gScrp[fptr->v1].y;
		CMASK|=gScrp[fptr->v2].y;
		CMASK|=gScrp[fptr->v3].y;

		cp[0].ev.v = rVertex[fptr->v1];  cp[0].tx = fptr->tax;  cp[0].ty = fptr->tay;
		cp[1].ev.v = rVertex[fptr->v2];  cp[1].tx = fptr->tbx;  cp[1].ty = fptr->tby;
		cp[2].ev.v = rVertex[fptr->v3];  cp[2].tx = fptr->tcx;  cp[2].ty = fptr->tcy;

		if (CMASK == 0xFF)
		{
			for (u=0; u<vused; u++) cp[u].ev.v.z+=16.0f;
			for (u=0; u<vused; u++) ClipVector(ClipZ,u);
			for (u=0; u<vused; u++) cp[u].ev.v.z-=16.0f;
			if (vused<3) goto LNEXT;
		}

		if (CMASK & 1) for (u=0; u<vused; u++) ClipVector(ClipA,u); if (vused<3) goto LNEXT;
		if (CMASK & 2) for (u=0; u<vused; u++) ClipVector(ClipC,u); if (vused<3) goto LNEXT;
		if (CMASK & 4) for (u=0; u<vused; u++) ClipVector(ClipB,u); if (vused<3) goto LNEXT;
		if (CMASK & 8) for (u=0; u<vused; u++) ClipVector(ClipD,u); if (vused<3) goto LNEXT;

		for (j=0; j<vused-2; j++)
		{
			u = 0;
			lpVertex->x       = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			lpVertex->y       = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			lpVertex->z       = (ZSCALE+0.5f) / cp[u].ev.v.z;
			lpVertex->rhw      = 1.f;
			lpVertex->color    = 0x80000000;//GlassL;
			lpVertex->fog		= 0.0f ;
			lpVertex->tu       = 0.f;
			lpVertex->tv       = 0.f;
			lpVertex++;

			u = j+1;
			lpVertex->x       = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			lpVertex->y       = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			lpVertex->z       = (ZSCALE+0.5f) / cp[u].ev.v.z;
			lpVertex->rhw      = 1.f;
			lpVertex->color    = 0x80000000;//GlassL;
			lpVertex->fog		= 0.0f ;
			lpVertex->tu       = 0.f;
			lpVertex->tv       = 0.f;
			lpVertex++;

			u = j+2;
			lpVertex->x       = (float)(VideoCX - (int)(cp[u].ev.v.x / cp[u].ev.v.z * CameraW));
			lpVertex->y       = (float)(VideoCY + (int)(cp[u].ev.v.y / cp[u].ev.v.z * CameraH));
			lpVertex->z       = (ZSCALE+0.5f) / cp[u].ev.v.z;
			lpVertex->rhw      = 1.f;
			lpVertex->color    = 0x80000000;//GlassL;
			lpVertex->fog		= 0.0f ;
			lpVertex->tu       = 0.f;
			lpVertex->tv       = 0.f;
			lpVertex++;

			fproc1++;
		}



LNEXT:
		f = mptr->gFace[f].Next;

		if ( fproc1 >= 1024 )
		{
			oglFlushBuffer(fproc1, 0);
			oglStartBuffer();
			fproc1 = 0;
		}
	}

	oglFlushBuffer(fproc1, 0);
	oglSetRenderState( GL_BLEND, false );
}

bool CopyHARDToDIB(void)
{
	// -> Copy the buffer to a BMP canvas
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer( GL_FRONT );

	if ( lpScreenBuf ) free( lpScreenBuf );
	lpScreenBuf = malloc( sizeof(uint32_t) * ( WinW * WinH) );

	//float* lpDepthBuf = new float [ WinW * WinH ];

	glReadPixels( 0,0, WinW,WinH, GL_BGR, GL_UNSIGNED_BYTE, lpScreenBuf );
	//glReadPixels( 0,0, WinW,WinH, GL_DEPTH_COMPONENT, GL_FLOAT, lpDepthBuf );

	glReadBuffer( GL_BACK );

	/*for ( int i=0; i<WinW*WinH; ++i )
	{
		float d = lpDepthBuf[i];
		*( (uint32_t*)lpScreenBuf + i ) = Color_ARGB( 255*d,255*d,255*d, 255 );
	}

	delete [] lpDepthBuf;*/

	FILE* hf = 0;                  /* file handle */
    TARGAFILEHEADER tga;

	tga.tgaIdentSize		= 0;
	tga.tgaColorMapType		= 0;
	tga.tgaImageType		= TARGA_RGBA;

	tga.tgaColorMapOffset	= 0;
	tga.tgaColorMapLength	= 0;
	tga.tgaColorMapBits		= 0;

	tga.tgaXStart			= 0;
	tga.tgaYStart			= 0;
	tga.tgaWidth			= WinW;
	tga.tgaHeight			= WinH;
	tga.tgaBits				= 24;
	tga.tgaDescriptor		= 0;

	CreateDirectory( "SCREENSHOTS", 0 );

    sprintf( logt, "SCREENSHOTS/AF_SCREEN%04d.TGA", ++_shotcounter );
    hf = fopen( logt, "wb" );
    if ( hf != 0 )
    {
		fwrite( &tga, sizeof(TARGAFILEHEADER), 1, hf );
		fwrite( lpScreenBuf, WinW*WinH*3, 1, hf );
		fclose(hf);
		return true;
	}

	return false;
}

void DrawHMap()
{
	// -> Render the player map
	int xx,yy;

	// == Draw the map picture
	DrawPicture(VideoCX-MapPic.m_width/2, VideoCY - MapPic.m_height/2-6, MapPic);

	oglSetRenderState( GL_TEXTURE_2D, GL_FALSE );
	oglSetRenderState( GL_DEPTH_TEST, GL_FALSE );

	// == Draw the player
	xx = VideoCX - 128 + (CCX>>2);
	yy = VideoCY - 128 + (CCY>>2);
	if (yy<0 || yy>=WinH) goto endmap;
	if (xx<0 || xx>=WinW) goto endmap;
	oglDrawBox(xx-1,yy-1,xx+1,yy+1,0xFF00FF00);
	oglDrawCircle(int(xx), int(yy), int(ctViewR / 4.0f), 0xFF00FF00);
	//oglDrawLine(xx,yy,xx+R,yy+R,0xFF00FF00);

	// == Draw the huntable creatures
	if (RadarMode)
	for (int c=0; c<ChCount; c++)
	{
		uint32_t color = 0xFF0000FF;

		//if (Characters[c].AI<10) continue;
		//if (! (TargetDino & (1<<Characters[c].AI)) ) continue;

		if (! (TargetDino & (1<<Characters[c].AI)) ) color = 0xFFFF0000;
		if ( Characters[c].AI < 10 ) color = 0xFF00FF00;

		if (!Characters[c].Health) continue;
		xx = VideoCX - 128 + (int)Characters[c].pos.x / 1024;
		yy = VideoCY - 128 + (int)Characters[c].pos.z / 1024;
		if (yy<=0 || yy>=WinH) goto endmap;
		if (xx<=0 || xx>=WinW) goto endmap;
		oglDrawBox(xx-1,yy-1, xx+1, yy+1, color);
	}

	// == Draw the dropship
	xx = VideoCX - 128 + (int)Ship.pos.x / 1024;
	yy = VideoCY - 128 + (int)Ship.pos.z / 1024;
	if (xx>=1 && yy>=1)
	{
		oglDrawBox(xx-1,yy-1, xx+1, yy+1, 0xFF0080FF);
	}

	xx = VideoCX - 128 + (int)SupplyShip.pos.x / 1024;
	yy = VideoCY - 128 + (int)SupplyShip.pos.z / 1024;
	if (xx>=1 && yy>=1)
	{
		oglDrawBox(xx-1,yy-1, xx+1, yy+1, 0xFF0080FF);
	}

endmap:

	oglSetRenderState( GL_DEPTH_TEST, GL_TRUE );
	oglSetRenderState( GL_TEXTURE_2D, GL_TRUE );
	return;
}


void Hardware_ZBuffer(bool bl)
{
	// == Global Function == //
	// -> Toggle zbuffer
	// -> Used by DrawPostObjects so that all objects rendered appear close?
	if (!bl)
	{
		//glClearDepth(0.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

void AddSkySum(uint16_t C)
{
  int R,G,B;

 // if (VMFORMAT565) {
	  R = C>>11;  G = (C>>5) & 63; B = C & 31;
  //} else {
	  //R = C>>10; G = (C>>5) & 31; B = C & 31; C=C*2;
 // }

  SkySumR += R*8;
  SkySumG += G*4;
  SkySumB += B*8;
}

float GetSkyK(int x, int y)
{
	/*
		Maybe replace this by comparing screenspace co-ordinates of geometry against the center of the sun?
		the game engine already translates to screenspace, so may as well use it.
	*/
	return SkyTraceK = 1.0f;
    /*if (x<10 || y<10 || x>WinW-10 || y>WinH-10) return 0.5;
    SkySumR = 0;
    SkySumG = 0;
    SkySumB = 0;
    float k = 0;

    uint16_t *lpSurface = new uint16_t[WinW*WinH];
    ZeroMemory(lpSurface,WinW*2*WinH);
    glReadPixels(0,0,WinW,WinH,GL_BGR,GL_UNSIGNED_SHORT_5_6_5_REV,lpSurface);

    int bw = WinW;//WinW * sizeof(uint16_t);
    AddSkySum(lpSurface[(y+0)*bw + x+0]);
    AddSkySum(lpSurface[(y+6)*bw + x+0]);
    AddSkySum(lpSurface[(y-6)*bw + x+6]);
    AddSkySum(lpSurface[(y+0)*bw + x-6]);
    AddSkySum(lpSurface[(y+0)*bw + x+0]);

    AddSkySum(lpSurface[(y+4)*bw + x+4]);
    AddSkySum(lpSurface[(y+4)*bw + x-4]);
    AddSkySum(lpSurface[(y-4)*bw + x+4]);
    AddSkySum(lpSurface[(y-4)*bw + x-4]);

    delete [] lpSurface;

	k = (float)sqrtf(SkySumR*SkySumR + SkySumG*SkySumG + SkySumB*SkySumB) / 9;

    if (k>80) k = 80;
    if (k<  0) k = 0;
    k = 1.0f - k/80.f;
    if (k<0.2) k=0.2f;
    if (OptDayNight==2) k=0.3f + k/2.75f;
    DeltaFunc(SkyTraceK, k, (0.07f + (float)fabsf(k-SkyTraceK)) * (TimeDt / 512.f) );

    return SkyTraceK;*/
}

float GetTraceK(int x, int y)
{
	return TraceK = 1.0f;
	/*if (x<8 || y<8 || x>WinW-8 || y>WinH-8) return 0.f;

	float k = 0;

	uint16_t *lpSurface = new uint16_t[WinW*WinH];
    ZeroMemory(lpSurface,WinW*2*WinH);
    glReadPixels(0,0,WinW,WinH,GL_BGR,GL_UNSIGNED_SHORT_5_6_5_REV,lpSurface);

	uint16_t CC = 200;
	int bw = WinW;
	if ( *(lpSurface + (y+0 )*bw + x+0 ) < CC ) k+=1.f;
	if ( *(lpSurface + (y+10)*bw + x+0 ) < CC ) k+=1.f;
	if ( *(lpSurface + (y-10)*bw + x+0 ) < CC ) k+=1.f;
	if ( *(lpSurface + (y+0 )*bw + x+10) < CC ) k+=1.f;
	if ( *(lpSurface + (y+0 )*bw + x-10) < CC ) k+=1.f;

	if ( *(lpSurface + (y+8)*bw + x+8) < CC ) k+=1.f;
	if ( *(lpSurface + (y+8)*bw + x-8) < CC ) k+=1.f;
	if ( *(lpSurface + (y-8)*bw + x+8) < CC ) k+=1.f;
	if ( *(lpSurface + (y-8)*bw + x-8) < CC ) k+=1.f;

	delete [] lpSurface;
	k/=9;

	DeltaFunc(TraceK, k, TimeDt / 1024.f);
	return TraceK;*/
}

int RenCreateTexture(bool alpha, bool mipmaps, int w, int h, int bpp, void *data)
{
    //oglCreateTexture(alpha, mipmaps, w, h, bpp, data);
	return 0;
}

