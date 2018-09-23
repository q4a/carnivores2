
#ifndef OPENGL_EXTRA_H
#define OPENGL_EXTRA_H

#define GLEW_STATIC

#include "Hunt.h"
#include <assert.h>
#include <stdio.h>
#include "VideoOptions.h"


#ifdef OPENGL_VARS
#define GLVAR
#else
#define GLVAR extern
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif


class GLVertex
{
    public:

    GLVertex(){}
    GLVertex(float X,float Y,float Z,float RHW,uint32_t C,float TU,float TV) : x(X), y(Y), z(Z), rhw(RHW), color(C), tu(TU), tv(TV)
    {}

	float	x,
			y,
			z,
			rhw;
	uint32_t	color;
	float	fog;
	float	tu,tv;
};

typedef struct _GLTriangle
{
	uint32_t V1,V2,V3;
} GLTriangle;

class GLshader
{
    public:

    GLshader(){}
    GLshader(GLuint V, GLuint F, GLuint P)
    {
        this->VSH = V;
        this->FSH = F;
        this->Program = P;
    }

    GLuint VSH,FSH,Program;
};


uint32_t DarkRGBX( uint32_t c );
uint32_t Color_ARGB( BYTE R, BYTE G, BYTE B, BYTE A );

bool oglInit();
bool oglDeinit();
unsigned int oglLoadShader(char* fname);
void oglCreateFont();
void oglGetCaps();
void oglDeleteFont();
void oglDrawTriangle(GLVertex &vtx1, GLVertex &vtx2, GLVertex &vtx3);
void oglSetRenderState( GLenum State, GLuint Value );
void oglStartBufferBMP();
void oglStartBuffer( );
void oglEndBuffer( );
void oglFlushBuffer( int fproc1, int fproc2 );
void oglDrawBox(int x1, int y1, int x2, int y2, uint32_t color);
void oglDrawLine(float x1,float y1,float x2,float y2,uint32_t color);
void oglDrawCircle(int x,int y,int r, uint32_t color);
void oglSetTexture( GLuint );
void oglTextOut(int x,int y, char *text,uint32_t color);
void GLTextureFilter(GLint Min, GLint Mag);
void oglClearBuffers(void);
int oglCreateSprite(bool Alpha, TPicture &pic);
bool oglCreateTexture(bool alpha, bool mipmaps, CTexture* tptr );
void oglAddVertex( float x, float y, float z, float rhw, float fog, unsigned long diffuse, float tu, float tv );
bool oglPointInTriangle( Vector2df A, Vector2df B, Vector2df C, Vector2df P );
bool oglIsTextureBound( GLuint p_texture );


GLVAR GLuint					DrawCalls;
GLVAR GLuint					TextureBinds;

GLVAR GLuint					CurrentTexture,
								CurrentTextureIndex,
								CurrentMinFilter,
								CurrentMagFilter;
GLVAR GLfloat					CurrentAnisotropy;
GLVAR GLuint					LastTexture,
								LastMinFilter,
								LastMagFilter;
GLVAR GLVertex					pGLVertices[3072];
GLVAR GLVertex*					lpVertex;
GLVAR GLTriangle				pGLTriangles[2048];
GLVAR GLTriangle*				lpTriangle;
GLVAR GLuint					pGLVerticesCount;

GLVAR GLint					glMajor,glMinor,glRevision;
GLVAR GLVertex					gvtx[16],*pVtxBuf;
GLVAR GLuint					pTextures[2048];
GLVAR GLenum					pTextureMin[2048];
GLVAR GLuint					glTextureCount ;

GLVAR GLshader					pShaders[64];
GLVAR unsigned int				glShaderCount;
GLVAR bool						bBufNeedOp;
GLVAR bool						bFullscreen;
GLVAR GLuint					pFont;

GLVAR bool						AnisoTexture, BumpMaps, g_VertBufferBMP ;
GLVAR float						fMaxAniso;

// -- States
GLVAR GLint						gStateGLBlendDst,
								gStateGLBlendSrc,
								gStateGLTextureBind;
GLVAR GLuint					gStateGLFogColor;


#endif
