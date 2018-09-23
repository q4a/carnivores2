#include "Hunt.h"
#include "ImageTypes.h"
#include <stdio.h>
#include <string.h>
#include "gl_extra.h"

FILE* hfile;

// == Forward Declaration == //
void GenerateModelMipMaps(TModel *mptr);
void GenerateAlphaFlags(TModel *mptr);


void AddMessage( const char* mt, float Seconds)
{
    // -- [RH99] Old Code:
    /*
    MessageList[0].timeleft = timeGetTime() + 5 * 1000;
    lstrcpy(MessageList[0].mtext, mt);
    */

    // -- [RH99] New Code:
    if ( mt == 0 ) return;
    bool printed = false;

    memcpy( &MessageList[1], &MessageList[0], sizeof(TMessageList)*31 );
	MessageList[0].m_color = rgba_t( 255,255,255, 255 ); // Change this based on command
	strcpy( MessageList[0].mtext, mt );
}

void AddMessage( const char* mt)
{
    AddMessage( mt, 5.0f );
}

void PlaceHunter()
{
    if (LockLanding) return;

    if (TrophyMode)
    {
        PlayerX = 76*256+128;
        PlayerZ = 70*256+128;
        PlayerY = GetLandQH(PlayerX, PlayerZ);
        return;
    }

    int p = ((uint64_t)(glfwGetTime()*1000.0) % LandingList.PCount);
    PlayerX = (float)LandingList.list[p].x * 256+128;
    PlayerZ = (float)LandingList.list[p].y * 256+128;
    PlayerY = GetLandQH(PlayerX, PlayerZ);
}


int DitherHi(int C)
{
    int d = C & 255;
    C = C / 256;
    if (rand() * 255 / RAND_MAX < d) C++;
    if (C>31) C=31;
    return C;
}




void CreateWaterTab()
{
    for (int c=0; c<0x8000; c++)
    {
        int R = (c >> 10);
        int G = (c >>  5) & 31;
        int B = c & 31;
        R =  1+(R * 8 ) / 28;
        if (R>31) R=31;
        G =  2+(G * 18) / 28;
        if (G>31) G=31;
        B =  3+(B * 22) / 28;
        if (B>31) B=31;
        FadeTab[64][c] = HiColor(R, G, B);
    }
}

void CreateFadeTab()
{
#ifdef _soft
    for (int l=0; l<64; l++)
        for (int c=0; c<0x8000; c++)
        {
            int R = (c >> 10);
            int G = (c >>  5) & 31;
            int B = c & 31;

            R = (int)((float)R * (l) / 60.f + (float)rand() *0.2f / RAND_MAX);
            if (R>31) R=31;
            G = (int)((float)G * (l) / 60.f + (float)rand() *0.2f / RAND_MAX);
            if (G>31) G=31;
            B = (int)((float)B * (l) / 60.f + (float)rand() *0.2f / RAND_MAX);
            if (B>31) B=31;
            FadeTab[l][c] = HiColor(R, G, B);
        }

    CreateWaterTab();
#endif
}


void CreateDivTable()
{
    DivTbl[0] = 0x7fffffff;
    DivTbl[1] = 0x7fffffff;
    DivTbl[2] = 0x7fffffff;
    for( int i = 3; i < 10240; i++ )
        DivTbl[i] = (int) ((float)0x10000000 / i);

    for (int y=0; y<32; y++)
        for (int x=0; x<32; x++)
            RandomMap[y][x] = rand() * 1024 / RAND_MAX;
}


void CreateVideoDIB()
{
    // create lpVideoBuf
    if ( lpVideoBuf )
    {
		lpVideoBuf = new uint16_t [ 1024*768 ];
    }
}






int GetObjectH(int x, int y, int R)
{
    x = (x<<8) + 128;
    y = (y<<8) + 128;
    float hr,h;
    hr =GetLandH((float)x,    (float)y);
    h = GetLandH( (float)x+R, (float)y);
    if (h < hr) hr = h;
    h = GetLandH( (float)x-R, (float)y);
    if (h < hr) hr = h;
    h = GetLandH( (float)x,   (float)y+R);
    if (h < hr) hr = h;
    h = GetLandH( (float)x,   (float)y-R);
    if (h < hr) hr = h;
    hr += 15;
    return  (int) (hr / ctHScale);
}


int GetObjectHWater(int x, int y)
{
    if (FMap[y][x] & fmReverse)
        return (int)(HMap[y][x+1]+HMap[y+1][x]) / 2 + 48;
    else
        return (int)(HMap[y][x]+HMap[y+1][x+1]) / 2 + 48;
}



void CreateTMap()
{
    int x,y;
    LandingList.PCount = 0;

    int ScMaps = 0;
    int SL = (100*(OptBrightness + 128))>>8;
    for (y=0; y<ctMapSize; y++)
        for (x=0; x<ctMapSize; x++)
        {
            if (TMap1[y][x]==0xFFFF) TMap1[y][x] = 1;
            if (TMap2[y][x]==0xFFFF) TMap2[y][x] = 1;

            if (Textures[TMap1[y][x]]->m_rgb.R > SL &&
                    Textures[TMap1[y][x]]->m_rgb.G > SL &&
                    Textures[TMap1[y][x]]->m_rgb.B > SL) ScMaps++;
        }

    g->SNOW = (ScMaps > 200000);


    /*
      for (y=1; y<ctMapSize-1; y++)
         for (x=1; x<ctMapSize-1; x++)
    		 if (!(FMap[y][x] & fmWater) ) {

    			 if (FMap[y  ][x+1] & fmWater) { FMap[y][x]|= fmWater2; WMap[y][x] = WMap[y  ][x+1];}
    			 if (FMap[y+1][x  ] & fmWater) { FMap[y][x]|= fmWater2; WMap[y][x] = WMap[y+1][x  ];}
    			 if (FMap[y  ][x-1] & fmWater) { FMap[y][x]|= fmWater2; WMap[y][x] = WMap[y  ][x-1];}
    			 if (FMap[y-1][x  ] & fmWater) { FMap[y][x]|= fmWater2; WMap[y][x] = WMap[y-1][x  ];}

    			 if (FMap[y][x] & fmWater2)
    			     if (HMap[y][x] > WaterList[WMap[y][x]].wlevel) HMap[y][x]=WaterList[WMap[y][x]].wlevel;
    		 }

      for (y=1; y<ctMapSize-1; y++)
         for (x=1; x<ctMapSize-1; x++)
    		 if (FMap[y][x] & fmWater2) {
    			 FMap[y][x]-=fmWater2;
    			 FMap[y][x]+=fmWater;
    		 }
    */


    for (y=1; y<ctMapSize-1; y++)
        for (x=1; x<ctMapSize-1; x++)
            if (!(FMap[y][x] & fmWater) )
            {
                if (FMap[y  ][x+1] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y  ][x+1];
                }
                if (FMap[y+1][x  ] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y+1][x  ];
                }
                if (FMap[y  ][x-1] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y  ][x-1];
                }
                if (FMap[y-1][x  ] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y-1][x  ];
                }

                bool l = true;

#ifdef _soft
                if (FMap[y][x] & fmWater2)
                {
                    l = false;
                    if (HMap[y][x] > WaterList[WMap[y][x]].wlevel) HMap[y][x]=WaterList[WMap[y][x]].wlevel;
                    HMap[y][x]=WaterList[WMap[y][x]].wlevel;
                }
#endif

                if (FMap[y-1][x-1] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y-1][x-1];
                }
                if (FMap[y-1][x+1] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y-1][x+1];
                }
                if (FMap[y+1][x-1] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y+1][x-1];
                }
                if (FMap[y+1][x+1] & fmWater)
                {
                    FMap[y][x]|= fmWater2;
                    WMap[y][x] = WMap[y+1][x+1];
                }

                if (l)
                    if (FMap[y][x] & fmWater2)
                        if (HMap[y][x] == WaterList[WMap[y][x]].wlevel) HMap[y][x]+=1;

                //if (FMap[y][x] & fmWater2)

            }

#ifdef _soft
    for (y=0; y<1024; y++)
        for (x=0; x<1024; x++ )
        {
            if( abs( HMap[y][x]-HMap[y+1][x+1] ) > abs( HMap[y+1][x]-HMap[y][x+1] ) )
                FMap[y][x] |= fmReverse;
            else
                FMap[y][x] &= ~fmReverse;
        }
#endif


    for (y=0; y<ctMapSize; y++)
        for (x=0; x<ctMapSize; x++)
        {

            if (!(FMap[y][x] & fmWaterA))
                WMap[y][x]=255;

#ifdef _soft
            if (MObjects[OMap[y][x]].info.flags & ofNOSOFT2)
                if ( (x+y) & 1 )
                    OMap[y][x]=255;

            if (MObjects[OMap[y][x]].info.flags & ofNOSOFT)
                OMap[y][x]=255;
#endif

            if (OMap[y][x]==254)
            {
                LandingList.list[LandingList.PCount].x = x;
                LandingList.list[LandingList.PCount].y = y;
                LandingList.PCount++;
                OMap[y][x]=255;
            }

            int ob = OMap[y][x];
            if (ob == 255)
            {
                HMapO[y][x] = 0;
                continue;
            }

            //HMapO[y][x] = GetObjectH(x,y);
            if (MObjects[ob].info.flags & ofPLACEGROUND) HMapO[y][x] = GetObjectH(x,y, MObjects[ob].info.GrRad);
            //if (MObjects[ob].info.flags & ofPLACEWATER)  HMapO[y][x] = GetObjectHWater(x,y);

        }

    if (!LandingList.PCount)
    {
        LandingList.list[LandingList.PCount].x = 256;
        LandingList.list[LandingList.PCount].y = 256;
        LandingList.PCount=1;
    }

    if (TrophyMode)
    {
        LandingList.PCount = 0;
        for (x=0; x<6; x++)
        {
            LandingList.list[LandingList.PCount].x = 69 + x*3;
            LandingList.list[LandingList.PCount].y = 66;
            LandingList.PCount++;
        }

        for (y=0; y<6; y++)
        {
            LandingList.list[LandingList.PCount].x = 87;
            LandingList.list[LandingList.PCount].y = 69 + y*3;
            LandingList.PCount++;
        }

        for (x=0; x<6; x++)
        {
            LandingList.list[LandingList.PCount].x = 84 - x*3;
            LandingList.list[LandingList.PCount].y = 87;
            LandingList.PCount++;
        }

        for (y=0; y<6; y++)
        {
            LandingList.list[LandingList.PCount].x = 66;
            LandingList.list[LandingList.PCount].y = 84 - y*3;
            LandingList.PCount++;
        }
    }


}



int CalcImageDifference(uint16_t* A, uint16_t* B, int L)
{
    int r = 0;
    L*=L;
    for (int l=0; l<L; l++)
    {
        uint16_t C1 = *(A + l);
        uint16_t C2 = *(B + l);
        int R1 = (C1>>10) & 31;
        int G1 = (C1>> 5) & 31;
        int B1 = (C1>> 0) & 31;
        int R2 = (C2>>10) & 31;
        int G2 = (C2>> 5) & 31;
        int B2 = (C2>> 0) & 31;

        r+=(R1-R2)*(R1-R2) +
           (G1-G2)*(G1-G2) +
           (B1-B2)*(B1-B2);
    }

    return r;
}


void RotateImage(uint16_t* src, uint16_t* dst, int L)
{
    for (int y=0; y<L; y++)
        for (int x=0; x<L; x++)
            *(dst + x*L + (L-1-y) ) = *(src + x + y*L);
}


void BrightenTexture(uint16_t* A, int L)
{
    int factor=OptBrightness + 128;
    //if (factor > 256) factor = (factor-256)*3/2 + 256;
    for (int c=0; c<L; c++)
    {
        uint16_t w = *(A +  c);
        int B = (w>> 0) & 31;
        int G = (w>> 5) & 31;
        int R = (w>>10) & 31;
        B = (B * factor) >> 8;
        if (B > 31) B = 31;
        G = (G * factor) >> 8;
        if (G > 31) G = 31;
        R = (R * factor) >> 8;
        if (R > 31) R = 31;

        if (OptDayNight==2)
            if (NightVision)
            {
                R=0;//G>>4;
                B=0;//G>>4;
                if (G>27)
                {
                    R=31;
                    B=31;
                    G=31;
                };
                if (G>31) G=31;
                if (G<0) G=0;
            }

        *(A + c) = (B) + (G<<5) + (R<<10);
    }
}

void GenerateMipMap(uint16_t* A, uint16_t* D, int L)
{
    for (int y=0; y<L; y++)
        for (int x=0; x<L; x++)
        {
            int C1 = *(A + x*2 +   (y*2+0)*2*L);
            int C2 = *(A + x*2+1 + (y*2+0)*2*L);
            int C3 = *(A + x*2 +   (y*2+1)*2*L);
            int C4 = *(A + x*2+1 + (y*2+1)*2*L);
            int B = ( ((C1>>0) & 31) + ((C2>>0) & 31) + ((C3>>0) & 31) + ((C4>>0) & 31) +2 ) >> 2;
            int G = ( ((C1>>5) & 31) + ((C2>>5) & 31) + ((C3>>5) & 31) + ((C4>>5) & 31) +2 ) >> 2;
            int R = ( ((C1>>10) & 31) + ((C2>>10) & 31) + ((C3>>10) & 31) + ((C4>>10) & 31) +2 ) >> 2;

            // Opacity Fix
            if (R<3 && G<3 && B<3)
            {
                R=0;
                G=0;
                B=0;
            }

            *(D + x + y * L) = HiColor(R,G,B);
        }
}


int CalcColorSum(uint16_t* A, int L)
{
    int R = 0, G = 0, B = 0;
    for (int x=0; x<L; x++)
    {
        B+= (*(A+x) >> 0) & 31;
        G+= (*(A+x) >> 5) & 31;
        R+= (*(A+x) >>10) & 31;
    }
    return HiColor(R/L, G/L, B/L);
}


void GenerateShadedMipMap(uint16_t* src, uint16_t* dst, int L)
{
    for (int x=0; x<16*16; x++)
    {
        int B = (*(src+x) >> 0) & 31;
        int G = (*(src+x) >> 5) & 31;
        int R = (*(src+x) >>10) & 31;
        R=DitherHi(SkyR*L/8 + R*(256-L)+6);
        G=DitherHi(SkyG*L/8 + G*(256-L)+6);
        B=DitherHi(SkyB*L/8 + B*(256-L)+6);
        *(dst + x) = HiColor(R,G,B);
    }
}


void GenerateShadedSkyMipMap(uint16_t* src, uint16_t* dst, int L)
{
    for (int x=0; x<128*128; x++)
    {
        int B = (*(src+x) >> 0) & 31;
        int G = (*(src+x) >> 5) & 31;
        int R = (*(src+x) >>10) & 31;
        R=DitherHi(SkyR*L/8 + R*(256-L)+6);
        G=DitherHi(SkyG*L/8 + G*(256-L)+6);
        B=DitherHi(SkyB*L/8 + B*(256-L)+6);
        *(dst + x) = HiColor(R,G,B);
    }
}


void DATASHIFT(uint16_t* d, int cnt)
{
    cnt>>=1;
    /*
    for (int l=0; l<cnt; l++)
      *(d+l)=(*(d+l)) & 0x3e0;
    */
    if (HARD3D) return;

    for (int l=0; l<cnt; l++)
        *(d+l)*=2;

}



void ApplyAlphaFlags(void* tptr, int cnt)
{
#ifdef _soft
#else
    //for (int w=0; w<cnt; w++)
    //	*(tptr+w)|=0x8000;
#endif
}


void CalcMidColor(uint16_t* tptr, int l, uint8_t &mr, uint8_t &mg, uint8_t &mb)
{
    for (int w=0; w<l; w++)
    {
        uint16_t c = *(tptr + w);
        mb+=((c>> 0) & 31)*8;
        mg+=((c>> 5) & 31)*8;
        mr+=((c>>10) & 31)*8;
    }

    mr/=l;
    mg/=l;
    mb/=l;
}

void LoadTexture(CTexture* &T)
{
    T = new CTexture();
    T->m_width 		= 128;
    T->m_height 	= 128;
    T->m_bpp		= 16;

    T->Release();
	T->Allocate();

    fread( T->m_data, 128*128*2, 1, hfile );

    // Alpha Flags
    for (int y=0; y<128; y++)
    {
        for (int x=0; x<128; x++)
        {

            if (!*((uint16_t*)T->m_data + (y*128+x))) *((uint16_t*)T->m_data + (y*128+x)) = 1;
        }
    }

    BrightenTexture((uint16_t*)T->m_data, 128*128);

    CalcMidColor( (uint16_t*)T->m_data, 128*128, T->m_rgb.R, T->m_rgb.G, T->m_rgb.B );

    //DATASHIFT((unsigned short *)T, sizeof(CTexture));

    ApplyAlphaFlags( (uint16_t*)T->m_data, 128*128 );

	// -- Create the Graphics Memory texture
    oglCreateTexture( false, true, T );
}




void LoadSky()
{
	SkyTexture.m_width	= 256;
	SkyTexture.m_height	= 256;
	SkyTexture.m_bpp	= 16;

	SkyTexture.Release();
	SkyTexture.Allocate();

    fseek( hfile, 256*512*OptDayNight, SEEK_CUR );
    fread( SkyTexture.m_data, 256*256*2, 1, hfile );
    fseek( hfile, 256*512*(2-OptDayNight), SEEK_CUR );

    BrightenTexture( (uint16_t*)SkyTexture.m_data, 256*256);

    for (int y=0; y<128; y++)
        for (int x=0; x<128; x++)
            *((uint16_t*)SkyFade + (y*128+x)) = *((uint16_t*)SkyTexture.m_data + (y*2*256  + x*2));

    ApplyAlphaFlags( (uint16_t*)SkyTexture.m_data, 256*256);
}


void LoadSkyMap()
{
    fread( SkyMap, 128*128, 1, hfile );
}





void fp_conv(void* d)
{
    int i;
    float f;
    memcpy(&i, d, 4);

    f = ((float)i);// / 256.f;

    memcpy(d, &f, 4);
}



void CorrectModel(TModel *mptr)
{
    TFace tface[ MAX_TRIANGLES ];

    for (uint32_t f=0; f<mptr->FCount; f++)
    {
        if (!(mptr->gFace[f].Flags & sfDoubleSide))
            mptr->gFace[f].Flags |= sfNeedVC;

        fp_conv(&mptr->gFace[f].tax);
        fp_conv(&mptr->gFace[f].tay);
        fp_conv(&mptr->gFace[f].tbx);
        fp_conv(&mptr->gFace[f].tby);
        fp_conv(&mptr->gFace[f].tcx);
        fp_conv(&mptr->gFace[f].tcy);
    }


    int fp = 0;
    for (uint32_t f=0; f<mptr->FCount; f++)
        if ( (mptr->gFace[f].Flags & (sfOpacity | sfTransparent))==0)
        {
            //tface[fp] = mptr->gFace[f];
            memcpy( &tface[fp], &mptr->gFace[fp], sizeof( TFacef ) );
            fp++;
        }

    for (uint32_t f=0; f<mptr->FCount; f++)
        if ( (mptr->gFace[f].Flags & sfOpacity)!=0)
        {
            //tface[fp] = mptr->gFace[f];
            memcpy( &tface[fp], &mptr->gFace[fp], sizeof( TFacef ) );
            fp++;
        }

    for (uint32_t f=0; f<mptr->FCount; f++)
        if ( (mptr->gFace[f].Flags & sfTransparent)!=0)
        {
            //tface[fp] = mptr->gFace[f];
            memcpy( &tface[fp], &mptr->gFace[fp], sizeof( TFacef ) );
            fp++;
        }



    memcpy( mptr->gFace, tface, sizeof(tface) );
}

void LoadModel(TModel* &mptr)
{
	if ( mptr ) { assert( 0 ); }
    mptr = new TModel();

    fread( &mptr->VCount,      4,         1, hfile );
    fread( &mptr->FCount,      4,         1, hfile );
    fread( &OCount,            4,         1, hfile );
    fread( &mptr->TextureSize, 4,         1, hfile );
    fread( mptr->gFace,        mptr->FCount<<6, 1, hfile );
    fread( mptr->gVertex,      mptr->VCount<<4, 1, hfile );
    fread( gObj,               OCount*48, 1, hfile );

    if (HARD3D) CalcLights(mptr);

    int ts = mptr->TextureSize;
    mptr->TextureHeight = 256;
    mptr->TextureSize = mptr->TextureHeight*512;

    mptr->m_texture.m_width = 256;
    mptr->m_texture.m_height = 256;
    mptr->m_texture.m_bpp = 16;

    mptr->m_texture.Release();
    mptr->m_texture.Allocate();

    fread( mptr->m_texture.m_data, ts, 1, hfile );
    BrightenTexture( (uint16_t*)mptr->m_texture.m_data, ts/2);

    for (uint32_t v=0; v<mptr->VCount; v++)
    {
        mptr->gVertex[v].x*=2.f;
        mptr->gVertex[v].y*=2.f;
        mptr->gVertex[v].z*=-2.f;
    }

    CorrectModel(mptr);

    //DATASHIFT(mptr->lpTexture, mptr->TextureSize);
	oglCreateTexture( true, true, &mptr->m_texture );
}



void LoadAnimation(TVTL &vtl)
{
    int vc;

    fread( &vc,          4,    1, hfile );
    fread( &vc,          4,    1, hfile );
    fread( &vtl.aniKPS,  4,    1, hfile );
    fread( &vtl.FramesCount,  4,    1, hfile );
    vtl.FramesCount++;

    vtl.AniTime = (vtl.FramesCount * 1000) / vtl.aniKPS;
    vtl.aniData = new int16_t [ vc*vtl.FramesCount*6 ];
    fread( vtl.aniData, (vc*vtl.FramesCount*6), 1, hfile );

}



void LoadModelEx(TModel* &mptr, char* FName)
{
    hfile = fopen( FName, "rb" );

    if ( !hfile )
    {
        char sz[512];
        sprintf( sz, "Error opening model file\n%s.", FName );
        DoHalt(sz);
    }

    mptr = new TModel();

    fread( &mptr->VCount,      4,         1, hfile );
    fread( &mptr->FCount,      4,         1, hfile );
    fread( &OCount,            4,         1, hfile );
    fread( &mptr->TextureSize, 4,         1, hfile );
    fread( mptr->gFace,        mptr->FCount<<6, 1, hfile );
    fread( mptr->gVertex,      mptr->VCount<<4, 1, hfile );
    fread( gObj,               OCount*48, 1, hfile );

    int ts = mptr->TextureSize;
    if (HARD3D) mptr->TextureHeight = 256;
    else  mptr->TextureHeight = mptr->TextureSize>>9;
    mptr->TextureSize = mptr->TextureHeight*512;

	mptr->m_texture.m_width = 256;
    mptr->m_texture.m_height = 256;
    mptr->m_texture.m_bpp = 16;

    mptr->m_texture.Release();
    mptr->m_texture.Allocate();

    fread( mptr->m_texture.m_data, ts, 1, hfile );
    BrightenTexture( (uint16_t*)mptr->m_texture.m_data, ts/2);

    for (uint32_t v=0; v<mptr->VCount; v++)
    {
        mptr->gVertex[v].x*=2.f;
        mptr->gVertex[v].y*=2.f;
        mptr->gVertex[v].z*=-2.f;
    }

    CorrectModel(mptr);

    oglCreateTexture( true, true, &mptr->m_texture );

    //DATASHIFT(mptr->lpTexture, mptr->TextureSize);
}




void LoadWav(char* FName, TSFX &sfx)
{
    FILE* hfile = fopen( FName, "rb" );

    if( !hfile )
    {
        char sz[512];
        sprintf( sz, "Error opening sound file\n%s.", FName );
        DoHalt(sz);
    }

    if ( sfx.lpData ) free( sfx.lpData );
    sfx.lpData = NULL;

    fseek( hfile, 36, SEEK_SET );

    char c[5];
    c[4] = 0;

    while ( true )
    {
        fread( c, 1, 1, hfile );
        if( c[0] == 'd' )
        {
            fread( &c[1], 3, 1, hfile );
            if( !strcmp( c, "data" ) ) break;
            else fseek( hfile, -3, SEEK_CUR );
        }
    }

    fread( &sfx.length, 4, 1, hfile );

    sfx.lpData = (int16_t*)malloc( sfx.length );
    if ( !sfx.lpData )
    {
    	DoHalt( "fucking audio failed to be allocated" );
    }

    fread( sfx.lpData, sfx.length, 1, hfile );
    fclose( hfile );
}


uint16_t conv_565(uint16_t c)
{
    if (HARD3D) return c;
    else return (c & 31) + ( (c & 0xFFE0) << 1 );
}


int conv_xGx(int c)
{
    if (OptDayNight!=2) return c;
    if (!NightVision) return c;
    uint32_t a = c;
    int r = ((c>> 0) & 0xFF);
    int g = ((c>> 8) & 0xFF);
    int b = ((c>>16) & 0xFF);
    c = std::max(r,g);
    c = std::max(c,b);
    return (c<<8) + (a & 0xFF000000);
}

void conv_pic(TPicture &pic)
{
    if (!HARD3D) return;

    for (int y=0; y<pic.m_height; y++)
        for (int x=0; x<pic.m_width; x++)
            *((uint16_t*)pic.m_data + x + y*pic.m_width) = conv_565(*((uint16_t*)pic.m_data + x + y*pic.m_width));
}



/*
	LoadPicture()
	Load a Picture from a valid image file.
	Supports:
	.BMP
	.TGA
*/
void LoadPicture(TPicture &pic, const char* pname)
{
	char fname[260];
	strcpy( fname, pname );
    strlwr( fname );
    if ( strstr( fname, ".bmp" ) )
    {
        LoadPictureBMP( pic, pname );
    }
    if ( strstr( fname, ".tga" ) )
    {
        LoadPictureTGA( pic, pname );
    }
}


/*
	LoadPictureBMP()
	Load a Picture from a BMP file.
	// TODO (Rexhunter99#1#): Use C++ new/delete, reflecting HEAP removal.
*/
void LoadPictureBMP(TPicture &pic, const char* pname)
{
    BITMAPFILEHEADER bmpFH;
    BITMAPINFOHEADER bmpIH;
    FILE* hfile;

    hfile = fopen( pname, "rb" );
    if( !hfile )
    {
        char sz[512];
        sprintf( sz, "Error opening bmp file\n%s.", pname );
        DoHalt(sz);
    }

    fread( &bmpFH, sizeof( BITMAPFILEHEADER ), 1, hfile );
    fread( &bmpIH, sizeof( BITMAPINFOHEADER ), 1, hfile );

	pic.m_width =			(uint16_t)bmpIH.biWidth;
    pic.m_height=			(uint16_t)bmpIH.biHeight;
    pic.m_bpp	=			(uint16_t)bmpIH.biBitCount;

    pic.Release();
    pic.Allocate();

    //printf( "LoadPictureBMP() : m_width(%u), m_height(%u), m_bpp(%u)\n", pic.m_width, pic.m_height, pic.m_bpp );

    fread( pic.m_data, pic.m_width*(pic.m_bpp/8)*pic.m_height, 1, hfile );

    fclose( hfile );
}


/*
	LoadPictureTGA()
	Load a Picture from a TGA file.
	ToDo:
    [Y] Use C++ new/delete, reflecting HEAP removal.
*/
void LoadPictureTGA(TPicture &pic, const char* pname)
{
    uint32_t	l;
    uint8_t		ColorMap;
    uint8_t		ImageType;
    void*		raster;
    FILE*		hfile;
    TARGAFILEHEADER tga;

    hfile = fopen( pname, "rb" );

    if( !hfile )
    {
        char sz[512];
        sprintf( sz, "Error opening picture file\n%s", pname );
        DoHalt(sz);
    }

    fread( &tga, sizeof(TARGAFILEHEADER), 1, hfile );

    pic.m_width = tga.tgaWidth;
    pic.m_height = tga.tgaHeight;
    pic.m_bpp = tga.tgaBits;

    if ( tga.tgaImageType != 2 ) { fclose( hfile ); return; }

    //fprintf( stderr, "LoadTGA() : %u, %u, %u\n", pic.m_width, pic.m_height, pic.m_bpp );

    /*fseek( hfile, 1, SEEK_SET );
    fread( &ColorMap, 1, 1, hfile );
    fread( &ImageType, 1, 1, hfile );

    fseek( hfile, 12, SEEK_SET );

    fread( &pic.m_width, 2, 1, hfile );
    fread( &pic.m_height, 2, 1, hfile );
    fread( &pic.m_bpp, 1, 1, hfile );

    fseek( hfile, 18, SEEK_SET );*/

    /*
    	Release the existing memory
    */
    pic.Release();

    /*
    	Allocate memory for the image
    */
    pic.Allocate();
    /*if ( pic.m_bpp == 16 )
    {
        pic.m_data = malloc( sizeof(uint16_t) * (pic.m_width * pic.m_height) );
    }
    if ( pic.m_bpp == 24 )
    {
        pic.m_data = malloc( sizeof(rgb_t) * (pic.m_width * pic.m_height) );
    }
    if ( pic.m_bpp == 32 )
    {
        pic.m_data = malloc( sizeof(rgba_t) * (pic.m_width * pic.m_height) );
    }*/

    /*
    	Read the data from the file
    */
    //fread( pic.m_data, pic.m_width*pic.m_height, pic.m_bpp/8, hfile );
    for ( int y=pic.m_height-1; y>=0; y-- )
    {
		fread( (uint8_t*)pic.m_data + ( (pic.m_width*(pic.m_bpp/8))*y ), pic.m_width*(pic.m_bpp/8), 1, hfile );
    }

    /*
    	Close the file handle
    */
    fclose( hfile );

	//printf( "LoadPictureTGA() : m_bpp(%d)\n", pic.m_bpp );

    /*
		Create the texture in graphics memory
	*/
	//pic.m_texid = RenCreateTexture( false, pic.m_width, pic.m_height, 16, pic.m_data );
}


void CreateMipMapMT(uint16_t* dst, uint16_t* src, int H)
{
    for (int y=0; y<H; y++)
        for (int x=0; x<127; x++)
        {
            int C1 = *(src + (x*2+0) + (y*2+0)*256);
            int C2 = *(src + (x*2+1) + (y*2+0)*256);
            int C3 = *(src + (x*2+0) + (y*2+1)*256);
            int C4 = *(src + (x*2+1) + (y*2+1)*256);

            if (!HARD3D)
            {
                C1>>=1;
                C2>>=1;
                C3>>=1;
                C4>>=1;
            }

            /*if (C1 == 0 && C2!=0) C1 = C2;
              if (C1 == 0 && C3!=0) C1 = C3;
              if (C1 == 0 && C4!=0) C1 = C4;*/

            if (C1 == 0)
            {
                *(dst + x + y*128) = 0;
                continue;
            }

            //C4 = C1;

            if (!C2) C2=C1;
            if (!C3) C3=C1;
            if (!C4) C4=C1;

            int B = ( ((C1>> 0) & 31) + ((C2 >>0) & 31) + ((C3 >>0) & 31) + ((C4 >>0) & 31) +2 ) >> 2;
            int G = ( ((C1>> 5) & 31) + ((C2 >>5) & 31) + ((C3 >>5) & 31) + ((C4 >>5) & 31) +2 ) >> 2;
            int R = ( ((C1>>10) & 31) + ((C2>>10) & 31) + ((C3>>10) & 31) + ((C4>>10) & 31) +2 ) >> 2;
            if (!HARD3D) *(dst + x + y * 128) = HiColor(R,G,B)*2;
            else *(dst + x + y * 128) = HiColor(R,G,B);
        }
}



void GetObjectCaracteristics(TModel* mptr, int& ylo, int& yhi)
{
    ylo = 10241024;
    yhi =-10241024;
    for (int v=0; v<mptr->VCount; v++)
    {
        if (mptr->gVertex[v].y < ylo) ylo = (int)mptr->gVertex[v].y;
        if (mptr->gVertex[v].y > yhi) yhi = (int)mptr->gVertex[v].y;
    }
    if (yhi<ylo) yhi=ylo+1;
}



void GenerateAlphaFlags(TModel *mptr)
{
#ifdef _soft
#else

    int w;
    bool Opacity = false;
    uint16_t* tptr = (uint16_t*)mptr->m_texture.m_data;

    for (w=0; w<mptr->FCount; w++)
        if ((mptr->gFace[w].Flags & sfOpacity)>0) Opacity = true;

    if (Opacity)
    {
        for (w=0; w<256*256; w++)
            if ( *(tptr+w)>0 ) *(tptr+w)=(*(tptr+w)) + 0x8000;
    }
    else
        for (w=0; w<256*256; w++)
            *(tptr+w)=(*(tptr+w)) + 0x8000;

#endif
}


#define RGB15( r, g, b ) ((uint16_t) (b&31) | ((g&31)<<5) | ((r&31)<<10) | (1<<15) )
#define RGB15_R( c ) ((BYTE) ( (c>>10) & 31 ) )
#define RGB15_G( c ) ((BYTE) ( (c>>5 ) & 31 ) )
#define RGB15_B( c ) ((BYTE) ( (c>>0 ) & 31 ) )

void GenerateMapImage()
{
    int YShift = 23;
    int XShift = 11;
    int lsw = MapPic.m_width;

    for (int y=0; y<256; y++)
        for (int x=0; x<256; x++)
        {
            int t;
            uint16_t c;

            if (FMap[y<<2][x<<2] & fmWater)
            {
                t = WaterList[WMap[y<<2][x<<2]].tindex;
                c = *((uint16_t*)Textures[t]->m_data ); // TODO: use modulus and multiplication
            }
            else
            {
                t = TMap1[y<<2][x<<2];
                c = *( (uint16_t*)Textures[t]->m_data + ( (x%128) + (y%128) * 128 ) );
                float l = (float)(LMap[y<<2][x<<2]) / 255.0f;

                int R = RGB15_R( c ) * l;
                int G = RGB15_G( c ) * l;
                int B = RGB15_B( c ) * l;

                // Old Style
                c = *((uint16_t*)Textures[t]->m_data);
                // iOS Style
                c = RGB15( R,G,B );
            }

            *((uint16_t*)MapPic.m_data + ((y+YShift)*lsw + x + XShift) ) = c;
        }

	// -- High resolution radar imagery RadarPic
	uint16_t* img = (uint16_t*)RadarPic.m_data;

	for (int y=0; y<1024; y++)
	{
        for (int x=0; x<1024; x++)
        {
        	int i = ( x + y * 1024 );

        	if (FMap[y][x] & fmWater)
            {
                *( img + i ) = *((uint16_t*)Textures[ WaterList[WMap[y][x]].tindex ]->m_data + ( ( (x%32) + (y%32) * 128 ) ) );
            }
            else
            {
            	uint16_t c = *( (uint16_t*)Textures[ TMap1[y][x] ]->m_data + ( (x%128) + (y%128) * 128 ) );
            	float l = (float)(LMap[y][x]) / 255.0f;

				int R = RGB15_R( c ) * l;
                int G = RGB15_G( c ) * l;
                int B = RGB15_B( c ) * l;

				*( img + i ) = RGB15( R,G,B );
            }
        }
	}

	FILE* fp = fopen( "MAP.TGA", "wb" );

	if ( fp )
	{
		TARGAFILEHEADER tga;

		tga.tgaIdentSize		= 0;
		tga.tgaColorMapType		= 0;
		tga.tgaImageType		= TARGA_RGBA;

		tga.tgaColorMapOffset	= 0;
		tga.tgaColorMapLength	= 0;
		tga.tgaColorMapBits		= 0;

		tga.tgaXStart			= 0;
		tga.tgaYStart			= 0;
		tga.tgaWidth			= 1024;
		tga.tgaHeight			= 1024;
		tga.tgaBits				= 16;
		tga.tgaDescriptor		= 0;

		fwrite( &tga, sizeof(TARGAFILEHEADER), 1, fp );
		fwrite( RadarPic.m_data, 1024*1024*2, 1, fp );

		fclose(fp);
	}
}



void ReleaseResources()
{
    HeapReleased=0;

    for (int t=0; t<1024; t++)
    {
        if (Textures[t])
        {
            delete Textures[t];
            Textures[t] = 0;
        }
        else
        {
            break;
        }
    }

    for (int m=0; m<255; m++)
    {
        TModel *mptr = MObjects[m].model;
        if (mptr)
        {
			MObjects[m].bmpmodel.m_texture.Release();

            if (MObjects[m].vtl.FramesCount>0)
            {
            	delete [] MObjects[m].vtl.aniData;
                MObjects[m].vtl.aniData = NULL;
            }

			mptr->m_texture.Release();

			delete MObjects[m].model;
			MObjects[m].model = 0;
        }
        else continue;
    }

    for (int a=0; a<255; a++)
    {
        if (!Ambient[a].sfx.lpData) continue; // We want to skip this particular one if its null.
        delete [] Ambient[a].sfx.lpData;
        Ambient[a].sfx.lpData = 0;
    }

    for (int r=0; r<255; r++)
    {
        if (!RandSound[r].lpData) continue; // We want to skip this particular one if its null.
        delete [] RandSound[r].lpData;
        RandSound[r].lpData = 0;
        RandSound[r].length = 0;
    }

    free( RadarPic.m_data );
    RadarPic.m_data = 0;
}


void LoadBMPModel(TObject &obj)
{
    obj.bmpmodel.m_texture.m_width = 128;
    obj.bmpmodel.m_texture.m_height = 128;
    obj.bmpmodel.m_texture.m_bpp = 16;

    obj.bmpmodel.m_texture.Release();
    obj.bmpmodel.m_texture.Allocate();

    fread( obj.bmpmodel.m_texture.m_data, 128*128, sizeof( uint16_t ), hfile );

    BrightenTexture( (uint16_t*)obj.bmpmodel.m_texture.m_data, 128*128);
    //DATASHIFT(obj.bmpmodel.lpTexture, 128*128*2);

    if (HARD3D) // -- we're always hardware now, so this is a redundant check
    {
		for (int x=0; x<128; x++)
		{
			for (int y=0; y<128; y++)
			{
				//if ( *(obj.bmpmodel.lpTexture + x + y*128) )
                 //   *(obj.bmpmodel.lpTexture + x + y*128) |= 0x8000;
			}
		}
    }

	oglCreateTexture( true, false, &obj.bmpmodel.m_texture );

    float mxx = obj.model->gVertex[0].x+0.5f;
    float mnx = obj.model->gVertex[0].x-0.5f;

    float mxy = obj.model->gVertex[0].x+0.5f;
    float mny = obj.model->gVertex[0].y-0.5f;

    for (int v=0; v<obj.model->VCount; v++)
    {
        float x = obj.model->gVertex[v].x;
        float y = obj.model->gVertex[v].y;
        if (x > mxx) mxx=x;
        if (x < mnx) mnx=x;
        if (y > mxy) mxy=y;
        if (y < mny) mny=y;
    }

    obj.bmpmodel.gVertex[0].x = mnx;
    obj.bmpmodel.gVertex[0].y = mxy;
    obj.bmpmodel.gVertex[0].z = 0;

    obj.bmpmodel.gVertex[1].x = mxx;
    obj.bmpmodel.gVertex[1].y = mxy;
    obj.bmpmodel.gVertex[1].z = 0;

    obj.bmpmodel.gVertex[2].x = mxx;
    obj.bmpmodel.gVertex[2].y = mny;
    obj.bmpmodel.gVertex[2].z = 0;

    obj.bmpmodel.gVertex[3].x = mnx;
    obj.bmpmodel.gVertex[3].y = mny;
    obj.bmpmodel.gVertex[3].z = 0;
}


void LoadResources()
{

    int  FadeRGB[3][3];
    int TransRGB[3][3];

    int tc,mc;
    char MapName[128],RscName[128];
    HeapAllocated=0;
    if (strstr(ProjectName, "TROPHY"))
    {
        TrophyMode = true;
        ctViewR = 60;
        ctViewR1 = 48;
    }
    sprintf(MapName,"%s%s", ProjectName, ".MAP");
    sprintf(RscName,"%s%s", ProjectName, ".RSC");

    ReleaseResources();

	/* Open the Area RSC file */
    hfile = fopen( RscName, "rb" );

    if ( !hfile )
    {
        char sz[512];
        sprintf( sz, "Error opening resource file\n\"%s\"\n", RscName );
        DoHalt(sz);
        return;
    }

    fread( &tc, 4, 1, hfile);
    fread( &mc, 4, 1, hfile);


    fread(  FadeRGB, 4*3*3, 1, hfile);
    fread( TransRGB, 4*3*3, 1, hfile);

    SkyR  =  FadeRGB[OptDayNight][0];
    SkyG  =  FadeRGB[OptDayNight][1];
    SkyB  =  FadeRGB[OptDayNight][2];

    SkyTR = TransRGB[OptDayNight][0];
    SkyTG = TransRGB[OptDayNight][1];
    SkyTB = TransRGB[OptDayNight][2];

    if (OptDayNight==2)
    {
        SkyR = 0;
        SkyB = 0;
        SkyTR = 0;
        SkyTB = 0;
    }

    SkyTR = std::min(255,SkyTR * (OptBrightness + 128) / 256);
    SkyTG = std::min(255,SkyTG * (OptBrightness + 128) / 256);
    SkyTB = std::min(255,SkyTB * (OptBrightness + 128) / 256);

    SkyR = std::min(255,SkyR * (OptBrightness + 128) / 256);
    SkyG = std::min(255,SkyG * (OptBrightness + 128) / 256);
    SkyB = std::min(255,SkyB * (OptBrightness + 128) / 256);


    PrintLog("Loading textures:");
    for (int tt=0; tt<tc; tt++)
    {
        LoadTexture(Textures[tt]);
    }
    PrintLog(" Done.\n");
    TextureCount = tc;



    PrintLog("Loading models:");
    PrintLoad("Loading models...");
    for (int mm=0; mm<mc; mm++)
    {
        fread( &MObjects[mm].info, 64, 1, hfile);
        MObjects[mm].info.Radius*=2;
        MObjects[mm].info.YLo*=2;
        MObjects[mm].info.YHi*=2;
        MObjects[mm].info.linelenght = (MObjects[mm].info.linelenght / 128) * 128;
        LoadModel(MObjects[mm].model);
        LoadBMPModel(MObjects[mm]);

        if (MObjects[mm].info.flags & ofNOLIGHT)
            memset( MObjects[mm].model->VLight, 0, 4*1024*4 );

        if (MObjects[mm].info.flags & ofANIMATED)
            LoadAnimation(MObjects[mm].vtl);


        MObjects[mm].info.BoundR = 0;
        for (int v=0; v<MObjects[mm].model->VCount; v++)
        {
            float r = (float)sqrt(MObjects[mm].model->gVertex[v].x * MObjects[mm].model->gVertex[v].x +
                                  MObjects[mm].model->gVertex[v].z * MObjects[mm].model->gVertex[v].z );
            if (r>MObjects[mm].info.BoundR) MObjects[mm].info.BoundR=r;
        }

        if (MObjects[mm].info.flags & ofBOUND)
            CalcBoundBox(MObjects[mm].model, MObjects[mm].bound);

        //GenerateModelMipMaps(MObjects[mm].model);
        //GenerateAlphaFlags(MObjects[mm].model);
    }
    PrintLog(" Done.\n");
    ModelCount = mc;




    PrintLoad("Finishing with .res...");
    PrintLog("Finishing with .res:");
    LoadSky();
    LoadSkyMap();

    int FgCount;
    fread( &FgCount, 4, 1, hfile);
    fread( &FogsList[1], FgCount * sizeof(TFogEntity), 1, hfile);


    for (int f=0; f<=FgCount; f++)
    {
        int fb = (FogsList[f].fogRGB >> 00) & 0xFF;
        int fg = (FogsList[f].fogRGB >>  8) & 0xFF;
        int fr = (FogsList[f].fogRGB >> 16) & 0xFF;
        FogsList[f].fogRGB = (fr) + (fg<<8) + (fb<<16);
        if (OptDayNight==2) FogsList[f].fogRGB&=0x00FF00;
    }


    int RdCount, AmbCount, WtrCount;

    fread( &RdCount, 4, 1, hfile);
    for (int r=0; r<RdCount; r++)
    {
        fread( &RandSound[r].length, 4, 1, hfile);
        if ( RandSound[r].lpData ) free( RandSound[r].lpData );
        RandSound[r].lpData = (int16_t*) malloc( RandSound[r].length );
        fread( RandSound[r].lpData, RandSound[r].length, 1, hfile);
    }

    fread( &AmbCount, 4, 1, hfile);
    for (int a=0; a<AmbCount; a++)
    {
        fread( &Ambient[a].sfx.length, 4, 1, hfile);
        if ( Ambient[a].sfx.lpData ) free( Ambient[a].sfx.lpData );
        Ambient[a].sfx.lpData = (int16_t*) malloc( Ambient[a].sfx.length );
        fread( Ambient[a].sfx.lpData, Ambient[a].sfx.length, 1, hfile);

        fread( Ambient[a].rdata, sizeof(Ambient[a].rdata), 1, hfile);
        fread( &Ambient[a].RSFXCount, 4, 1, hfile);
        fread( &Ambient[a].AVolume, 4, 1, hfile);

        if (Ambient[a].RSFXCount)
            Ambient[a].RndTime = (Ambient[a].rdata[0].RFreq / 2 + rRand(Ambient[a].rdata[0].RFreq)) * 1000;

        int F = Ambient[a].rdata[0].RFreq;
        int E = Ambient[a].rdata[0].REnvir;
/////////////////

        //sprintf(logt,"Env=%d  Flag=%d  Freq=%d\n", E, Ambient[a].rdata[0].Flags, F);
        //PrintLog(logt);

        if (OptDayNight==2)
            for (int r=0; r<Ambient[a].RSFXCount; r++)
                if (Ambient[a].rdata[r].Flags)
                {
                    if (r!=15) memcpy(&Ambient[a].rdata[r], &Ambient[a].rdata[r+1], (15-r)*sizeof(TRD));
                    Ambient[a].RSFXCount--;
                    r--;
                }

        Ambient[a].rdata[0].RFreq = F;
        Ambient[a].rdata[0].REnvir = E;


    }

    fread( &WtrCount, 4, 1, hfile);
    fread( WaterList, 16*WtrCount, 1, hfile);

    WaterList[255].wlevel = 0;
    for (int w=0; w<WtrCount; w++)
    {
        memcpy( &WaterList[w].fogRGB, &Textures[WaterList[w].tindex]->m_rgb, sizeof(rgb_t) );
    }
    fclose(hfile);
    PrintLog(" Done.\n");




//================ Load MAPs file ==================//
    PrintLoad("Loading .map...");
    PrintLog("Loading .map:");
    hfile = fopen(MapName, "rb" );

    if ( !hfile )
        DoHalt("Error opening map file.");

    fread( HMap,    1024*1024, 1, hfile);
    fread( TMap1,   1024*1024*2, 1, hfile);
    fread( TMap2,   1024*1024*2, 1, hfile);
    fread( OMap,    1024*1024, 1, hfile);
    fread( FMap,    1024*1024*2, 1, hfile);
    fseek( hfile, 1024*1024*OptDayNight, SEEK_CUR );
    fread( LMap,    1024*1024, 1, hfile);
    fseek( hfile, 1024*1024*(2-OptDayNight), SEEK_CUR );
    fread( WMap ,   1024*1024, 1, hfile);
    fread( HMapO,   1024*1024, 1, hfile);
    fread( FogsMap, 512*512, 1, hfile);
    fread( AmbMap,  512*512, 1, hfile);

    if (FogsList[1].YBegin>1.f)
        for (int x=0; x<510; x++)
            for (int y=0; y<510; y++)
                if (!FogsMap[y][x])
                    if (HMap[y*2+0][x*2+0]<FogsList[1].YBegin || HMap[y*2+0][x*2+1]<FogsList[1].YBegin || HMap[y*2+0][x*2+2] < FogsList[1].YBegin ||
                            HMap[y*2+1][x*2+0]<FogsList[1].YBegin || HMap[y*2+1][x*2+1]<FogsList[1].YBegin || HMap[y*2+1][x*2+2] < FogsList[1].YBegin ||
                            HMap[y*2+2][x*2+0]<FogsList[1].YBegin || HMap[y*2+2][x*2+1]<FogsList[1].YBegin || HMap[y*2+2][x*2+2] < FogsList[1].YBegin)
                        FogsMap[y][x] = 1;

    fclose(hfile);
    PrintLog(" Done.\n");




//======= Post load rendering ==============//
    PrintLoad("Prepearing maps...");
    CreateTMap();
    RenderLightMap();

    LoadPictureTGA(MapPic, "HUNTDAT/MENU/MAPFRAME.TGA");
    conv_pic(MapPic);

    RadarPic.m_cwidth = RadarPic.m_width = 1024;
    RadarPic.m_cheight = RadarPic.m_height = 1024;
    RadarPic.m_bpp = 16;
    RadarPic.Release();
    RadarPic.Allocate();

    GenerateMapImage();

    if (TrophyMode) LoadPictureTGA(TrophyPic, "HUNTDAT/MENU/TROPHY.TGA");
    else LoadPictureTGA(TrophyPic, "HUNTDAT/MENU/TROPHY_G.TGA");
    conv_pic(TrophyPic);

//    ReInitGame();
}



void LoadCharacters()
{
    bool pres[64];
    memset( pres, 0, sizeof(pres) );
    pres[0]=true;
    for (int c=0; c<ChCount; c++)
    {
        pres[Characters[c].CType] = true;
    }

    for (int c=0; c<TotalC; c++) if (pres[c])
        {
            if (!ChInfo[c].mptr)
            {
                sprintf(logt, "HUNTDAT/%s", DinoInfo[c].FName);
                LoadCharacterInfo(ChInfo[c], logt);
                PrintLog("Loading: ");
                PrintLog(logt);
                PrintLog("\n");
            }
        }

#ifdef _iceage // alacn
    for (int c=10; c<20; c++)
#else
    for (int c=10; c<19; c++)
#endif
        if (TargetDino & (1<<c))
            if (!DinoInfo[AI_to_CIndex[c]].CallIcon.m_data)
            {
                sprintf(logt, "HUNTDAT/MENU/PICS/CALL%d.TGA", c-9);
                LoadPicture(DinoInfo[AI_to_CIndex[c]].CallIcon, logt);
                oglCreateSprite( true, DinoInfo[AI_to_CIndex[c]].CallIcon );
                conv_pic(DinoInfo[AI_to_CIndex[c]].CallIcon);
            }


    for (int c=0; c<TotalW; c++)
        if (WeaponPres & (1<<c))
        {
            if (!Weapon.chinfo[c].mptr)
            {
                sprintf(logt, "HUNTDAT/WEAPONS/%s", WeapInfo[c].FName);
                LoadCharacterInfo(Weapon.chinfo[c], logt);
                PrintLog("Loading: ");
                PrintLog(logt);
                PrintLog("\n");
            }


            if (!Weapon.BulletPic[c].m_data)
            {
                sprintf(logt, "HUNTDAT/WEAPONS/%s", WeapInfo[c].BFName);
                LoadPicture(Weapon.BulletPic[c], logt);
                oglCreateSprite( true, Weapon.BulletPic[c] );
                conv_pic(Weapon.BulletPic[c]);
                PrintLog("Loading: ");
                PrintLog(logt);
                PrintLog("\n");
            }

        }

#ifdef _iceage // alacn
    for (int c=10; c<20; c++)
#else
    for (int c=10; c<20; c++)
#endif
        if (TargetDino & (1<<c))
            if ( !fxCall[c-10][0] )
            {
                sprintf(logt,"HUNTDAT/SOUNDFX/CALLS/CALL%d_A.WAV", (c-9));
                fxCall[c-10][0] = g_AudioDevice->loadSound( logt );
                sprintf(logt,"HUNTDAT/SOUNDFX/CALLS/CALL%d_B.WAV", (c-9));
                fxCall[c-10][0] = g_AudioDevice->loadSound( logt );
                sprintf(logt,"HUNTDAT/SOUNDFX/CALLS/CALL%d_C.WAV", (c-9));
                fxCall[c-10][0] = g_AudioDevice->loadSound( logt );
            }
}

void ReInitGame()
{
    PrintLog("ReInitGame();\n");
    PlaceHunter();
    if (TrophyMode)	PlaceTrophy();
    else PlaceCharacters();

    LoadCharacters();

    LockLanding = false;
    SupplyUsed = false;
    Wind.alpha = rRand(1024) * 2.f * pi / 1024.f;
    Wind.speed = 10;
    MyHealth = MAX_HEALTH;
    TargetWeapon = -1;

    for (int w=0; w<TotalW; w++)
        if ( WeaponPres & (1<<w) )
        {
            ShotsLeft[w] = WeapInfo[w].Shots;
            if (DoubleAmmo) AmmoMag[w] = 1;
            if (TargetWeapon==-1) TargetWeapon=w;
        }

    CurrentWeapon = TargetWeapon;

    Weapon.state = 0;
    Weapon.FTime = 0;
    PlayerAlpha = 0;
    PlayerBeta  = 0;

    WCCount = 0;
    SnCount = 0;
    ElCount = 0;
    BloodTrail.Count = 0;
    g->BINMODE = false;
    g->OPTICMODE = false;
    g->EXITMODE = false;
    g->PAUSE = false;

    Ship.pos.x = PlayerX;
    Ship.pos.z = PlayerZ;
    Ship.pos.y = GetLandUpH(Ship.pos.x, Ship.pos.z) + 2048;
    Ship.State = -1;
    Ship.tgpos.x = Ship.pos.x;
    Ship.tgpos.z = Ship.pos.z + 60*256;
    Ship.cindex  = -1;
    Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + 2048;
    ShipTask.tcount = 0;

    if (!TrophyMode)
    {
        TrophyRoom.Last.smade = 0;
        TrophyRoom.Last.success = 0;
        TrophyRoom.Last.path  = 0;
        TrophyRoom.Last.time  = 0;
    }

    DemoPoint.DemoTime = 0;
    RestartMode = false;
    TrophyTime=0;
    answtime = 0;
    ExitTime = 0;
}



void ReleaseCharacterInfo(TCharacterInfo &chinfo)
{
    if (!chinfo.mptr) return;

	delete [] chinfo.mptr;
	chinfo.mptr = 0;

    for (int c = 0; c<64; c++)
    {
        if (!chinfo.Animation[c].aniData) break;
        delete []  chinfo.Animation[c].aniData;
        chinfo.Animation[c].aniData = NULL;
    }

    for (int c = 0; c<64; c++)
    {
        //if (!chinfo.SoundFX[c].lpData) break;
        //delete [] chinfo.SoundFX[c].lpData;
        //chinfo.SoundFX[c].lpData = NULL;
        g_AudioDevice->freeSound( chinfo.SoundFX[c] );
        chinfo.SoundFX[c] = 0;
    }

    chinfo.AniCount = 0;
    chinfo.SfxCount = 0;
}




void LoadCharacterInfo(TCharacterInfo &chinfo, const char* FName)
{
    ReleaseCharacterInfo(chinfo);

    FILE* hfile = fopen(FName, "rb" );

    if ( !hfile )
    {
        char sz[512];
        sprintf( sz, "Error opening character file:\n%s", FName );
        DoHalt(sz);
    }

    fread( chinfo.ModelName, 32, 1, hfile);
    fread( &chinfo.AniCount,  4, 1, hfile);
    fread( &chinfo.SfxCount,  4, 1, hfile);

//============= read model =================//

    chinfo.mptr = new TModel();

    fread( &chinfo.mptr->VCount,      4,         1, hfile );
    fread( &chinfo.mptr->FCount,      4,         1, hfile );
    fread( &chinfo.mptr->TextureSize, 4,         1, hfile );
    fread( chinfo.mptr->gFace,        chinfo.mptr->FCount<<6, 1, hfile );
    fread( chinfo.mptr->gVertex,      chinfo.mptr->VCount<<4, 1, hfile );

    int ts = chinfo.mptr->TextureSize;
    if (HARD3D) chinfo.mptr->TextureHeight = 256;
    else  chinfo.mptr->TextureHeight = chinfo.mptr->TextureSize>>9;
    chinfo.mptr->TextureSize = chinfo.mptr->TextureHeight*512;

	chinfo.mptr->m_texture.m_width = 256;
    chinfo.mptr->m_texture.m_height = 256;
    chinfo.mptr->m_texture.m_bpp = 16;

    chinfo.mptr->m_texture.Release();
    chinfo.mptr->m_texture.Allocate();

    fread( chinfo.mptr->m_texture.m_data, ts, 1, hfile);
    BrightenTexture( (uint16_t*)chinfo.mptr->m_texture.m_data, ts/2);

    oglCreateTexture(true, true, &chinfo.mptr->m_texture );

    //DATASHIFT(chinfo.mptr->m_texture.m_data, chinfo.mptr->TextureSize);

    //============= read animations =============//
    for (int a=0; a<chinfo.AniCount; a++)
    {
        fread( chinfo.Animation[a].aniName, 32, 1, hfile);
        fread( &chinfo.Animation[a].aniKPS, 4, 1, hfile);
        fread( &chinfo.Animation[a].FramesCount, 4, 1, hfile);
        chinfo.Animation[a].AniTime = (chinfo.Animation[a].FramesCount * 1000) / chinfo.Animation[a].aniKPS;
        chinfo.Animation[a].aniData = new int16_t [chinfo.mptr->VCount*chinfo.Animation[a].FramesCount*3];

        fread( chinfo.Animation[a].aniData, (chinfo.mptr->VCount*chinfo.Animation[a].FramesCount*6), 1, hfile);
    }

//============= read sound fx ==============//
    BYTE tmp[32];
    for (int s=0; s<chinfo.SfxCount; s++)
    {
		uint32_t	pcm_length = 0;
		uint8_t*	pcm_data = 0;

        fread( tmp, 32, 1, hfile);
        fread( &pcm_length, 4, 1, hfile);

        pcm_data = new uint8_t [ pcm_length ];
        fread( pcm_data, pcm_length, 1, hfile);

        chinfo.SoundFX[s] = g_AudioDevice->loadSound( pcm_data, pcm_length, 22050, 1, 16 );

        delete [] pcm_data;
    }

    for (int v=0; v<chinfo.mptr->VCount; v++)
    {
        chinfo.mptr->gVertex[v].x*=2.f;
        chinfo.mptr->gVertex[v].y*=2.f;
        chinfo.mptr->gVertex[v].z*=-2.f;
    }

    CorrectModel(chinfo.mptr);


    size_t l = fread( chinfo.Anifx, 64*4, 1, hfile);
    if (l!=1)
    {
        for (l=0; l<64; l++)
        {
        	chinfo.Anifx[l] = -1;
        }
    }

    fclose(hfile);
}


//================ light map ========================//



void FillVector(int x, int y, vec3& v)
{
    v.x = (float)x*256;
    v.z = (float)y*256;
    v.y = (float)((int)HMap[y][x])*ctHScale;
}

bool TraceVector(vec3 v, vec3 lv)
{
    v.y+=4;
    NormVector(lv,64);
    for (int l=0; l<32; l++)
    {
        v.x-=lv.x;
        v.y-=lv.y/6;
        v.z-=lv.z;
        if (v.y>255 * ctHScale) return true;
        if (GetLandH(v.x, v.z) > v.y) return false;
    }
    return true;
}


void AddShadow(int x, int y, int d)
{
    if (x<0 || y<0 || x>1023 || y>1023) return;
    int l = LMap[y][x];
    l-=d;
    if (l<32) l=32;
    LMap[y][x]=l;
}

void RenderShadowCircle(int x, int y, int R, int D)
{
    int cx = x / 256;
    int cy = y / 256;
    int cr = 1 + R / 256;
    for (int yy=-cr; yy<=cr; yy++)
        for (int xx=-cr; xx<=cr; xx++)
        {
            int tx = (cx+xx)*256;
            int ty = (cy+yy)*256;
            int r = (int)sqrtf( float( (tx-x)*(tx-x) + (ty-y)*(ty-y) ) );
            if (r>R) continue;
            AddShadow(cx+xx, cy+yy, D * (R-r) / R);
        }
}

void RenderLightMap()
{
    vec3 lv;
    int x,y;

    lv.x = - 412;
    lv.z = - 412;
    lv.y = - 1024;
    NormVector(lv, 1.0f);

    for (y=1; y<ctMapSize-1; y++)
        for (x=1; x<ctMapSize-1; x++)
        {
            int ob = OMap[y][x];
            if (ob == 255) continue;

            int l = MObjects[ob].info.linelenght / 128;
            int s = 1;
            if (OptDayNight==2) s=-1;
            if (OptDayNight!=1) l = MObjects[ob].info.linelenght / 70;
            if (l>0) RenderShadowCircle(x*256+128,y*256+128, 256, MObjects[ob].info.lintensity * 2);
            for (int i=1; i<l; i++)
                AddShadow(x+i*s, y+i*s, MObjects[ob].info.lintensity);

            l = MObjects[ob].info.linelenght * 2;
            RenderShadowCircle(x*256+128+l*s,y*256+128+l*s,
                               MObjects[ob].info.circlerad*2,
                               MObjects[ob].info.cintensity*4);
        }

}





void SaveScreenShot()
{
    FILE* hf;					/* file handle */
    BITMAPFILEHEADER hdr;       /* bitmap file-header */
    BITMAPINFOHEADER bmi;       /* bitmap info-header */

    //if (WinW>1024) return;

    //MessageBeep(0xFFFFFFFF);

    if ( !CopyHARDToDIB() ) return;
    return;

    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biWidth = WinW;
    bmi.biHeight = WinH;
    bmi.biPlanes = 1;
    bmi.biBitCount = 24;
    bmi.biCompression = BI_RGB;

    bmi.biSizeImage = WinW * WinH * 3;
    bmi.biClrImportant = 0;
    bmi.biClrUsed = 0;



    hdr.bfType = 0x4d42;
    hdr.bfSize = (uint32_t) (sizeof(BITMAPFILEHEADER) +
                             bmi.biSize + bmi.biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;
    hdr.bfOffBits = (uint32_t) sizeof(BITMAPFILEHEADER) +
                    bmi.biSize;


    // char t[12];
    char t[16]; // 12 + null

    //CreateDirectory( "Screenshots", 0 );

    sprintf(t,"Screenshots\\HUNT%04d.BMP",++_shotcounter);
    hf = fopen( t, "wb" );

	if ( !hf ) return;

    fwrite( (void*) &hdr, sizeof(BITMAPFILEHEADER), 1, hf );

    fwrite( &bmi, sizeof(BITMAPINFOHEADER), 1, hf );

    uint8_t *fRGB = new uint8_t [WinW*3];

    for (int y=0; y<WinH; y++)
    {
        for (int x=0; x<WinW; x++)
        {
            uint16_t C = *((uint16_t*)lpScreenBuf + (WinEY-y)*WinW+x);

            //Blue
            fRGB[x*3+0] = (C & 31)<<3;

            if (HARD3D) //565 format
            {
                //Green -> Red
                fRGB[x*3+1] = ((C>> 5) & 63)<<2;
                fRGB[x*3+2] = ((C>>11) & 31)<<3;
            }
            else //555 format
            {
                //Green -> Red
                fRGB[x*3+1] = ((C>> 5) & 31)<<3;
                fRGB[x*3+2] = ((C>>10) & 31)<<3;
            }
        }

        fwrite( fRGB, WinW*3, 1, hf );
    }

    delete [] fRGB;

    fclose(hf);
    ////MessageBeep(0xFFFFFFFF);
}












//===============================================================================================
void ReadWeapons(FILE *stream)
{
    TotalW = 0;
    char line[256];
    while (fgets( line, 255, stream))
    {

        if (strstr(line, "}")) break;

        if (strstr(line, "{"))
            while (fgets( line, 255, stream))
            {
                if (strstr(line, "}"))
                {
                    TotalW++;
                    break;
                }
                if (strstr(line, "//")) continue;
                if (strstr(line, "--")) continue;

				char full_line[256];
				memcpy( full_line, line, 255 );

                char* var = strtok( line, "\t =\n" );
                char* value = 0;
                char* offset = 0;

                if ( ( offset = strstr( full_line, "\'" ) ) ) // string
                {
                	value = strtok( offset, ";\n" );
                }
                else if ( ( offset = strstr( full_line, "\"" ) ) )
                {
                	value = strtok( offset, ";\n" );
                }
                else
                {
                	value = strtok( 0, "\t =;\n" );
                }

                if (!value)
                {
                    DoHalt("_RES.txt :: Weapon value is invalid!");
                }
                if (!var)
                {
                    DoHalt("_RES.txt :: Weapon variable is invalid!");
                }

                if (!strcmp(var, "power"))  WeapInfo[TotalW].Power = (float)atof(value);
                if (!strcmp(var, "prec"))   WeapInfo[TotalW].Prec  = (float)atof(value);
                if (!strcmp(var, "loud"))   WeapInfo[TotalW].Loud  = (float)atof(value);
                if (!strcmp(var, "rate"))   WeapInfo[TotalW].Rate  = (float)atof(value);
                if (!strcmp(var, "shots"))  WeapInfo[TotalW].Shots =        atoi(value);
                if (!strcmp(var, "reload")) WeapInfo[TotalW].Reload=        atoi(value);
                if (!strcmp(var, "trace"))  WeapInfo[TotalW].TraceC=        atoi(value)-1;
                if (!strcmp(var, "optic"))  WeapInfo[TotalW].Optic =        atoi(value);
                if (!strcmp(var, "fall"))   WeapInfo[TotalW].Fall  =        atoi(value);

                if (!strcmp(var, "hipx"))  WeapInfo[TotalW].HipX = (float)atof(value);
                if (!strcmp(var, "hipy"))  WeapInfo[TotalW].HipY = (float)atof(value);
                if (!strcmp(var, "hipz"))  WeapInfo[TotalW].HipZ = (float)atof(value);

                if (!strcmp(var, "crosshaircolor"))
                {
                    int red		= atoi( strtok( value, "," ) );
                    int green	= atoi( strtok( 0, "," ) );
                    int blue	= atoi( strtok( 0, ",;\n" ) );

                    WeapInfo[TotalW].CrossHairColor = (0x80<<24) | (red<<16) | (green<<8) | (blue<<0);
                }

                if (!strcmp(var, "crosshair"))		WeapInfo[TotalW].CrossHair = atoi(value);
                if (!strcmp(var, "crosshairtype"))	WeapInfo[TotalW].CrossHairType = atoi(value);
                if (!strcmp(var, "zoom"))			WeapInfo[TotalW].Zoom = (float)atof(value);

                if (!strcmp(var, "name"))
                {
                    size_t len = strstr( &value[1], "\'" ) - &value[1];
                    strncpy( WeapInfo[TotalW].Name, &value[1], len ); // fuck you CR LF line endings
                }

                if (!strcmp(var, "file"))
                {
                	size_t len = strstr( &value[1], "\'" ) - &value[1];
                    strncpy( WeapInfo[TotalW].FName, &value[1], len ); // fuck you CR LF line endings
                }

                if (!strcmp(var, "pic"))
                {
                	size_t len = strstr( &value[1], "\'" ) - &value[1];
                    strncpy( WeapInfo[TotalW].BFName, &value[1], len ); // fuck you CR LF line endings
                }
            }

    }

}


void ReadCharacters(FILE *stream)
{
    TotalC = 0;
    char line[256], *value;
    while (fgets( line, 255, stream))
    {
        if (strstr(line, "}")) break;
        if (strstr(line, "//")) continue;
        if (strstr(line, "{"))
            while (fgets( line, 255, stream))
            {
                if (strstr(line, "//")) continue;
                if (strstr(line, "--")) continue;

                if (strstr(line, "}"))
                {
                    AI_to_CIndex[DinoInfo[TotalC].AI] = TotalC;
                    TotalC++;
                    break;
                }

                value = strstr(line, "=");
                if (!value)
                    DoHalt("Script loading error");
                value++;

                if (strstr(line, "mass"     )) DinoInfo[TotalC].Mass      = (float)atof(value);
                if (strstr(line, "length"   )) DinoInfo[TotalC].Length    = (float)atof(value);
                if (strstr(line, "radius"   )) DinoInfo[TotalC].Radius    = (float)atof(value);
                if (strstr(line, "health"   )) DinoInfo[TotalC].Health0   = atoi(value);
                if (strstr(line, "basescore")) DinoInfo[TotalC].BaseScore = atoi(value);
                if (strstr(line, "ai"       )) DinoInfo[TotalC].AI        = atoi(value);
                if (strstr(line, "smell"    )) DinoInfo[TotalC].SmellK    = (float)atof(value);
                if (strstr(line, "hear"     )) DinoInfo[TotalC].HearK     = (float)atof(value);
                if (strstr(line, "look"     )) DinoInfo[TotalC].LookK     = (float)atof(value);
                if (strstr(line, "shipdelta")) DinoInfo[TotalC].ShDelta   = (float)atof(value);
                if (strstr(line, "scale0"   )) DinoInfo[TotalC].Scale0    = atoi(value);
                if (strstr(line, "scaleA"   )) DinoInfo[TotalC].ScaleA    = atoi(value);
                if (strstr(line, "danger"   )) DinoInfo[TotalC].DangerCall= true;
                if (strstr(line, "group"   )) DinoInfo[TotalC].Group		= atoi(value);

                if (strstr(line, "name"))
                {
                    value = strstr(line, "'");
                    if (!value) DoHalt("Script loading error: Failed to find a string!");
					value++;

                    //strncpy( DinoInfo[TotalC].Name, &value[1], strlen(value)-4 );

					for ( int i=0; true; i++ )
					{
						if ( i == 47 ) break;
						if ( *value == '\'' || *value == '\n' || *value == 0 ) break;
						DinoInfo[TotalC].Name[i] = *value++;
					}
                }

                if (strstr(line, "file"))
                {
                    value = strstr(line, "'");
                    if (!value) DoHalt("Script loading error: Failed to find a string!");
                    value++;

                    //strncpy( DinoInfo[TotalC].FName, &value[1], strlen(value)-4 ); // fuck you CR LF line endings

					for ( int i=0; true; i++ )
					{
						if ( i == 47 ) break;
						if ( *value == '\'' || *value == '\n' || *value == 0 ) break;
						DinoInfo[TotalC].FName[i] = *value++;
					}
                }

                if (strstr(line, "pic"))
                {
                    value = strstr(line, "'");
                    if (!value) DoHalt("Script loading error: Failed to find a string!");
                    strncpy( DinoInfo[TotalC].PName, &value[1], strlen(value)-4 );
					if(false)
					{
					}
                }

                // -- [RH99] AI Data File
                if (strstr(line, "aidata"))
                {
                    value = strstr(line, "'");
                    if (!value) DoHalt("Script loading error");
                    value[strlen(value)-2] = 0;
                    strcpy(DinoInfo[TotalC].PName, &value[1]);
					if(false){}
                }
            }

    }
}



void LoadResourcesScript()
{
    FILE *stream;
    char line[256];

    stream = fopen("HUNTDAT/_RES.TXT", "r");
    if (!stream) DoHalt("Can't open resources file \"HUNTDAT/_RES.TXT\"\n");

    while (fgets( line, 255, stream))
    {
        if (line[0] == '.') break;
        if (strstr(line, "//") ) continue;
        if (strstr(line, "weapons") ) ReadWeapons(stream);
        if (strstr(line, "characters") ) ReadCharacters(stream);
    }

    fclose (stream);
}
//===============================================================================================





void CreateLog()
{
    // TODO (Rexhunter99#1#): check the file handles to make sure they are valid

    hlog = fopen( "GAME.LOG", "w" );

	fprintf( hlog, "================================================\n" );
    fprintf( hlog, "\tAtmosFEAR 2\n" );
    fprintf( hlog, "\tVersion: %u.%u.%u r%u\n", Version::MAJOR, Version::MINOR, Version::BUILD, Version::REVISION );
    fprintf( hlog, "\tPlatform: %s\n", V_PLATFORM );
    fprintf( hlog, "\tDate: %s\n", __DATE__ );
	fprintf( hlog, "================================================\n" );
}


void PrintLog(const char* l)
{

    // TODO: Awaiting \r\n fix
    // -> Hack
    /*for (unsigned int i=0; i<strlen(l); i++)
    {
    	BYTE c = l[i];

    	if (l[i] == 0x0A && l[i-1] != 0x0D )
    	{WriteFile(hlog, "\r\n", 2, &w, NULL); continue;}
    	//if (c == 0x0D)

    	WriteFile(hlog, &c, 1, &w, NULL);
    }*/
    fwrite( l, strlen(l), 1, hlog );

    /* -> Original
    if (l[strlen(l)-1]==0x0A)
    {
    	BYTE b = 0x0D;
    	WriteFile(hlog, l, strlen(l)-1, &w, NULL);
    	WriteFile(hlog, &b, 1, &w, NULL);
    	b = 0x0A;
    	WriteFile(hlog, &b, 1, &w, NULL);
    }
    else WriteFile(hlog, l, strlen(l), &w, NULL);*/
}

void CloseLog()
{
    if ( hlog ) fclose(hlog);
    hlog = 0;
}
