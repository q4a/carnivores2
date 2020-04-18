
#ifndef H_HUNT_H
#define H_HUNT_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#if defined( AF_PLATFORM_LINUX )
#	include <GL/glew.h>
#	include <GL/glxew.h>
#	include <GLFW/glfw3.h>
/* Dummy Defines */
#	define HDC int
#	define HANDLE void*
#elif defined( AF_PLATFORM_WINDOWS )
#	include <GL/glew.h>
#	include <GL/wglew.h>
#	include <GL/glfw.h>
#endif


#include "Platform.h"
#include "Globals.h"
#include "ImageTypes.h"
#include "resource.h"
#include "version.h"

#include "AudioAL.h"


#if defined( AF_DEBUG )
#	include <assert.h>
#else
#	define assert( expr )
#endif


/* Engine Specific (AF_ prefix) */
#define V_MAJOR					3
#define V_MINOR					1
#define V_PATCH					9
#define V_VERSION				"3.1.9"
#if defined( AF_PLATFORM_LINUX )
#	define V_PLATFORM				"LINUX"
#else
#	define V_PLATFORM				"WIN32"
#endif

/* Important constants */
#define ctHScale  64
#define ctMapSize 1024
#define PMORPHTIME 256
#define HiColor(R,G,B) ( ((R)<<10) + ((G)<<5) + (B) )

/* Texture min/max coord constants */
#define TCMAX ( 0.995f )
#define TCMIN ( 0.005f )

/* Not defined in GNU GCC, need to do them ourselves */
//#undef max
//#undef min
//#define max( a, b ) ( ( a > b) ? a : b )
//#define min( a, b ) ( ( a < b) ? a : b )
#define af_max( a, b ) ( ( a > b) ? a : b )
#define af_min( a, b ) ( ( a < b) ? a : b )


/* Ugly, horrible, nasty way to declare globals... */
#ifdef _MAIN_
#	define _EXTORNOT
#else
#	define _EXTORNOT extern
#endif


/* PI Pi Pie */
#define pi 		3.1415926535f
#define AF_PI 	3.1415926535f

/* Vert/Tri maximum constants */
#define MAX_TRIANGLES	2048
#define MAX_VERTICES	MAX_TRIANGLES*3


#if defined( AF_PLATFORM_LINUX )

char* strlwr( char* str );

#endif //AF_PLATFORM_LINUX


///////////////////////////////////////////////////////////////////////////////////////////////////
// ENUM
///////////////////////////////////////////////////////////////////////////////////////////////////

// -- for _GameState
enum gamestate_e {
	GAMESTATE_MAINMENU		=-1,
	GAMESTATE_GAMESTART		= 0,
	GAMESTATE_INGAME		= 1,
};

/* Particle type constants */
enum particletype_e {
	partNone	= 0,
	partBlood	= 1,
	partWater	= 2,
	partGround	= 3,
	partBubble	= 4,
	partSnow	= 5,
	partRain	= 6
};

enum rank_e {
	RANK_NOVICE			= 0,
	RANK_ADVANCED		= 1,
	RANK_EXPERT			= 2,
	RANK_MASTER			= 3
};

enum audiodriver_e
{
	AUDIO_SOFTWARE			= 0,
	AUDIO_DIRECTSOUND		= 1,
	AUDIO_OPENAL			= 2
};


/* Colour Types */
#pragma pack(push, 1)
typedef struct rgb_t {
     uint8_t        R;
     uint8_t        G;
     uint8_t        B;
} rgb_t;
#pragma pack(pop)

class rgba_t {
public:

	rgba_t() : R(0), G(0), B(0), A(255) {}
	rgba_t( uint8_t r, uint8_t g, uint8_t  b, uint8_t a) : R(r), G(g), B(b), A(a) {}

	uint32_t as32(void) { return (A<<24) | (B<<16) | (G<<8) | (R); }

	inline operator uint32_t () const
	{
		return (A<<24) | (B<<16) | (G<<8) | (R);
	}

	// -- 5551
	inline operator uint16_t () const
	{
		return ( (A & 1)<<15 ) | ( (B & 31)<<10 ) | ( (G & 31)<<5 ) | ( (R & 31) );
	}

	uint8_t		R;
	uint8_t		G;
	uint8_t		B;
	uint8_t		A;
};

typedef struct bgr_t {
     uint8_t        B;
     uint8_t        G;
     uint8_t        R;
} bgr_t;

typedef struct bgra_t {
     uint8_t        B;
     uint8_t        G;
     uint8_t        R;
     uint8_t		A;
} bgra_t;

typedef struct tagMessageList {
   int32_t			mTimeLeft;
   int32_t			mTimeStart;
   rgba_t		    m_color;
   char				mtext[512];
} TMessageList;

typedef struct _Animation {
    char            aniName[32];
    int32_t         aniKPS, FramesCount, AniTime;
    int16_t*       aniData;
} TAni;

typedef struct _VTLdata {
	int32_t			aniKPS,
					FramesCount,
					AniTime;
	int16_t*		aniData;
} TVTL;

class TSFX {
public:
	int32_t			length;
	int16_t*		lpData;

	TSFX() : length(0), lpData(0) {}
	~TSFX()
	{
		if ( lpData ) free( lpData );
	}

	// -- New struct
	/*
	uint32_t		m_bytelength;
	uint8_t			m_bitdepth;
	uint8_t*		m_data;
	*/
};



typedef struct _TRD {
	uint32_t		RNumber,
					RVolume,
					RFreq;
	uint16_t		REnvir,
					Flags;
} TRD;

typedef struct _TAmbient {
	TSFX		sfx;
	TRD			rdata[16];
	uint32_t	RSFXCount;
	uint32_t	AVolume;
	uint32_t	RndTime;
} TAmbient;


class CTexture
{
public:
	void*		m_data;

    uint16_t		m_width,m_height;
    uint8_t			m_bpp;
	uint32_t		m_texid;

    rgb_t			m_rgb;

    void Allocate();
    void Release();

    CTexture() :
		m_data(0),
		m_width(0), m_height(0),
		m_bpp(0),
		m_texid(0)
    {}
    ~CTexture()
    {
        this->Release();
    }

};


/*
* class TPicture
* Updates:
    -Modified to reflect the removal of HEAP
    -Modified to better support hardware renderer modules
*/
class TPicture
{
public:
    uint16_t	m_width,
				m_height;
    void*		m_data;
	uint32_t	m_texid;
	uint16_t	m_cwidth,
				m_cheight;  // True canvas dimensions (used in Textures)
	uint16_t	m_bpp;

	void Allocate();
	void Release();

	TPicture() :
		m_width(0),
		m_height(0),
		m_data(0),
		m_texid(0),
		m_cwidth(0),
		m_cheight(0),
		m_bpp(0)
	{}
	~TPicture()
	{
	    Release();
	}
};


class vec3
{
public:
	float x,y,z;

	vec3() : x(0.0f), y(0.0f), z(0.0f) {}
	vec3( float a, float b, float c ) : x(a), y(b), z(c) {}
};


typedef struct vec2i
{
	int32_t x,y;
} vec2i;

typedef struct vec2d
{
	double x,y;
} vec2d;

class Vector2df
{
public:

	Vector2df(){}
	Vector2df(float a, float b){ x = a; y = b; }

	float x,y;
};


typedef struct TagScrPoint
{
	float 		x,y, tx,ty;
	uint32_t 	Light, z, r2, r3;
} ScrPoint;

typedef struct TagMScrPoint {
	uint32_t	x,y, tx,ty;
} MScrPoint;

typedef struct tagClipPlane
{
	vec3		v1,v2,nv;
} CLIPPLANE;





typedef struct TagEPoint
{
	vec3		v;
	uint16_t	DFlags;
	int16_t		ALPHA;
	float		scrx, scry;	// -- Scren X/Y co-ordinates
	uint32_t	Light; // -- Light value
	float		Fog; // -- Fog strength?
} EPoint;


typedef struct TagClipPoint {
  EPoint ev;
  float tx, ty;
} ClipPoint;


//================= MODEL ========================
typedef struct _Point3d {
	float x;
	float y;
	float z;
	short owner;
	short hide;
} TPoint3d;



typedef struct _Face {
   int v1, v2, v3;
   float   tax, tbx, tcx, tay, tby, tcy;
   uint16_t Flags,DMask;
   int Distant, Next, group;
   char reserv[12];
} TFace;


typedef struct _Facef {
   int v1, v2, v3;
   float tax, tbx, tcx, tay, tby, tcy;
   uint16_t Flags,DMask;
   int Distant, Next, group;
   char reserv[12];
} TFacef;



typedef struct _Obj {
   char OName [32];
   float ox;
   float oy;
   float oz;
   short owner;
   short hide;
} TObj;


class TModel
{
public:
    uint32_t	VCount,						// Vertex Count
				FCount,						// Face/Triangle Count
				TextureSize,				// Deprecated
				TextureHeight;				// Deprecated
    TPoint3d 	gVertex[ MAX_VERTICES ];	// Vertices
	TFacef   	gFace  [ MAX_TRIANGLES ];	// Faces

    CTexture	m_texture;					// Texture

	uint32_t	VLight[4][ MAX_VERTICES ];	// Vertex Light

	TModel() :
		VCount(0),
		FCount(0),
		TextureSize(0),
		TextureHeight(0)
	{}
	//~TModel(){}
};


//=========== END MODEL ==============================//


typedef struct _ObjInfo {
   int  Radius;
   int  YLo, YHi;
   int  linelenght, lintensity;
   int  circlerad, cintensity;
   int  flags;
   int  GrRad;
   int  DefLight;
   int  LastAniTime;
   float BoundR;
   uint8_t res[16];
} TObjInfo;

typedef struct _TBMPModel {
    vec3  		gVertex[4];
	CTexture	m_texture;
} TBMPModel;

typedef struct _TBound {
	float cx, cy, a, b,  y1, y2;
} TBound;

typedef struct TagObject {
   TObjInfo info;
   TBound   bound[8];
   TBMPModel bmpmodel;
   TModel  *model;
   TVTL    vtl;
} TObject;


typedef struct _TCharacterInfo {
  char ModelName[32];
  int AniCount,SfxCount;
  TModel* mptr;
  TAni Animation[64];
  sound_t SoundFX[64];
  int  Anifx[64];
} TCharacterInfo;

typedef struct _TWeapon {
  TCharacterInfo chinfo[10];
  TPicture       BulletPic[10];
  vec3       normals[ MAX_VERTICES ];
  int state, FTime;
  float shakel;
} TWeapon;



typedef struct _TWCircle {
    vec3 pos;
	float scale;
	int FTime;
} TWCircle;


typedef struct _TSnowElement {
    vec3 pos;
	float hl, ftime;
} TSnowElement;




typedef struct _TCharacter  {
  int CType, AI;
  TCharacterInfo *pinfo;
  int StateF;
  int State;
  int NoWayCnt, NoFindCnt, AfraidTime, tgtime;
  int PPMorphTime, PrevPhase,PrevPFTime, Phase, FTime;

  float vspeed, rspeed, bend, scale;
  int Slide;
  float slidex, slidez;
  float tgx, tgz;

  vec3 pos, rpos;
  float tgalpha, alpha, beta,
        tggamma,gamma,
        lookx, lookz;
  int Health, BloodTime, BloodTTime;
} TCharacter;



typedef struct tagPlayer {
  bool			Active;
  unsigned int	IPaddr;
  vec3		pos;
  float			alpha, beta, vspeed;
  int			kbState;
  char			NickName[16];
} TPlayer;


typedef struct _TDemoPoint {
  vec3 pos;
  int DemoTime, CIndex;
} TDemoPoint;

typedef struct tagLevelDef {
    char FileName[64];
    char MapName[128];
    uint32_t DinosAvail;
    uint16_t *lpMapImage;
} TLevelDef;


typedef struct tagShipTask {
  int tcount;
  int clist[255];
} TShipTask;

typedef struct tagShip {
  vec3 pos, rpos, tgpos, retpos;
  float alpha, tgalpha, speed, rspeed, DeltaY;
  int State, cindex, FTime;
} TShip;


typedef struct tagAmmoBag {
  vec3 pos, rpos, speed;
  float alpha;
  int State, FTime;
} TAmmoBag;


typedef struct tagSupplyShip {
  vec3 pos, rpos, tgpos, retpos;
  float alpha, tgalpha, speed, rspeed;
  int State;
} TSupplyShip;



typedef struct tagLandingList {
  int PCount;
  vec2i list[64];
} TLandingList;


typedef struct _TPlayerR {
	char	PName[128];
	int		RegNumber;
	int		Score, Rank;
	float	x,y,z;
	float	alpha,beta;
	int		State;
} TPlayerR;

typedef struct _TTrophyItem {
  int ctype, weapon, phase,
	  height, weight, score,
	  date, time;
  float scale, range;
  int r1, r2, r3, r4;
} TTrophyItem;


typedef struct _TStats {
	int smade, success;
	float path, time;
} TStats;


typedef struct _TTrophyRoom {
  char PlayerName[128];
  int  RegNumber;
  int  Score, Rank;

  TStats Last, Total;

  TTrophyItem Body[24];
} TTrophyRoom;


// -- [RH99] Cleaned up and commented
typedef struct _TDinoInfo {
	char		Name[48],		// Animal name
				FName[48],		// Data file
				PName[48];		// Call picture file
	int			Health0,		// Max health
				AI;				// AI index
	bool		DangerCall;		// Scare herbivores with call?
	float		Mass,			// Weight
				Length,			// Length
				Radius,			// Special physics radius (used in detection of collisions and bullets)
				SmellK,			// Sensory Smell
				HearK,			// Sensory Hearing
				LookK,			// Sensory Sight
				ShDelta;		// Unique drop ship height
	int			Scale0,			// Scale base
				ScaleA,			// Scale additional
				BaseScore,		// Base score
				Group,
				GroupMinSize,
				GroupMaxSize;
	TPicture	CallIcon;		// Call icon
} TDinoInfo;


// -- [RH99] Weapon information - Cleaned and commented
class TWeapInfo
{
public:

	char	Name[48];			// Name of weapon
	char	FName[48];			// File of weapon
	char	BFName[48];			// Bullet icon file
	float	Power;				// Power of tracer
	float	Prec;				// Precision of tracer
	float	Loud;				// Audio level of tracer fire
	float	Rate;				// Rate of fire (for menu)

	float	HipX,				// Hip XYZ for moving weapon on screen
			HipY,
			HipZ;

	int		Shots,				// Number of bullets in magazine
			Optic,				// Is it optical like sniper rifle?
			Fall,				// Does it fall like an arrow? (does not work)
			TraceC,				// Number of tracers per shot
			Reload;				// How many times can it fire before needing to reload?
	float	Zoom,				// Zoom level of weapon when aiming (1.0==None)
			Range;				// Range of tracer(s) (Does not work)

	bool	CrossHair;			// Does this weapon have a crosshair?
	int		CrossHairType;		// If so, what crosshair does it have?  (-1 == image?)
	uint32_t	CrossHairColor;		// What is the 32-bit colour of the crosshair?

	// -- Constructor to initialize certain values
	TWeapInfo()
	{
		Range = 1.0f;
		Zoom = 1.0f;
		CrossHair = 0;
		CrossHairType = 0;
		CrossHairColor = 0x8000AF10;
	}
};


typedef struct _TFogEntity {
  int fogRGB;
  float YBegin;
  bool  Mortal;
  float Transp, FLimit;
} TFogEntity;


typedef struct _TWaterEntity {
  int tindex, wlevel;
  float transp;
  int fogRGB;
} TWaterEntity;


typedef struct TWind {
   float alpha;
   float speed;
   vec3 nv;
} TWind;




typedef struct _TElement {
   vec3 pos, speed;
   int     Flags;
   float   R;
} TElement;

typedef struct _TElements {
	int Type, ECount, EDone, LifeTime;
	int Param1, Param2, Param3;
	uint32_t RGBA, RGBA2;
    vec3 pos;
    TElement EList[32];
} TElements;


typedef struct _TBloodP {
	int LTime;
	vec3 pos;
} TBloodP;

typedef struct _TBTrail {
	int Count;
    TBloodP Trail[512];
} TBTrail;


//============= functions ==========================//

void HLineTxB( void );
void HLineTxC( void );
void HLineTxGOURAUD( void );


void HLineTxModel25( void );
void HLineTxModel75( void );
void HLineTxModel50( void );

void HLineTxModel3( void );
void HLineTxModel2( void );
void HLineTxModel( void );

void HLineTDGlass75( void );
void HLineTDGlass50( void );
void HLineTDGlass25( void );
void HLineTBGlass25( void );


void SetVideoMode(int, int);

void CreateDivTable();
void DrawTexturedFace();
int GetTextW(HDC, const char*);
int GetTextH(HDC, const char*);
void wait_mouse_release();

//============================== render =================================//
void ShowControlElements();
void InsertModelList(TModel* mptr, float x0, float y0, float z0, int light, float al, float bt);
void RenderGround();
void RenderWater();
void RenderElements();
void CreateChRenderList();
void RenderModelsList();
void Render_Text(int, int, char*, unsigned long);
void ProcessMap  (int x, int y, int r);
void ProcessMap2 (int x, int y, int r);
void ProcessMapW (int x, int y, int r);
void ProcessMapW2(int x, int y, int r);

void DrawTPlane(bool);
void DrawTPlaneClip(bool);
void ClearVideoBuf();
void DrawTrophyText(int, int);
void DrawHMap();
void RenderCharacter(int);
void RenderShip();
void RenderPlayer(int);
void RenderSkyPlane();
void RenderHealthBar();
void Render_Cross(int, int);
void Render_LifeInfo(int);

void RenderModelClipEnvMap(TModel*, float, float, float, float, float);
void RenderModelClipPhongMap(TModel*, float, float, float, float, float);

void RenderModel         (TModel*, float, float, float, int, int, float, float);
void RenderBMPModel      (TBMPModel*, float, float, float, int);
void RenderModelClipWater(TModel*, float, float, float, int, int, float, float);
void RenderModelClip     (TModel*, float, float, float, int, int, float, float);
void RenderNearModel     (TModel*, float, float, float, int, float, float);
void DrawPicture         (int x, int y, TPicture &pic);

void InitClips();
void InitClips2();
void InitDirectDraw();
void WaitRetrace();

//============= Characters =======================
void Characters_AddSecondaryOne(int ctype);
void AddDeadBody(TCharacter *cptr, int);
void PlaceCharacters();
void PlaceTrophy();
void AnimateCharacters();
void MakeNoise(vec3, float);
void CheckAfraid();
void CreateChMorphedModel(TCharacter* cptr);
void CreateMorphedObject(TModel* mptr, TVTL &vtl, int FTime);
void CreateMorphedModel(TModel* mptr, TAni *aptr, int FTime, float scale);

//=============================== Math ==================================//

void        CalcLights  (TModel* mptr);
void        CalcModelGroundLight(TModel *mptr, float x0, float z0, int FI);
void        CalcNormals (TModel* mptr, vec3 *nvs);
void        CalcGouraud (TModel* mptr, vec3 *nvs);

void        CalcPhongMapping(TModel* mptr, vec3 *nv);
void        CalcEnvMapping(TModel* mptr, vec3 *nv);

void        CalcBoundBox(TModel* mptr, TBound *bound);
void        NormVector(vec3&, float);
float       SGN(float);
void        DeltaFunc(float &a, float b, float d);
void        MulVectorsScal(vec3, vec3, float&);
void        MulVectorsVect(vec3, vec3, vec3&);
vec3    SubVectors( vec3, vec3);
vec3    AddVectors( vec3, vec3);
vec3    RotateVector(vec3);
float       VectorLength(vec3);
int         siRand(int);
int         rRand(int);
void        CalcHitPoint(CLIPPLANE&, vec3, vec3, vec3);
void        ClipVector(CLIPPLANE& C, int vn);
float       FindVectorAlpha(float, float);
float       AngleDifference(float a, float b);

int         TraceShot(  float ax, float ay, float az,
                        float &bx, float &by, float &bz );
int         TraceLook(  float ax, float ay, float az,
                        float bx, float by, float bz );


void    CheckCollision(float&, float&);
float   CalcFogLevel(vec3 v);
//=================================================================//
void    AddMessage(const char* mt);
void	AddMessage(const char* mt, float Seconds);
void    CreateTMap();


void    LoadSky();
void    LoadSkyMap();
void    LoadTexture( CTexture*& );
void    LoadWav(char* FName, TSFX &sfx);

//Renderer
template <typename T> void CreateMipMap( T* dstBuf, T* srcBuf, int W, int H );
int 	RenCreateTexture(bool alpha, bool mipmaps, int w, int h, int bpp, void *data);
void	SetWireframe( bool );


void    ApplyAlphaFlags( void*, int);
uint16_t    conv_565(uint16_t c);
int     conv_xGx(int);
void    conv_pic(TPicture &pic);
void    LoadPicture(TPicture &pic, const char* pname);
void    LoadPictureBMP(TPicture &pic, const char* pname);
void    LoadPictureTGA(TPicture &pic, const char* pname);
void    LoadCharacterInfo(TCharacterInfo&, const char*);
void    LoadModelEx(TModel* &mptr, char* FName);
void    LoadModel(TModel*&);
void    LoadResources();
void    ReInitGame();


void    SaveScreenShot();
void    CreateWaterTab();
void    CreateFadeTab();
void    CreateVideoDIB();
void    RenderLightMap();


void*  _HeapAlloc(HANDLE hHeap, uint32_t dwFlags, uint32_t dwuint8_ts);
bool    _HeapFree(HANDLE hHeap, uint32_t  dwFlags, void* lpMem);

//============ game ===========================//
void	ProcessCommandLine();
float   GetLandCeilH(float, float);
float   GetLandH(float, float);
float   GetLandOH(int, int);
float   GetLandLt(float, float);
float   GetLandUpH(float, float);
float   GetLandQH(float, float);
float   GetLandQHNoObj(float, float);
float   GetLandHObj(float, float);

void	ResetMousePos();
void    LoadResourcesScript();
void    InitEngine();
void    ShutDownEngine();
void    ProcessSyncro();
void    AddShipTask(int);
void    LoadTrophy();
void	LoadPlayersInfo(); //<-Multiplayer?
void    SaveTrophy();
void    RemoveCurrentTrophy();
void    MakeCall();
void    MakeShot(float ax, float ay, float az,
                 float bx, float by, float bz);

void    AddBloodTrail(TCharacter *cptr);
void    AddElements(float, float, float, int, int);
void    AddWCircle(float, float, float);
void    AnimateProcesses();
void    DoHalt(const char*);

_EXTORNOT   char logt[128];
void    CreateLog();
void    PrintLog(const char* l);
void    CloseLog();

#if defined( AF_PLATFORM_WINDOWS )
_EXTORNOT	HFONT fnt_BIG, fnt_Small, fnt_Midd;
_EXTORNOT	HDC		hdcMain;
#endif

_EXTORNOT   float BackViewR;
_EXTORNOT   int   BackViewRR;
_EXTORNOT   int   UnderWaterT;
_EXTORNOT   int   TotalC, TotalW;


//========== common ==================//

_EXTORNOT   bool    blActive;
_EXTORNOT   uint8_t KeyboardState[256];
_EXTORNOT   int     KeyFlags, _shotcounter;

_EXTORNOT   TMessageList MessageList[32];
_EXTORNOT   char    ProjectName[128];
_EXTORNOT   int     _GameState;
_EXTORNOT   sound_t		fxCall[10][3],
						fxUnderwater, fxWaterIn, fxWaterOut, fxJump, fxStep[3], fxStepW[3],
						fxScream[4], mscMenu;

//========== map =====================//
_EXTORNOT   uint8_t HMap[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t WMap[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t HMapO[ctMapSize][ctMapSize];
_EXTORNOT   uint16_t FMap[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t LMap[ctMapSize][ctMapSize];
_EXTORNOT   uint16_t TMap1[ctMapSize][ctMapSize];
_EXTORNOT   uint16_t TMap2[ctMapSize][ctMapSize];
_EXTORNOT   uint8_t OMap[ctMapSize][ctMapSize];

_EXTORNOT   uint8_t FogsMap[512][512];
_EXTORNOT   uint8_t AmbMap[512][512];

_EXTORNOT   TFogEntity    FogsList[256];
_EXTORNOT   TWaterEntity  WaterList[256];
_EXTORNOT   TWind       Wind;
_EXTORNOT   TShip       Ship;
_EXTORNOT   TAmmoBag    AmmoBag;
_EXTORNOT   TSupplyShip SupplyShip;
_EXTORNOT   TShipTask   ShipTask;

_EXTORNOT   int SkyR, SkyG, SkyB, WaterR, WaterG, WaterB, WaterA,
                SkyTR,SkyTG,SkyTB, CurFogColor;
_EXTORNOT   int RandomMap[32][32];

_EXTORNOT   Vector2df PhongMapping[ MAX_VERTICES ];
_EXTORNOT   TPicture TFX_SPECULAR, TFX_ENVMAP;
_EXTORNOT	CTexture SkyTexture;
_EXTORNOT   uint16_t SkyFade[9][128*128];
_EXTORNOT   uint8_t SkyMap[128*128];

_EXTORNOT   CTexture* Textures[1024];
_EXTORNOT   TAmbient Ambient[256];
_EXTORNOT   TSFX     RandSound[256];

_EXTORNOT   int	TextureCount,ModelCount;

//========= GAME ====================//
_EXTORNOT double StepStart;
_EXTORNOT uint32_t FramesPerSecond,Frames;

_EXTORNOT int TargetDino, TargetArea, TargetWeapon, WeaponPres, TargetCall,
              TrophyTime, ObservMode, Tranq, ObjectsOnLook,
			  CurrentWeapon, ShotsLeft[10], AmmoMag[10];

_EXTORNOT vec3 answpos;
_EXTORNOT int answtime, answcall;

_EXTORNOT bool ScentMode, CamoMode, NightVision,
               RadarMode, LockLanding,
			   SurviveMode,
			   Windowed, CustomRes,
			   TrophyMode, DoubleAmmo, Supply, SupplyUsed;

_EXTORNOT TTrophyRoom TrophyRoom;
_EXTORNOT TPlayerR PlayerR[16];
_EXTORNOT TPicture LandPic,DinoPic,DinoPicM, MapPic, RadarPic, WepPic;
_EXTORNOT TLandingList LandingList;

//======== MODEL ======================//
_EXTORNOT TObject  MObjects[256];
_EXTORNOT TModel* mptr;
_EXTORNOT TWeapon Weapon;


_EXTORNOT int   OCount, iModelFade, iModelBaseFade, Current;
_EXTORNOT vec3  rVertex[ MAX_VERTICES ];
_EXTORNOT TObj      gObj[ 1024 ];
_EXTORNOT vec2i gScrp[ MAX_VERTICES ];
_EXTORNOT Vector2df gScrpf[ MAX_VERTICES ];

//============= Characters ==============//
_EXTORNOT TPicture  PausePic, ExitPic, TrophyExit, TrophyPic;
_EXTORNOT TModel *SunModel;
_EXTORNOT TCharacterInfo WCircleModel;
_EXTORNOT TModel *CompasModel;
_EXTORNOT TModel *Binocular;
_EXTORNOT TDinoInfo DinoInfo[128];
_EXTORNOT TWeapInfo WeapInfo[10];
_EXTORNOT TCharacterInfo ShipModel, SShipModel, AmmoModel;
_EXTORNOT int AI_to_CIndex[128];
_EXTORNOT int ChCount, WCCount, ElCount, SnCount, RainCount,
              ShotDino, TrophyBody;
_EXTORNOT TCharacterInfo WindModel;
_EXTORNOT TCharacterInfo PlayerInfo;
_EXTORNOT TCharacterInfo ChInfo[128];
_EXTORNOT TCharacter     Characters[256];
_EXTORNOT TWCircle       WCircles[128];
_EXTORNOT TSnowElement   Snow[8024],Rain[8024];
_EXTORNOT TDemoPoint     DemoPoint;

_EXTORNOT TPlayer        Players[16];
_EXTORNOT vec3       PlayerPos, CameraPos;

//========== Render ==================//
_EXTORNOT   GLFWwindow* GWindow;
_EXTORNOT   void* lpVideoRAM;
_EXTORNOT   bool DirectActive, RestartMode;
_EXTORNOT   bool LoDetailSky;
_EXTORNOT   int  WinW,WinH,WinEX,WinEY,WindowCX,WindowCY,VideoCX,VideoCY,VideoCX16,VideoCY16,iuint8_tsPerLine,ts,r,MapMinY;
_EXTORNOT   float Aspect;
_EXTORNOT   float VideoCXf, VideoCYf, CameraW,CameraH,CameraW16,CameraH16,Soft_Persp_K, stepdy, stepdd, SunShadowK, FOVK;
_EXTORNOT   CLIPPLANE ClipA,ClipB,ClipC,ClipD,ClipZ,ClipW;
_EXTORNOT   int u,vused, CCX, CCY;

_EXTORNOT   uint32_t Mask1,Mask2;
_EXTORNOT   uint32_t HeapAllocated, HeapReleased;


_EXTORNOT   EPoint VMap[256][256];
_EXTORNOT   EPoint VMap2[256][256];
_EXTORNOT   EPoint ev[3];

_EXTORNOT   ClipPoint cp[16];
_EXTORNOT   ClipPoint hleft,hright;


_EXTORNOT   void  *HLineT;
_EXTORNOT   int   rTColor;
_EXTORNOT   int   SKYMin, SKYDTime, GlassL, ctViewR, ctViewR1, ctViewRM,
                  dFacesCount, ReverseOn, TDirection;
_EXTORNOT   uint16_t  FadeTab[65][0x8000];
_EXTORNOT   TElements Elements[32];
_EXTORNOT   TBTrail   BloodTrail;

_EXTORNOT   int     PrevTime, TimeDt, T, Takt, RealTime, StepTime, MyHealth, MyOxygen, ExitTime,
                    ChCallTime, CallLockTime, NextCall;
_EXTORNOT   float   DeltaT;
_EXTORNOT   float   CameraX, CameraY, CameraZ, CameraAlpha, CameraBeta;
_EXTORNOT   float   PlayerX, PlayerY, PlayerZ, PlayerAlpha, PlayerBeta,
                    HeadY, HeadBackR, HeadBSpeed, HeadAlpha, HeadBeta,
                    PlayerSpeed, SSpeed,VSpeed,RSpeed,YSpeed;
_EXTORNOT   vec3 PlayerNv;

_EXTORNOT   float   ca,sa,cb,sb, wpnDAlpha, wpnDBeta;
_EXTORNOT   void    *lpVideoBuf, *lpScreenBuf, *lpTextureAddr;
_EXTORNOT   int     DivTbl[10240];

_EXTORNOT   vec3  v[3];
_EXTORNOT   ScrPoint  scrp[3];
_EXTORNOT   MScrPoint mscrp[3];
_EXTORNOT   vec3  nv, waterclipbase, Sun3dPos;

_EXTORNOT	FILE* hlog, *haudiolog;

extern Globals* g;

_EXTORNOT	AudioAL*			g_AudioDevice;


_EXTORNOT   struct _t {
		int fkForward, fkBackward, fkUp, fkDown, fkLeft, fkRight, fkFire, fkShow, fkSLeft, fkSRight, fkStrafe, fkJump, fkRun, fkCrouch, fkCall, fkCCall, fkBinoc, fkSupply;
} KeyMap;


#define kfForward     0x00000001
#define kfBackward    0x00000002
#define kfLeft        0x00000004
#define kfRight       0x00000008
#define kfLookUp      0x00000010
#define kfLookDn      0x00000020
#define kfJump        0x00000040
#define kfCall        0x00000100

#define kfSLeft       0x00001000
#define kfSRight      0x00002000
#define kfStrafe      0x00004000

#define fmWater   0x0080
#define fmWater2  0x8000
#define fmNOWAY   0x0020
#define fmReverse 0x0010

#define fmWaterA  0x8080


#define tresGround 1
#define tresWater  2
#define tresModel  3
#define tresChar   4

enum {
	sfDoubleSide	= 0x0001,
	sfDarkBack		= 0x0002,
	sfOpacity		= 0x0004,
	sfTransparent	= 0x0008,
	sfMortal		= 0x0010,
	sfPhong			= 0x0020,
	sfEnvMap		= 0x0040,
	sfNeedVC		= 0x0080,
	sfDark			= 0x8000,
};

enum
{
	ofPLACEWATER	= 0x00000001,
	ofPLACEGROUND	= 0x00000002,
	ofPLACEUSER		= 0x00000004,
	ofCIRCLE		= 0x00000008,
	ofBOUND			= 0x00000010,
	ofNOBMP			= 0x00000020,
	ofNOLIGHT		= 0x00000040,
	ofDEFLIGHT		= 0x00000080,
	ofGRNDLIGHT		= 0x00000100,
	ofNOSOFT		= 0x00000200,
	ofNOSOFT2		= 0x00000400,
	ofANIMATED		= 0x80000000,
};

#define csONWATER          0x00010000
#define MAX_HEALTH         100000//128000

#define HUNT_EAT      0
#define HUNT_BREATH   1
#define HUNT_FALL     2
#define HUNT_KILL     3

#include "AI.h"


_EXTORNOT bool bDevShow;
_EXTORNOT char cDevConsole[256];
_EXTORNOT int  CameraFogI, QUITMODE;
_EXTORNOT int OptDayNight, OptAgres, OptDens, OptSens, OptRes, OptViewR,
              OptMsSens, OptBrightness, OptSound, OptRender,
              OptText, OptSys, WaitKey, OPT_ALPHA_COLORKEY;


void GenerateMipMap(uint16_t* A, uint16_t* D, int L);


//========== for network ==============//
void InitializeNetwork();
void ShutdownNetwork();
void CreateSocket( char*, char*, uint32_t );

//========== for audio ==============//
void  PlayWAVSample( int16_t* buffer, size_t length );
int16_t* AudioResample( int16_t* buffer, size_t length );
void  AddVoice( int length, int16_t* lpdata );
void  AddVoicev  (int, int16_t*, int);
void  AddVoice3dv(int, int16_t*, float, float, float, int);
void  AddVoice3d (int, int16_t*, float, float, float);

void SetAmbient3d(int, int16_t*, float, float, float);
void SetAmbient(int, int16_t*, int);
void AudioSetCameraPos(float, float, float, float, float);
void InitAudioSystem( int32_t );
void Audio_Restore();
void AudioStop();
void Audio_Shutdown();
void Audio_SetEnvironment(int, float);
//========== for 3d hardware =============//
_EXTORNOT bool HARD3D;
void ShowVideo();
void Init3DHardware();
void Activate3DHardware();
void ShutDown3DHardware();
void Render3DHardwarePosts();
void CopyBackToDIB();
bool CopyHARDToDIB();
void Hardware_ZBuffer(bool zb);
void Draw_Text(int x,int y,char* text, unsigned long color);


//=========== loading =============
void StartLoading();
void EndLoading();
void PrintLoad(const char *t);

#ifdef _MAIN_
_EXTORNOT char KeysName[256][24] = {
"...",
"Esc",
"1",
"2",
"3",
"4",
"5",
"6",
"7",
"8",
"9",
"0",
"-",
"=",
"BSpace",
"Tab",
"Q",
"W",
"E",
"R",
"T",
"Y",
"U",
"I",
"O",
"P",
"[",
"]",
"Enter",
"Ctrl",
"A",
"S",
"D",
"F",
"G",
"H",
"J",
"K",
"L",
";",
"'",
"~",
"Shift",
"\\",
"Z",
"X",
"C",
"V",
"B",
"N",
"M",
",",
".",
"/",
"Shift",
"*",
"Alt",
"Space",
"CLock",
"F1",
"F2",
"F3",
"F4",
"F5",
"F6",
"F7",
"F8",
"F9",
"F10",
"NLock",
"SLock",
"Home",
"Up",
"PgUp",
"-",
"Left",
"Midle",
"Right",
"+",
"End",
"Down",
"PgDn",
"Ins",
"Del",
"",
"",
"",
"F11",
"F12",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"Mouse1",
"Mouse2",
"Mouse3",
"<?>",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
	};
#else
   _EXTORNOT char KeysName[128][24];
#endif

#endif //H_HUNT_H
