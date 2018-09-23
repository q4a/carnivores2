#include "Hunt.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

void SetupRes()
{
    if ( CustomRes ) return;

    if (!HARD3D) if (OptRes>5) OptRes=5;

    //4:3
    if (OptRes==0)
    {
        WinW = 320;
        WinH=240;
    }
    if (OptRes==1)
    {
        WinW = 400;
        WinH=300;
    }
    if (OptRes==2)
    {
        WinW = 512;
        WinH=384;
    }
    if (OptRes==3)
    {
        WinW = 640;
        WinH=480;
    }
    if (OptRes==4)
    {
        WinW = 800;
        WinH=600;
    }
    if (OptRes==5)
    {
        WinW =1024;
        WinH=768;
    }
    if (OptRes==6)
    {
        WinW =1280;
        WinH=1024;
    }
    if (OptRes==7)
    {
        WinW =1600;
        WinH=1200;
    }

    //16:9
    if (OptRes==8 )
    {
        WinW =  960;
        WinH=540;
    }
    if (OptRes==9 )
    {
        WinW = 1024;
        WinH=576;
    }
    if (OptRes==10)
    {
        WinW = 1280;
        WinH=720;
    }
    if (OptRes==11)
    {
        WinW = 1366;
        WinH=768;
    }
    if (OptRes==12)
    {
        WinW = 1600;
        WinH=900;
    }
    if (OptRes==13)
    {
        WinW = 1920;
        WinH=1080;
    }
    if (OptRes==14)
    {
        WinW = 2048;
        WinH=1152;
    }

    //16:10
    if (OptRes==15)
    {
        WinW =768;
        WinH=480;
    }
    if (OptRes==16)
    {
        WinW =1024;
        WinH=640;
    }
    if (OptRes==17)
    {
        WinW =1152;
        WinH=720;
    }
    if (OptRes==18)
    {
        WinW =1280;
        WinH=800;
    }
    if (OptRes==19)
    {
        WinW =1440;
        WinH=900;
    }
    if (OptRes==20)
    {
        WinW =1680;
        WinH=1050;
    }
    if (OptRes==21)
    {
        WinW =1920;
        WinH=1200;
    }
    if (OptRes==22)
    {
        WinW =2304;
        WinH=1440;
    }

    Aspect = (float(WinW) / float(WinH));
}


float GetLandOH(int x, int y)
{
    return (float)(HMapO[y][x]) * ctHScale;
}


float GetLandOUH(int x, int y)
{
    if (FMap[y][x] & fmReverse)
        return (float)((int)(HMap[y][x+1]+HMap[y+1][x])/2.f)*ctHScale;
    else
        return (float)((int)(HMap[y][x]+HMap[y+1][x+1])/2.f)*ctHScale;
}



float GetLandUpH(float x, float y)
{

    int CX = (int)x / 256;
    int CY = (int)y / 256;

    if (!(FMap[CY][CX] & fmWaterA)) return GetLandH(x,y);

    return (float)(WaterList[ WMap[CY][CX] ].wlevel * ctHScale);

}


bool IsInWater(float x, float y)
{

    int CX = (int)x / 256;
    int CY = (int)y / 256;

    if (!(FMap[CY][CX] & fmWaterA)) return false;

    return true;

}


float GetLandH(float x, float y)
{
    // Get the current height of the ground at x,y
    int CX = (int)x / 256;
    int CY = (int)y / 256;

    int dx = (int)x % 256;
    int dy = (int)y % 256;

    int h1 = HMap[CY][CX];
    int h2 = HMap[CY][CX+1];
    int h3 = HMap[CY+1][CX+1];
    int h4 = HMap[CY+1][CX];


    if (FMap[CY][CX] & fmReverse)
    {
        if (256-dx>dy) h3 = h2+h4-h1;
        else h1 = h2+h4-h3;
    }
    else
    {
        if (dx>dy) h4 = h1+h3-h2;
        else h2 = h1+h3-h4;
    }

    float h = (float)
              (h1   * (256-dx) + h2 * dx) * (256-dy) +
              (h4   * (256-dx) + h3 * dx) * dy;

    return  (h / 256.f / 256.f) * ctHScale;
}



float GetLandLt(float x, float y)
{
    // Get the light intensity of the ground at x,y
    int CX = (int)x / 256;
    int CY = (int)y / 256;

    int dx = (int)x % 256;
    int dy = (int)y % 256;

    int h1 = LMap[CY][CX];
    int h2 = LMap[CY][CX+1];
    int h3 = LMap[CY+1][CX+1];
    int h4 = LMap[CY+1][CX];

    float h = (float)
              (h1   * (256-dx) + h2 * dx) * (256-dy) +
              (h4   * (256-dx) + h3 * dx) * dy;

    return  (h / 256.f / 256.f);
}



float GetLandLt2(float x, float y)
{
    int CX = ((int)x / 512)*2 - CCX;
    int CY = ((int)y / 512)*2 - CCY;

    int dx = (int)x % 512;
    int dy = (int)y % 512;

    int h1 = VMap[CY+128][CX+128].Light;
    int h2 = VMap[CY+128][CX+2+128].Light;
    int h3 = VMap[CY+2+128][CX+2+128].Light;
    int h4 = VMap[CY+2+128][CX+128].Light;

    float h = (float)
              (h1   * (512-dx) + h2 * dx) * (512-dy) +
              (h4   * (512-dx) + h3 * dx) * dy;

    return  (h / 512.f / 512.f);
}



void CalcModelGroundLight(TModel *mptr, float x0, float z0, int FI)
{
    float ca = (float)cos(FI * pi / 2);
    float sa = (float)sin(FI * pi / 2);
    for (uint32_t v=0; v<mptr->VCount; v++)
    {
        float x = mptr->gVertex[v].x * ca + mptr->gVertex[v].z * sa + x0;
        float z = mptr->gVertex[v].z * ca - mptr->gVertex[v].x * sa + z0;
#if defined(_d3d) || defined(_gl)
        mptr->VLight[0][v] = (int)GetLandLt2(x, z) - 128;
#else
        mptr->VLight[0][v] = (float)GetLandLt2(x, z) - 128;
#endif
    }
}


bool PointOnBound(float &H, float px, float py, float cx, float cy, float oy, TBound *bound, int angle)
{
    px-=cx;
    py-=cy;

    float ca = (float) cos(angle*pi / 2.f);
    float sa = (float) sin(angle*pi / 2.f);

    bool _on = false;
    H=-10000;

    for (int o=0; o<8; o++)
    {

        if (bound[o].a<0) continue;
        if (bound[o].y2 + oy > PlayerY + 128) continue;

        float a,b;
        float ccx = bound[o].cx*ca + bound[o].cy*sa;
        float ccy = bound[o].cy*ca - bound[o].cx*sa;

        if (angle & 1)
        {
            a = bound[o].b;
            b = bound[o].a;
        }
        else
        {
            a = bound[o].a;
            b = bound[o].b;
        }

        if ( ( fabs(px - ccx) < a) &&  (fabs(py - ccy) < b) )
        {
            _on=true;
            if (H < bound[o].y2) H = bound[o].y2;
        }
    }

    return _on;
}



bool PointUnBound(float &H, float px, float py, float cx, float cy, float oy, TBound *bound, int angle)
{
    px-=cx;
    py-=cy;

    float ca = (float) cos(angle*pi / 2.f);
    float sa = (float) sin(angle*pi / 2.f);

    bool _on = false;
    H=+10000;

    for (int o=0; o<8; o++)
    {

        if (bound[o].a<0) continue;
        if (bound[o].y1 + oy < PlayerY + 128) continue;

        float a,b;
        float ccx = bound[o].cx*ca + bound[o].cy*sa;
        float ccy = bound[o].cy*ca - bound[o].cx*sa;

        if (angle & 1)
        {
            a = bound[o].b;
            b = bound[o].a;
        }
        else
        {
            a = bound[o].a;
            b = bound[o].b;
        }

        if ( ( fabs(px - ccx) < a) &&  (fabs(py - ccy) < b) )
        {
            _on=true;
            if (H > bound[o].y1) H = bound[o].y1;
        }
    }

    return _on;
}





float GetLandCeilH(float CameraX, float CameraZ)
{
    float h;

    h = GetLandH(CameraX, CameraZ) + 20480;

    int ccx = (int)CameraX / 256;
    int ccz = (int)CameraZ / 256;

    for (int z=-4; z<=4; z++)
        for (int x=-4; x<=4; x++)
            if (OMap[ccz+z][ccx+x]!=255)
            {
                int ob = OMap[ccz+z][ccx+x];

                float CR = (float)MObjects[ob].info.Radius - 1.f;

                float oz = (ccz+z) * 256.f + 128.f;
                float ox = (ccx+x) * 256.f + 128.f;

                float LandY = GetLandOH(ccx+x, ccz+z);

                if (!(MObjects[ob].info.flags & ofBOUND))
                {
                    if (MObjects[ob].info.YLo + LandY > h) continue;
                    if (MObjects[ob].info.YLo + LandY < PlayerY+100) continue;
                }

                float r = CR+1;

                if (MObjects[ob].info.flags & ofBOUND)
                {
                    float hh;
                    if (PointUnBound(hh, CameraX, CameraZ, ox, oz, LandY, MObjects[ob].bound, ((FMap[ccz+z][ccx+x] >> 2) & 3)  ) )
                        if (h > LandY + hh) h = LandY + hh;
                }
                else
                {
                    if (MObjects[ob].info.flags & ofCIRCLE)
                        r = (float) sqrt( (ox-CameraX)*(ox-CameraX) + (oz-CameraZ)*(oz-CameraZ) );
                    else
                        r = (float) std::max( fabs(ox-CameraX) , fabs(oz-CameraZ) );

                    if (r<CR) h = MObjects[ob].info.YLo + LandY;
                }

            }
    return h;
}



float GetLandQH(float CameraX, float CameraZ)
{
    float h,hh;

    h = GetLandH(CameraX, CameraZ);
    hh = GetLandH(CameraX-90.f, CameraZ-90.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX+90.f, CameraZ-90.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX-90.f, CameraZ+90.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX+90.f, CameraZ+90.f);
    if (hh>h) h=hh;

    hh = GetLandH(CameraX+128.f, CameraZ);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX-128.f, CameraZ);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX, CameraZ+128.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX, CameraZ-128.f);
    if (hh>h) h=hh;

    int ccx = (int)CameraX / 256;
    int ccz = (int)CameraZ / 256;

    for (int z=-4; z<=4; z++)
        for (int x=-4; x<=4; x++)
            if (OMap[ccz+z][ccx+x]!=255)
            {
                int ob = OMap[ccz+z][ccx+x];

                float CR = (float)MObjects[ob].info.Radius - 1.f;

                float oz = (ccz+z) * 256.f + 128.f;
                float ox = (ccx+x) * 256.f + 128.f;

                float LandY = GetLandOH(ccx+x, ccz+z);

                if (!(MObjects[ob].info.flags & ofBOUND))
                {
                    if (MObjects[ob].info.YHi + LandY < h) continue;
                    if (MObjects[ob].info.YHi + LandY > PlayerY+128) continue;
                    //if (MObjects[ob].info.YLo + LandY > PlayerY+256) continue;
                }

                float r = CR+1;

                if (MObjects[ob].info.flags & ofBOUND)
                {
                    float hh;
                    if (PointOnBound(hh, CameraX, CameraZ, ox, oz, LandY, MObjects[ob].bound, ((FMap[ccz+z][ccx+x] >> 2) & 3)  ) )
                        if (h < LandY + hh) h = LandY + hh;
                }
                else
                {
                    if (MObjects[ob].info.flags & ofCIRCLE)
                        r = (float) sqrt( (ox-CameraX)*(ox-CameraX) + (oz-CameraZ)*(oz-CameraZ) );
                    else
                        r = (float) std::max( fabs(ox-CameraX) , fabs(oz-CameraZ) );

                    if (r<CR) h = MObjects[ob].info.YHi + LandY;
                }

            }
    return h;
}


float GetLandHObj(float CameraX, float CameraZ)
{
    float h;

    h = 0;

    int ccx = (int)CameraX / 256;
    int ccz = (int)CameraZ / 256;

    for (int z=-3; z<=3; z++)
        for (int x=-3; x<=3; x++)
            if (OMap[ccz+z][ccx+x]!=255)
            {
                int ob = OMap[ccz+z][ccx+x];
                float CR = (float)MObjects[ob].info.Radius - 1.f;

                float oz = (ccz+z) * 256.f + 128.f;
                float ox = (ccx+x) * 256.f + 128.f;

                if (MObjects[ob].info.YHi + GetLandOH(ccx+x, ccz+z) < h) continue;
                if (MObjects[ob].info.YLo + GetLandOH(ccx+x, ccz+z) > PlayerY+256) continue;
                float r;
                if (MObjects[ob].info.flags & ofCIRCLE)
                    r = (float) sqrtf( (ox-CameraX)*(ox-CameraX) + (oz-CameraZ)*(oz-CameraZ) );
                else
                    r = (float) std::max( fabsf(ox-CameraX) , fabsf(oz-CameraZ) );

                if (r<CR)
                    h = MObjects[ob].info.YHi + GetLandOH(ccx+x, ccz+z);
            }

    return h;
}


float GetLandQHNoObj(float CameraX, float CameraZ)
{
    float h,hh;

    h = GetLandH(CameraX, CameraZ);
    hh = GetLandH(CameraX-90.f, CameraZ-90.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX+90.f, CameraZ-90.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX-90.f, CameraZ+90.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX+90.f, CameraZ+90.f);
    if (hh>h) h=hh;

    hh = GetLandH(CameraX+128.f, CameraZ);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX-128.f, CameraZ);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX, CameraZ+128.f);
    if (hh>h) h=hh;
    hh = GetLandH(CameraX, CameraZ-128.f);
    if (hh>h) h=hh;

    return h;
}


void ProcessCommandLine()
{
    if ( g->argc <= 1 )
    {
        strcpy( ProjectName, "HUNTDAT/AREAS/AREA2" );
        TargetDino = ( 8 ) * 1024; // Allo
        WeaponPres = 16;
        OptDayNight = 1;
        DoubleAmmo = true;
        RadarMode = true;
        return;
    }

    g->GREMEDY = false;

    for (int a=0; a<g->argc; a++)
    {
        char* s = g->argv[a];
        if (strstr(s,"x="))
        {
            PlayerX = (float)atof(&s[2])*256.f;
            LockLanding = true;
        }
        if (strstr(s,"y="))
        {
            PlayerZ = (float)atof(&s[2])*256.f;
            LockLanding = true;
        }
        if (strstr(s,"w="))
        {
            WinW = atoi(&s[2]);
            CustomRes=true;
        }
        if (strstr(s,"h="))
        {
            WinH = atoi(&s[2]);
            CustomRes=true;
        }
        if (strstr(s,"reg=")) TrophyRoom.RegNumber = atoi(&s[4]);
        if (strstr(s,"prj=")) strcpy(ProjectName, (s+4));
        if (strstr(s,"din=")) TargetDino = (atoi(&s[4])*1024);
        if (strstr(s,"wep=")) WeaponPres = atoi(&s[4]);
        if (strstr(s,"dtm=")) OptDayNight  = atoi(&s[4]);

        if (strstr(s,"-debug"))  g->DEBUG = true;
        if (strstr(s,"-gremedy"))g->GREMEDY = true;
        if (strstr(s,"-double")) DoubleAmmo = true;
        if (strstr(s,"-radar"))  RadarMode = true;
        //if (strstr(s,"-scent"))  RadarMode = true;
        //if (strstr(s,"-camo"))  RadarMode = true;
        if (strstr(s,"-nvision"))  NightVision = true;
        if (strstr(s,"-tranq"))  Tranq = true;
        if (strstr(s,"-observ")) ObservMode = true;
        if (strstr(s,"-survival"))  SurviveMode = true;
        if (strstr(s,"-window"))  Windowed = true;
        if (strstr(s,"-supply")) Supply = true;
    }
    Aspect = 2.0f - (float(WinW) / float(WinH));
}




void AddWCircle(float x, float z, float scale)
{
    if ( WCCount >= 126 ) return;

    WCircles[WCCount].pos.x = x;
    WCircles[WCCount].pos.z = z;
    WCircles[WCCount].pos.y = GetLandUpH(x, z);
    WCircles[WCCount].FTime = 0;
    WCircles[WCCount].scale = scale;
    WCCount++;
}


void AddShipTask(int cindex)
{
    TCharacter *cptr = &Characters[cindex];

    bool TROPHYON  = (GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) < 100) &&
                     (!Tranq);

    if (TROPHYON)
    {
        ShipTask.clist[ShipTask.tcount] = cindex;
        ShipTask.tcount++;
        g_AudioDevice->addVoice( ShipModel.SoundFX[3] );
    }

    //===== trophy =======//
    time_t ct = time(0);   // get time now
    struct tm * now = localtime( &ct );

    int t=0;
    for (t=0; t<23; t++)
        if (!TrophyRoom.Body[t].ctype) break;

    float score = (float)DinoInfo[Characters[cindex].CType].BaseScore;

    if (TrophyRoom.Last.success>1)
        score*=(1.f + TrophyRoom.Last.success / 10.f);

    if (!(TargetDino & (1<<Characters[cindex].AI)) ) score/=2.f;

    if (g->CHEATED) score = 0.0f;

    if (Tranq    ) score *= 1.25f;
    if (RadarMode) score *= 0.70f;
    if (ScentMode) score *= 0.80f;
    if (CamoMode ) score *= 0.85f;
    TrophyRoom.Score+=(int)score;


    if (!Tranq)
    {
        TrophyTime = 20 * 1000;
        TrophyBody = t;
        TrophyRoom.Body[t].ctype  = Characters[cindex].CType;
        TrophyRoom.Body[t].scale  = Characters[cindex].scale;
        TrophyRoom.Body[t].weapon = CurrentWeapon;
        TrophyRoom.Body[t].score  = (int)score;
        TrophyRoom.Body[t].phase  = (RealTime & 3);
        TrophyRoom.Body[t].time = (now->tm_hour<<10) + now->tm_min;
        TrophyRoom.Body[t].date = ( (now->tm_year + 1900)<<20) + ((now->tm_mon + 1)<<10) + now->tm_mday;
        TrophyRoom.Body[t].range = VectorLength( SubVectors(Characters[cindex].pos, PlayerPos) ) / 64.f;
        PrintLog("Trophy added: ");
        if (g->CHEATED) PrintLog("[CHEATED] ");
        PrintLog(DinoInfo[Characters[cindex].CType].Name);
        PrintLog("\n");
    }
}



void InitShip(int cindex)
{
    TCharacter *cptr = &Characters[cindex];

    Ship.DeltaY = 2048.f + DinoInfo[cptr->CType].ShDelta * cptr->scale;

    Ship.pos.x = 0;//PlayerX - 90*256;
    if (Ship.pos.x < 256) Ship.pos.x = PlayerX + 90*256;
    Ship.pos.z = 0;//PlayerZ - 90*256;
    if (Ship.pos.z < 256) Ship.pos.z = PlayerZ + 90*256;
    Ship.pos.y = GetLandUpH(Ship.pos.x, Ship.pos.z)  + Ship.DeltaY + 1024;

    Ship.tgpos.x = cptr->pos.x;
    Ship.tgpos.z = cptr->pos.z;
    Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z)  + Ship.DeltaY;
    Ship.State = 0;

    Ship.retpos = Ship.pos;
    Ship.cindex = cindex;
    Ship.FTime = 0;
}



void HideWeapon()
{
    TWeapon *wptr = &Weapon;
    if (g->UNDERWATER && !wptr->state) return;
    if (ObservMode || TrophyMode) return;

    if (wptr->state == 0)
    {
        if ( (ShotsLeft[CurrentWeapon]==0) && (AmmoMag[CurrentWeapon]==0) ) return;
        if (WeapInfo[CurrentWeapon].Optic) g->OPTICMODE = true;
        g_AudioDevice->addVoice( wptr->chinfo[CurrentWeapon].SoundFX[0] );
        wptr->FTime = 0;
        wptr->state = 1;
        g->BINMODE = false;
        g->MapMode = false;
        wptr->shakel = 0.2f;
        return;
    }

    if (wptr->state!=2 || wptr->FTime!=0) return;
    g_AudioDevice->addVoice( wptr->chinfo[CurrentWeapon].SoundFX[2] );
    wptr->state = 3;
    wptr->FTime = 0;
    g->OPTICMODE = false;
    return ;
}








void InitGameInfo()
{
    for (int c=0; c<32; c++)
    {
        DinoInfo[c].Scale0 = 800;
        DinoInfo[c].ScaleA = 600;
        DinoInfo[c].ShDelta = 0;
    }
    /*
        WeapInfo[0].Name = "Shotgun";
    	WeapInfo[0].Power = 1.5f;
    	WeapInfo[0].Prec  = 1.1f;
    	WeapInfo[0].Loud  = 0.3f;
    	WeapInfo[0].Rate  = 1.6f;
    	WeapInfo[0].Shots = 6;

    	WeapInfo[1].Name = "X-Bow";
    	WeapInfo[1].Power = 1.1f;
    	WeapInfo[1].Prec  = 0.7f;
    	WeapInfo[1].Loud  = 1.9f;
    	WeapInfo[1].Rate  = 1.2f;
    	WeapInfo[1].Shots = 8;

        WeapInfo[2].Name = "Sniper Rifle";
    	WeapInfo[2].Power = 1.0f;
    	WeapInfo[2].Prec  = 1.8f;
    	WeapInfo[2].Loud  = 0.6f;
    	WeapInfo[2].Rate  = 1.0f;
    	WeapInfo[2].Shots = 6;




    	DinoInfo[ 0].Name = "Moschops";
    	DinoInfo[ 0].Health0 = 2;
    	DinoInfo[ 0].Mass = 0.15f;

        DinoInfo[ 1].Name = "Galimimus";
    	DinoInfo[ 1].Health0 = 2;
    	DinoInfo[ 1].Mass = 0.1f;

    	DinoInfo[ 2].Name = "Dimorphodon";
        DinoInfo[ 2].Health0 = 1;
    	DinoInfo[ 2].Mass = 0.05f;

    	DinoInfo[ 3].Name = "Dimetrodon";
        DinoInfo[ 3].Health0 = 2;
    	DinoInfo[ 3].Mass = 0.22f;


    	DinoInfo[ 5].Name = "Parasaurolophus";
    	DinoInfo[ 5].Mass = 1.5f;
    	DinoInfo[ 5].Length = 5.8f;
    	DinoInfo[ 5].Radius = 320.f;
    	DinoInfo[ 5].Health0 = 5;
    	DinoInfo[ 5].BaseScore = 6;
    	DinoInfo[ 5].SmellK = 0.8f; DinoInfo[ 4].HearK = 1.f; DinoInfo[ 4].LookK = 0.4f;
    	DinoInfo[ 5].ShDelta = 48;

    	DinoInfo[ 6].Name = "Pachycephalosaurus";
    	DinoInfo[ 6].Mass = 0.8f;
    	DinoInfo[ 6].Length = 4.5f;
    	DinoInfo[ 6].Radius = 280.f;
    	DinoInfo[ 6].Health0 = 4;
    	DinoInfo[ 6].BaseScore = 8;
    	DinoInfo[ 6].SmellK = 0.4f; DinoInfo[ 5].HearK = 0.8f; DinoInfo[ 5].LookK = 0.6f;
    	DinoInfo[ 6].ShDelta = 36;

    	DinoInfo[ 7].Name = "Stegosaurus";
        DinoInfo[ 7].Mass = 7.f;
    	DinoInfo[ 7].Length = 7.f;
    	DinoInfo[ 7].Radius = 480.f;
    	DinoInfo[ 7].Health0 = 5;
    	DinoInfo[ 7].BaseScore = 7;
    	DinoInfo[ 7].SmellK = 0.4f; DinoInfo[ 6].HearK = 0.8f; DinoInfo[ 6].LookK = 0.6f;
    	DinoInfo[ 7].ShDelta = 128;

    	DinoInfo[ 8].Name = "Allosaurus";
    	DinoInfo[ 8].Mass = 0.5;
    	DinoInfo[ 8].Length = 4.2f;
    	DinoInfo[ 8].Radius = 256.f;
    	DinoInfo[ 8].Health0 = 3;
    	DinoInfo[ 8].BaseScore = 12;
    	DinoInfo[ 8].Scale0 = 1000;
    	DinoInfo[ 8].ScaleA = 600;
    	DinoInfo[ 8].SmellK = 1.0f; DinoInfo[ 7].HearK = 0.3f; DinoInfo[ 7].LookK = 0.5f;
    	DinoInfo[ 8].ShDelta = 32;
    	DinoInfo[ 8].DangerCall = true;

    	DinoInfo[ 9].Name = "Chasmosaurus";
    	DinoInfo[ 9].Mass = 3.f;
    	DinoInfo[ 9].Length = 5.0f;
    	DinoInfo[ 9].Radius = 400.f;
    	DinoInfo[ 9].Health0 = 8;
    	DinoInfo[ 9].BaseScore = 9;
    	DinoInfo[ 9].SmellK = 0.6f; DinoInfo[ 8].HearK = 0.5f; DinoInfo[ 8].LookK = 0.4f;
    	//DinoInfo[ 8].ShDelta = 148;
    	DinoInfo[ 9].ShDelta = 108;

    	DinoInfo[10].Name = "Velociraptor";
    	DinoInfo[10].Mass = 0.3f;
    	DinoInfo[10].Length = 4.0f;
    	DinoInfo[10].Radius = 256.f;
    	DinoInfo[10].Health0 = 3;
    	DinoInfo[10].BaseScore = 16;
    	DinoInfo[10].ScaleA = 400;
    	DinoInfo[10].SmellK = 1.0f; DinoInfo[ 9].HearK = 0.5f; DinoInfo[ 9].LookK = 0.4f;
    	DinoInfo[10].ShDelta =-24;
    	DinoInfo[10].DangerCall = true;

    	DinoInfo[11].Name = "T-Rex";
        DinoInfo[11].Mass = 6.f;
    	DinoInfo[11].Length = 12.f;
    	DinoInfo[11].Radius = 400.f;
    	DinoInfo[11].Health0 = 1024;
    	DinoInfo[11].BaseScore = 20;
    	DinoInfo[11].SmellK = 0.85f; DinoInfo[10].HearK = 0.8f; DinoInfo[10].LookK = 0.8f;
    	DinoInfo[11].ShDelta = 168;
    	DinoInfo[11].DangerCall = true;

    	DinoInfo[ 4].Name = "Brahiosaurus";
        DinoInfo[ 4].Mass = 9.f;
    	DinoInfo[ 4].Length = 12.f;
    	DinoInfo[ 4].Radius = 400.f;
    	DinoInfo[ 4].Health0 = 1024;
    	DinoInfo[ 4].BaseScore = 0;
    	DinoInfo[ 4].SmellK = 0.85f; DinoInfo[16].HearK = 0.8f; DinoInfo[16].LookK = 0.8f;
    	DinoInfo[ 4].ShDelta = 168;
    	DinoInfo[ 4].DangerCall = false;
    */
    LoadResourcesScript();
}




void InitEngine()
{
    //DEBUG			= true;
    g->CHEATED			= false;
    g->GREMEDY			= false;
    bDevShow		= false;

    g->WATERANI		= true;
    g->NODARKBACK		= true;
    LoDetailSky		= true;
    g->CORRECTION		= true;
    g->FOGON			= true;
    g->FOGENABLE		= true;
    g->PARTICLES		= true;
    g->TRANSPARENCY	= true;
    g->ALPHAKEY		= true;

    g->Clouds			= true;
    g->SKY				= true;
    g->GOURAUD			= true;
    g->MODELS			= true;
    g->TIMER			= g->DEBUG;
    g->BITMAPP			= false;
    g->MIPMAP			= true;
    g->NOCLIP			= false;
    g->CLIP3D			= true;


    g->SLOW			= false;
    g->LOWRESTX		= false;
    g->MORPHP			= true;
    g->MORPHA			= true;

    _GameState		= 0;

    RadarMode		= false;
    NightVision		= true;
    SurviveMode		= false;
    Windowed		= false;
    CustomRes		= false;

#if defined( AF_PLATFORM_WINDOWS )
    fnt_BIG = CreateFont(
                  23, 10, 0, 0,
                  600, 0,0,0,
#ifdef __rus
                  RUSSIAN_CHARSET,
#else
                  ANSI_CHARSET,
#endif
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);




    fnt_Small = CreateFont(
                    14, 5, 0, 0,
                    100, 0,0,0,
#ifdef __rus
                    RUSSIAN_CHARSET,
#else
                    ANSI_CHARSET,
#endif
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);


    fnt_Midd  = CreateFont(
                    16, 7, 0, 0,
                    550, 0,0,0,
#ifdef __rus
                    RUSSIAN_CHARSET,
#else
                    ANSI_CHARSET,
#endif
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, NULL);
#endif

    Textures[255] = new CTexture;

    WaterR = 10;
    WaterG = 38;
    WaterB = 46;
    WaterA = 10;
    TargetDino = 1<<10;
    TargetCall = 10;
    WeaponPres = 1;
    memset( MessageList, 0, sizeof(TMessageList)*32 );
    //MessageList[0].timeleft = 0;

    InitGameInfo();

    CreateVideoDIB();
    CreateFadeTab();
    CreateDivTable();
    InitClips();

    TrophyRoom.RegNumber=0;

    PlayerX = (ctMapSize / 3) * 256;
    PlayerZ = (ctMapSize / 3) * 256;

    ProcessCommandLine();


    switch (OptDayNight)
    {
    case 0:
        SunShadowK = 0.7f;
        Sun3dPos.x = - 4048;
        Sun3dPos.y = + 2048;
        Sun3dPos.z = - 4048;
        break;
    case 1:
        SunShadowK = 0.5f;
        Sun3dPos.x = - 2048;
        Sun3dPos.y = + 4048;
        Sun3dPos.z = - 2048;
        break;
    case 2:
        SunShadowK = -0.7f;
        Sun3dPos.x = + 3048;
        Sun3dPos.y = + 3048;
        Sun3dPos.z = + 3048;
        break;
    }

    LoadTrophy();

    ProcessCommandLine();




    //ctViewR  = 72;
    //ctViewR1 = 28;
    //ctViewRM = 24;
    ctViewR  = 42 + (int)(OptViewR / 8)*2;
    ctViewR1 = 28;
    ctViewRM = 24;

    Soft_Persp_K = 1.5f;
    HeadY = 220;

    FogsList[0].fogRGB = 0x000000;
    FogsList[0].YBegin = 0;
    FogsList[0].Transp = 000;
    FogsList[0].FLimit = 000;

    FogsList[127].fogRGB = 0x00504000;
    FogsList[127].Mortal = false;
    FogsList[127].Transp = 460;
    FogsList[127].FLimit = 200;

    memset( FogsMap, 0, sizeof(FogsMap) );
    PrintLog("Init Engine: [Ok]\n");
}





void ShutDownEngine()
{
    // < terminate GLFW here? >
}



void ProcessSyncro()
{
    RealTime = glfwGetTime() * 1000.0;

    srand( (unsigned) RealTime );
    if (g->SLOW) RealTime/=4;
    TimeDt = RealTime - PrevTime;
    if (TimeDt<0) TimeDt = 10;
    if (TimeDt>10000) TimeDt = 10;
    if (TimeDt>1000) TimeDt = 1000;
    PrevTime = RealTime;
    Takt++;
    if (!g->PAUSE)
        if (MyHealth) MyHealth+=TimeDt*4;
    if (MyHealth>MAX_HEALTH) MyHealth = MAX_HEALTH;
}






void AddBloodTrail(TCharacter *cptr)
{
    if (BloodTrail.Count>508)
    {
        memcpy(&BloodTrail.Trail[0], &BloodTrail.Trail[1], 510*sizeof(TBloodP));
        BloodTrail.Count--;
    }
    BloodTrail.Trail[BloodTrail.Count].LTime = 210000;
    BloodTrail.Trail[BloodTrail.Count].pos = cptr->pos;
    BloodTrail.Trail[BloodTrail.Count].pos.x+=siRand(32);
    BloodTrail.Trail[BloodTrail.Count].pos.z+=siRand(32);
    BloodTrail.Trail[BloodTrail.Count].pos.y =
        GetLandH(BloodTrail.Trail[BloodTrail.Count].pos.x,
                 BloodTrail.Trail[BloodTrail.Count].pos.z)+4;
    BloodTrail.Count++;
}





void MakeCall()
{
    if (!TargetDino) return;
    if (g->UNDERWATER) return;
    if (ObservMode || TrophyMode) return;
    if (CallLockTime) return;

    CallLockTime=1024*3;

    NextCall+=(RealTime % 2)+1;
    NextCall%=3;

    g_AudioDevice->addVoice( fxCall[TargetCall-10][NextCall] );

    float dmin = 512*256;
    int ai = -1;

    for (int c=0; c<ChCount; c++)
    {
        TCharacter *cptr = &Characters[c];

        if (DinoInfo[AI_to_CIndex[TargetCall] ].DangerCall)
            if (cptr->AI<10)
            {
                cptr->State=2;
                cptr->AfraidTime = (10 + rRand(5)) * 1024;
            }

        if (cptr->AI!=TargetCall) continue;
        if (cptr->AfraidTime) continue;
        if (cptr->State) continue;

        float d = VectorLength(SubVectors(PlayerPos, cptr->pos));
        if (d < ctViewR * 400)
        {
            if (rRand(128) > 32)
                if (d<dmin)
                {
                    dmin = d;
                    ai = c;
                }
            cptr->tgx = PlayerX + siRand(1800);
            cptr->tgz = PlayerZ + siRand(1800);
        }
    }

    if (ai!=-1)
    {
        answpos = SubVectors(Characters[ai].pos, PlayerPos);
        answpos.x/=-3.f;
        answpos.y/=-3.f;
        answpos.z/=-3.f;
        answpos = SubVectors(PlayerPos, answpos);
        answtime = 2000 + rRand(2000);
        answcall = TargetCall;
    }

}



uint32_t ColorSum(uint32_t C1, uint32_t C2)
{
    uint32_t R,G,B;
    R = std::min(255U, ((C1>> 0) & 0xFF) + ((C2>> 0) & 0xFF));
    G = std::min(255U, ((C1>> 8) & 0xFF) + ((C2>> 8) & 0xFF));
    B = std::min(255U, ((C1>>16) & 0xFF) + ((C2>>16) & 0xFF));
    return R + (G<<8) + (B<<16);
}


/*#define partBlood   1
#define partWater   2
#define partGround  3
#define partBubble  4*/

void AddElements(float x, float y, float z, int etype, int cnt)
{
    if (ElCount > 30)
    {
        memcpy(&Elements[0], &Elements[1], (ElCount-1) * sizeof(TElements));
        ElCount--;
    }

    Elements[ElCount].EDone  = 0;
    Elements[ElCount].Type = etype;
    Elements[ElCount].ECount = std::min(30, cnt);
    int c;

    switch (etype)
    {
    case partBlood:
#ifdef _d3d
        Elements[ElCount].RGBA = 0xE0600000;
        Elements[ElCount].RGBA2= 0x20300000;
#else
        Elements[ElCount].RGBA = 0xE0000060;
        Elements[ElCount].RGBA2= 0x20000030;
#endif
        break;

    case partGround:
#ifdef _d3d
        Elements[ElCount].RGBA = 0xF0F09E55;
        Elements[ElCount].RGBA2= 0x10F09E55;
#else
        Elements[ElCount].RGBA = 0xF0559EF0;
        Elements[ElCount].RGBA2= 0x10559EF0;
#endif
        break;


    case partBubble:
        c = WaterList[ WMap[ (int)z / 256][ (int)x / 256] ].fogRGB;
#ifdef _d3d
        c = ColorSum( ((c & 0xFEFEFE)>>1) , 0x152020);
#else
        c = ColorSum( ((c & 0xFEFEFE)>>1) , 0x202015);
#endif
        Elements[ElCount].RGBA = 0x70000000 + (ColorSum(c, ColorSum(c,c)));
        Elements[ElCount].RGBA2= 0x40000000 + (ColorSum(c, c));
        break;

    case partWater:
        c = WaterList[ WMap[ (int)z / 256][ (int)x / 256] ].fogRGB;
#ifdef _d3d
        c = ColorSum( ((c & 0xFEFEFE)>>1) , 0x152020);
#else
        c = ColorSum( ((c & 0xFEFEFE)>>1) , 0x202015);
#endif
        Elements[ElCount].RGBA  = 0xB0000000 + ( ColorSum(c, ColorSum(c,c)) );
        Elements[ElCount].RGBA2 = 0x40000000 + (c);
        break;
    }

    Elements[ElCount].RGBA  = conv_xGx(Elements[ElCount].RGBA);
    Elements[ElCount].RGBA2 = conv_xGx(Elements[ElCount].RGBA2);

    float al = siRand(128) / 128.f * pi / 4.f;
    float ss = sinf(al);
    float cc = cosf(al);

    for (int e=0; e<Elements[ElCount].ECount; e++)
    {
        Elements[ElCount].EList[e].pos.x = (float)(x);
        Elements[ElCount].EList[e].pos.y = (float)(y);
        Elements[ElCount].EList[e].pos.z = (float)(z);
        Elements[ElCount].EList[e].R = (float)(6 + rRand(5));
        Elements[ElCount].EList[e].Flags = 0;
        float v;

        switch (etype)
        {
        case partBlood:
            v = (float)(e * 6 + rRand(96) + 220);
            Elements[ElCount].EList[e].speed.x = (float)(ss*ca*v + siRand(32));
            Elements[ElCount].EList[e].speed.y = (float)(cc * (v * 3));
            Elements[ElCount].EList[e].speed.z = (float)(ss*sa*v + siRand(32));
            break;
        case partGround:
            Elements[ElCount].EList[e].speed.x = (float)(siRand(52)-sa*64);
            Elements[ElCount].EList[e].speed.y = (float)(rRand(100) + 600 + e * 20);
            Elements[ElCount].EList[e].speed.z = (float)(siRand(52)+ca*64);
            break;
        case partWater:
            Elements[ElCount].EList[e].speed.x = (float)(siRand(32));
            Elements[ElCount].EList[e].speed.y = (float)(rRand(80) + 400 +  e * 40);
            Elements[ElCount].EList[e].speed.z = (float)(siRand(32));
            break;
        case partBubble:
            Elements[ElCount].EList[e].speed.x = (float)(siRand(40));
            Elements[ElCount].EList[e].speed.y = (float)(rRand(140) + 20);
            Elements[ElCount].EList[e].speed.z = (float)(siRand(40));
            break;
        }
    }

    ElCount++;
}


void MakeShot(float ax, float ay, float az,
              float bx, float by, float bz)
{
    int sres;
    if (!WeapInfo[CurrentWeapon].Fall)
    {
        sres = TraceShot(ax, ay, az, bx, by, bz);
    }
    else
    {
        vec3 dl;
        float dy = 40.f * ctViewR / 36.f;
        dl.x = (bx-ax) / 3;
        dl.y = (by-ay) / 3;
        dl.z = (bz-az) / 3;
        bx = ax + dl.x;
        by = ay + dl.y - dy / 2;
        bz = az + dl.z;
        sres = TraceShot(ax, ay, az, bx, by, bz);
        if (sres!=-1) goto ENDTRACE;
        ax = bx;
        ay = by;
        az = bz;

        bx = ax + dl.x;
        by = ay + dl.y - dy * 3;
        bz = az + dl.z;
        sres = TraceShot(ax, ay, az, bx, by, bz);
        if (sres!=-1) goto ENDTRACE;
        ax = bx;
        ay = by;
        az = bz;

        bx = ax + dl.x;
        by = ay + dl.y - dy * 5;
        bz = az + dl.z;
        sres = TraceShot(ax, ay, az, bx, by, bz);
        if (sres!=-1) goto ENDTRACE;
        ax = bx;
        ay = by;
        az = bz;
    }

ENDTRACE:
    if (sres==-1) return;

    int mort = (sres & 0xFF00) && (Characters[ShotDino].Health);
    sres &= 0xFF;

    if (sres == tresGround)
        AddElements(bx, by, bz, partGround, (int)(6 + WeapInfo[CurrentWeapon].Power*4));
    if (sres == tresModel)
        AddElements(bx, by, bz, partGround, (int)(6 + WeapInfo[CurrentWeapon].Power*4));


    if (sres == tresWater)
    {
        AddElements(bx, by, bz, partWater, (int)(4 + WeapInfo[CurrentWeapon].Power*3));
        //AddElements(bx, GetLandH(bx, bz), bz, partBubble);
        AddWCircle(bx, bz, 1.2f);
        AddWCircle(bx, bz, 1.2f);
    }


    if (sres!=tresChar) return;
    AddElements(bx, by, bz, partBlood, (int)(4 + WeapInfo[CurrentWeapon].Power*4));
    if (!Characters[ShotDino].Health) return;

//======= character damage =========//


    if (mort) Characters[ShotDino].Health = 0;
    else Characters[ShotDino].Health-=(int)WeapInfo[CurrentWeapon].Power;
    if (Characters[ShotDino].Health<0) Characters[ShotDino].Health=0;


    if (!Characters[ShotDino].Health)
    {
        if (Characters[ShotDino].AI>=10)
        {
            TrophyRoom.Last.success++;
            AddShipTask(ShotDino);
        }

        if (Characters[ShotDino].AI<10)
            Characters_AddSecondaryOne(Characters[ShotDino].CType);

    }
    else
    {
        Characters[ShotDino].AfraidTime = 60*1000;
        if (Characters[ShotDino].State==0)
            Characters[ShotDino].State = 2;

        Characters[ShotDino].BloodTTime+=90000;

    }

}


void RemoveCharacter(int index)
{
    if (index==-1) return;
    memcpy( &Characters[index], &Characters[index+1], (255 - index) * sizeof(TCharacter) );
    ChCount--;

    if (DemoPoint.CIndex > index) DemoPoint.CIndex--;

    for (int c=0; c<ShipTask.tcount; c++)
        if (ShipTask.clist[c]>index) ShipTask.clist[c]--;
}


void AnimateShip()
{
    if (Ship.State==-1)
    {
        if (!ShipTask.tcount) return;
        InitShip(ShipTask.clist[0]);
        memcpy(&ShipTask.clist[0], &ShipTask.clist[1], 250*4);
        ShipTask.tcount--;
        return;
    }

	//SetAmbient3d(ShipModel.SoundFX[0].length, Ship.pos.x, Ship.pos.y, Ship.pos.z);

    int _TimeDt = TimeDt;

//====== get up/down time acceleration ===========//
    if (Ship.FTime)
    {
        int am = ShipModel.Animation[0].AniTime;
        if (Ship.FTime < 500) _TimeDt = TimeDt * (Ship.FTime + 48) / 548;
        if (am-Ship.FTime < 500) _TimeDt = TimeDt * (am-Ship.FTime + 48) / 548;
        if (_TimeDt<2) _TimeDt=2;
    }
//===================================

    float L  = VectorLength( SubVectors(Ship.tgpos, Ship.pos) );
    float L2 = sqrtf( (Ship.tgpos.x - Ship.pos.x) * (Ship.tgpos.x - Ship.pos.x) +
                      (Ship.tgpos.x - Ship.pos.x) * (Ship.tgpos.x - Ship.pos.x) );

    Ship.pos.y+=0.3f*(float)cos(RealTime / 256.f);



    Ship.tgalpha    = FindVectorAlpha(Ship.tgpos.x - Ship.pos.x, Ship.tgpos.z - Ship.pos.z);
    float currspeed;
    float dalpha = (float)fabs(Ship.tgalpha - Ship.alpha);
    float drspd = dalpha;
    if (drspd>pi) drspd = 2*pi - drspd;


//====== fly more away if I near =============//
    if (Ship.State)
        if (Ship.speed>1)
            if (L<4000)
                if (VectorLength(SubVectors(PlayerPos, Ship.pos))<(ctViewR+2)*256)
                {
                    Ship.tgpos.x += (float)cos(Ship.alpha) * 256*6.f;
                    Ship.tgpos.z += (float)sin(Ship.alpha) * 256*6.f;
                    Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + Ship.DeltaY;
                    Ship.tgpos.y = std::max(Ship.tgpos.y, GetLandUpH(Ship.pos.x, Ship.pos.z) + Ship.DeltaY);
                }
//==============================//



//========= animate down ==========//
    if (Ship.State==3)
    {
        Ship.FTime+=_TimeDt;
        if (Ship.FTime>=ShipModel.Animation[0].AniTime)
        {
            Ship.FTime=ShipModel.Animation[0].AniTime-1;
            Ship.State=2;
            g_AudioDevice->addVoice( ShipModel.SoundFX[4] );
            g_AudioDevice->addVoice( ShipModel.SoundFX[1], Ship.pos.x, Ship.pos.y, Ship.pos.z );
        }
        return;
    }


//========= get body on board ==========//
    if (Ship.State)
    {
        if (Ship.cindex!=-1)
        {
            DeltaFunc(Characters[Ship.cindex].pos.y, Ship.pos.y-650 - (Ship.DeltaY-2048), _TimeDt / 3.f);
            DeltaFunc(Characters[Ship.cindex].beta,  0, TimeDt / 4048.f);
            DeltaFunc(Characters[Ship.cindex].gamma, 0, TimeDt / 4048.f);
        }

        if (Ship.State==2)
        {
            Ship.FTime-=_TimeDt;
            if (Ship.FTime<0) Ship.FTime=0;

            if (Ship.FTime==0)
                if (fabs(Characters[Ship.cindex].pos.y - (Ship.pos.y-650 - (Ship.DeltaY-2048))) < 1.f)
                {
                    Ship.State = 1;
                    g_AudioDevice->addVoice( ShipModel.SoundFX[5] );
                    //g_AudioDevice->addVoice( ShipModel.SoundFX[2], Ship.pos.x, Ship.pos.y, Ship.pos.z );
                }
            return;
        }
    }
//=====================================//


//====== speed ===============//
    float vspeed = 1.f + L / 128.f;
    if (vspeed > 40) vspeed = 40;
    if (Ship.State) vspeed = 40;
    if (fabs(dalpha) > 0.4) vspeed = 0.f;
    float _s = Ship.speed;
    if (vspeed>Ship.speed) DeltaFunc(Ship.speed, vspeed, TimeDt / 200.f);
    else Ship.speed = vspeed;

    if (Ship.speed>0 && _s==0)
    {
        g_AudioDevice->addVoice( ShipModel.SoundFX[2], Ship.pos.x, Ship.pos.y, Ship.pos.z );
	}

//====== fly ===========//
    float l = TimeDt * Ship.speed / 16.f;

    if (fabs(dalpha) < 0.4)
    {
        if (l<L)
        {
            if (l>L2) l = L2 * 0.5f;
            if (L2<0.1) l = 0;
            Ship.pos.x += (float)cos(Ship.alpha)*l;
            Ship.pos.z += (float)sin(Ship.alpha)*l;
        }
        else
        {
            if (Ship.State)
            {
                Ship.State = -1;
                RemoveCharacter(Ship.cindex);
                SetAmbient3d(0,0, 0,0,0);
                return;
            }
            else
            {
                Ship.pos = Ship.tgpos;
                Ship.State = 3;
                Ship.FTime = 1;
                Ship.tgpos = Ship.retpos;
                Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + Ship.DeltaY;
                Ship.tgpos.y = std::max(Ship.tgpos.y, GetLandUpH(Ship.pos.x, Ship.pos.z) + Ship.DeltaY);
                Characters[Ship.cindex].StateF = 0xFF;
                g_AudioDevice->addVoice( ShipModel.SoundFX[1], Ship.pos.x, Ship.pos.y, Ship.pos.z );
            }
        }
    }

//======= y movement ============//
    float h = GetLandUpH(Ship.pos.x, Ship.pos.z);
    DeltaFunc(Ship.pos.y, Ship.tgpos.y, TimeDt / 4.f);
    if (Ship.pos.y < h + 1024)
    {
        if (Ship.State)
            if (Ship.cindex!=-1)
                Characters[Ship.cindex].pos.y+= h + 1024 - Ship.pos.y;
        Ship.pos.y = h + 1024;
    }



//======= rotation ============//

    if (Ship.tgalpha > Ship.alpha) currspeed = 0.1f + (float)fabs(drspd)/2.f;
    else currspeed =-0.1f - (float)fabs(drspd)/2.f;

    if (fabs(dalpha) > pi) currspeed=-currspeed;


    DeltaFunc(Ship.rspeed, currspeed, (float)TimeDt / 420.f);

    float rspd=Ship.rspeed * TimeDt / 1024.f;
    if (fabs(drspd) < fabs(rspd))
    {
        Ship.alpha = Ship.tgalpha;
        Ship.rspeed/=2;
    }
    else
    {
        Ship.alpha+=rspd;
        if (Ship.State)
            if (Ship.cindex!=-1)
                Characters[Ship.cindex].alpha+=rspd;
    }

    if (Ship.alpha<0) Ship.alpha+=pi*2;
    if (Ship.alpha>pi*2) Ship.alpha-=pi*2;

//======== move body ===========//
    if (Ship.State)
    {
        if (Ship.cindex!=-1)
        {
            Characters[Ship.cindex].pos.x = Ship.pos.x;
            Characters[Ship.cindex].pos.z = Ship.pos.z;
        }
        if (L>1000) Ship.tgpos.y+=TimeDt / 12.f;
    }
    else
    {
        Ship.tgpos.x = Characters[Ship.cindex].pos.x;
        Ship.tgpos.z = Characters[Ship.cindex].pos.z;
        Ship.tgpos.y = GetLandUpH(Ship.tgpos.x, Ship.tgpos.z) + Ship.DeltaY;
        Ship.tgpos.y = std::max(Ship.tgpos.y, GetLandUpH(Ship.pos.x, Ship.pos.z) + Ship.DeltaY);
    }



}





void AnimateSupplyShip()
{
    if (!SupplyShip.State) return;

    if (Ship.State==-1)
    {
		//SetAmbient3d( SShipModel.SoundFX[0], SupplyShip.pos.x, SupplyShip.pos.y, SupplyShip.pos.z );
	}

    //int _TimeDt = TimeDt;

//====== get up/down time acceleration ===========//
REENTER:
    float L  = VectorLength( SubVectors(SupplyShip.tgpos, SupplyShip.pos) );
    float L2 = sqrtf( (SupplyShip.tgpos.x - SupplyShip.pos.x) * (SupplyShip.tgpos.x - SupplyShip.pos.x) +
                      (SupplyShip.tgpos.x - SupplyShip.pos.x) * (SupplyShip.tgpos.x - SupplyShip.pos.x) );

    if (L2<1200)
    {
        SupplyShip.tgpos = SupplyShip.retpos;
        if (SupplyShip.State==2)
        {
            SupplyShip.State = 0;
            SetAmbient3d(0,0, 0,0,0);
            return;
        }
        AmmoBag.State = 1;
        AmmoBag.pos = SupplyShip.pos;
        AmmoBag.speed.y = 0;
        AmmoBag.speed.x = cosf(SupplyShip.alpha)*4;
        AmmoBag.speed.z = sinf(SupplyShip.alpha)*4;
        AmmoBag.FTime = 0;
//  AddMessage("Bag");
        SupplyShip.State = 2;
        goto REENTER;
    }

    SupplyShip.pos.y+=0.3f*(float)cos(RealTime / 256.f);

    SupplyShip.tgalpha = FindVectorAlpha(SupplyShip.tgpos.x - SupplyShip.pos.x, SupplyShip.tgpos.z - SupplyShip.pos.z);
    float currspeed;
    float dalpha = (float)fabs(SupplyShip.tgalpha - SupplyShip.alpha);
    float drspd = dalpha;
    if (drspd>pi) drspd = 2*pi - drspd;


//====== fly more away if I near =============//
    if (Ship.State==2)
        if (Ship.speed>1)
            if (L<4000)
                if (VectorLength(SubVectors(PlayerPos, SupplyShip.pos))<(ctViewR+2)*256)
                {
                    SupplyShip.tgpos.x += (float)cos(SupplyShip.alpha) * 256*6.f;
                    SupplyShip.tgpos.z += (float)sin(SupplyShip.alpha) * 256*6.f;
                    SupplyShip.tgpos.y = GetLandUpH(SupplyShip.tgpos.x, SupplyShip.tgpos.z) + 2048;
                    SupplyShip.tgpos.y = std::max(SupplyShip.tgpos.y, GetLandUpH(Ship.pos.x, Ship.pos.z) + 2048);
                }
//==============================//



//====== speed ===============//
    float vspeed = 48;
    if (fabs(dalpha) > 0.4) vspeed = 24;
    DeltaFunc(SupplyShip.speed, vspeed, TimeDt / 200.f);

//====== fly ===========//
    float l = TimeDt * SupplyShip.speed / 16.f;

    SupplyShip.pos.x += (float)cos(SupplyShip.alpha)*l;
    SupplyShip.pos.z += (float)sin(SupplyShip.alpha)*l;

//======= y movement ============//
    float h = GetLandUpH(SupplyShip.pos.x, SupplyShip.pos.z);
    DeltaFunc(SupplyShip.pos.y, SupplyShip.tgpos.y, TimeDt / 4.f);
    if (SupplyShip.pos.y < h + 2048)
        DeltaFunc(SupplyShip.pos.y, h + 2048, (float)TimeDt);

    if (SupplyShip.State==2) SupplyShip.tgpos.y+=TimeDt / 4.f;

//======= rotation ============//

    if (SupplyShip.tgalpha > SupplyShip.alpha) currspeed = 0.1f + (float)fabs(drspd)/3.f;
    else currspeed =-0.1f - (float)fabs(drspd)/3.f;
    if (fabs(dalpha) > pi) currspeed=-currspeed;
    DeltaFunc(SupplyShip.rspeed, currspeed, (float)TimeDt / 420.f);

    float rspd=SupplyShip.rspeed * TimeDt / 1024.f;
    if (fabs(drspd) < fabs(rspd))
    {
        SupplyShip.alpha = SupplyShip.tgalpha;
        SupplyShip.rspeed/=2;
    }
    else SupplyShip.alpha+=rspd;

    if (SupplyShip.alpha<0)    SupplyShip.alpha+=pi*2;
    if (SupplyShip.alpha>pi*2) SupplyShip.alpha-=pi*2;
}



void AnimateAmmoBag()
{
    if (!AmmoBag.State) return;

    float l = VectorLength(SubVectors(PlayerPos, AmmoBag.pos));
    if (l<256)
    {
        g_AudioDevice->addVoice( AmmoModel.SoundFX[1], AmmoBag.pos.x, AmmoBag.pos.y, AmmoBag.pos.z );
        AmmoBag.State=0;
        for (int w=0; w<TotalW; w++)
            if ( WeaponPres & (1<<w) )
            {
                ShotsLeft[w] = WeapInfo[w].Shots;
                if (DoubleAmmo) AmmoMag[w] = 1;
            }
    }

    if (AmmoBag.State==1)
    {
        AmmoBag.speed.y-=DeltaT*16.f;
        AmmoBag.pos.x+=AmmoBag.speed.x*DeltaT*128;
        AmmoBag.pos.y+=AmmoBag.speed.y*DeltaT*128;
        AmmoBag.pos.z+=AmmoBag.speed.z*DeltaT*128;
    }

    float h = GetLandH(AmmoBag.pos.x, AmmoBag.pos.z);
    if (AmmoBag.pos.y < h)
    {
        AmmoBag.pos.y = h;
        AmmoBag.State = 2;
        AmmoBag.FTime = 1;
        g_AudioDevice->addVoice( AmmoModel.SoundFX[0], AmmoBag.pos.x, AmmoBag.pos.y, AmmoBag.pos.z );

    }

    if (AmmoBag.FTime)
    {
        AmmoBag.FTime+=TimeDt;
        if (AmmoBag.FTime>= AmmoModel.Animation[0].AniTime)
            AmmoBag.FTime = AmmoModel.Animation[0].AniTime-1;
    }
}


void ProcessTrophy()
{
    TrophyBody = -1;

    for (int c=0; c<ChCount; c++)
    {
        vec3 p = Characters[c].pos;
        p.x+=Characters[c].lookx * 256*2.5f;
        p.z+=Characters[c].lookz * 256*2.5f;

        if (VectorLength( SubVectors(p, PlayerPos) ) < 148)
            TrophyBody = c;
    }

    if (TrophyBody==-1) return;

    TrophyBody = Characters[TrophyBody].State;
}



void RespawnSnow(int s, bool rand)
{
    Snow[s].pos.x = PlayerX + siRand(12*256);
    Snow[s].pos.z = PlayerZ + siRand(12*256);
    Snow[s].hl = GetLandUpH(Snow[s].pos.x, Snow[s].pos.z);
    Snow[s].ftime = 0;
    if (rand) Snow[s].pos.y = Snow[s].hl + 256+rRand(12*256);
    else Snow[s].pos.y = Snow[s].hl + (8+rRand(5))*256;
}


void RespawnRain(int r, bool rand)
{
    Rain[r].pos.x = PlayerX + siRand(12*256);
    Rain[r].pos.z = PlayerZ + siRand(12*256);
    Rain[r].hl = GetLandUpH(Snow[r].pos.x, Snow[r].pos.z);
    Rain[r].ftime = 0;
    if (rand)
    {
        Rain[r].pos.y = Rain[r].hl + 256+rRand(12*256);
    }
    else
    {
        Rain[r].pos.y = Rain[r].hl + (8+rRand(5))*256;
    }
}




void AnimateElements()
{
    for (int eg=0; eg<ElCount; eg++)
    {
        if  (Elements[eg].Type == partGround)
        {
            int a1 = Elements[eg].RGBA >> 24;
            a1-=TimeDt/4;
            if (a1<0) a1=0;
            Elements[eg].RGBA = (Elements[eg].RGBA  & 0x00FFFFFF) + (a1<<24);
            int a2 = Elements[eg].RGBA2>> 24;
            a2-=TimeDt/4;
            if (a2<0) a2=0;
            Elements[eg].RGBA2= (Elements[eg].RGBA2 & 0x00FFFFFF) + (a2<<24);
            if (a1 == 0 && a2==0) Elements[eg].ECount = 0;
        }

        if  (Elements[eg].Type == partWater)
            if (Elements[eg].EDone == Elements[eg].ECount)
                Elements[eg].ECount = 0;

        if  (Elements[eg].Type == partBubble)
            if (Elements[eg].EDone == Elements[eg].ECount)
                Elements[eg].ECount = 0;

        if  (Elements[eg].Type == partBlood)
            if ((Takt & 3)==0)
                if (Elements[eg].EDone == Elements[eg].ECount)
                {
                    int a1 = Elements[eg].RGBA >> 24;
                    a1--;
                    if (a1<0) a1=0;
                    Elements[eg].RGBA = (Elements[eg].RGBA  & 0x00FFFFFF) + (a1<<24);
                    int a2 = Elements[eg].RGBA2>> 24;
                    a2--;
                    if (a2<0) a2=0;
                    Elements[eg].RGBA2= (Elements[eg].RGBA2 & 0x00FFFFFF) + (a2<<24);
                    if (a1 == 0 && a2==0) Elements[eg].ECount = 0;
                }



//====== remove finished process =========//
        if (!Elements[eg].ECount)
        {
            memcpy(&Elements[eg], &Elements[eg+1], (ElCount+1-eg) * sizeof(TElements));
            ElCount--;
            eg--;
            continue;
        }


        for (int e=0; e<Elements[eg].ECount; e++)
        {
            if (Elements[eg].EList[e].Flags) continue;
            Elements[eg].EList[e].pos.x+=Elements[eg].EList[e].speed.x * TimeDt / 1000.f;
            Elements[eg].EList[e].pos.y+=Elements[eg].EList[e].speed.y * TimeDt / 1000.f;
            Elements[eg].EList[e].pos.z+=Elements[eg].EList[e].speed.z * TimeDt / 1000.f;

            float h;
            h = GetLandUpH(Elements[eg].EList[e].pos.x, Elements[eg].EList[e].pos.z);
            bool OnWater = GetLandH(Elements[eg].EList[e].pos.x, Elements[eg].EList[e].pos.z) < h;

            switch (Elements[eg].Type)
            {
            case partBubble:
                Elements[eg].EList[e].speed.y += 2.0f * 256 * TimeDt / 1000.f;
                if (Elements[eg].EList[e].speed.y > 824) Elements[eg].EList[e].speed.y = 824;
                if (Elements[eg].EList[e].pos.y > h)
                {
                    AddWCircle(Elements[eg].EList[e].pos.x, Elements[eg].EList[e].pos.z, 0.6f);
                    Elements[eg].EDone++;
                    Elements[eg].EList[e].Flags = 1;
                    if (OnWater) Elements[eg].EList[e].pos.y-= 10240;
                }
                break;

            default:
                Elements[eg].EList[e].speed.y -= 9.8f * 256.0f * TimeDt / 1000.f;
                if (Elements[eg].EList[e].pos.y < h)
                {
                    if (OnWater) AddWCircle(Elements[eg].EList[e].pos.x, Elements[eg].EList[e].pos.z, 0.6f);
                    Elements[eg].EDone++;
                    Elements[eg].EList[e].Flags = 1;
                    if (OnWater) Elements[eg].EList[e].pos.y-= 10240;
                    else Elements[eg].EList[e].pos.y = h + 4;
                }
                break;

            } //== switch ==//

        } // for(e) //
    } // for(eg) //

    for (int b=0; b<BloodTrail.Count; b++)
    {
        BloodTrail.Trail[b].LTime-=TimeDt;
        if (BloodTrail.Trail[b].LTime<=0)
        {
            memcpy(&BloodTrail.Trail[b], &BloodTrail.Trail[b+1], (511-b)*sizeof(TBloodP));
            BloodTrail.Count--;
            b--;
        }
    }

    if (g->SNOW) // Need to change to some weather variable
    {
        while (SnCount<2000)
        {
            RespawnSnow(SnCount, true);
            SnCount++;
        }

        nv = Wind.nv;
        NormVector(nv, (4+Wind.speed)*4*TimeDt/1000);

        for (int s=0; s<SnCount; s++)
        {

            if ( (fabs(Snow[s].pos.x - PlayerX) > 14*256) ||
                    (fabs(Snow[s].pos.z - PlayerZ) > 14*256) )
            {
                Snow[s].pos.x = PlayerX + siRand(12*256);
                Snow[s].pos.z = PlayerZ + siRand(12*256);
                Snow[s].pos.y = Snow[s].pos.y - Snow[s].hl;
                Snow[s].hl = GetLandUpH(Snow[s].pos.x, Snow[s].pos.z);
                Snow[s].pos.y+=Snow[s].hl;
            }

            if (!Snow[s].ftime)
            {
                float v = (float)(( ((RealTime + s * 23) % 800) - 400) * TimeDt) / 16000;
                Snow[s].pos.x+=ca*v;
                Snow[s].pos.z+=sa*v;

                Snow[s].pos = AddVectors(Snow[s].pos, nv);
                Snow[s].hl = GetLandUpH(Snow[s].pos.x, Snow[s].pos.z);
                Snow[s].pos.y-=TimeDt*192/1000.f;
                if (Snow[s].pos.y < Snow[s].hl+8)
                {
                    Snow[s].pos.y = Snow[s].hl+8;
                    Snow[s].ftime = 1;
                }
            }
            else
            {
                Snow[s].ftime+=TimeDt;
                Snow[s].pos.y-=TimeDt*3/1000.f;
                if (Snow[s].ftime>2000)  RespawnSnow(s, false);
            }

        }
    }

    if (g->RAIN) // same thing as snow
    {
        while (RainCount<4000)
        {
            RespawnRain(RainCount, true);
            RainCount++;
        }

        nv = Wind.nv;
        NormVector(nv, (4+Wind.speed)*4*TimeDt/1000);

        for (int r=0; r<RainCount; r++)
        {
            // If the particles are outside the immediate area of the player
            if ( (fabs(Rain[r].pos.x - PlayerX) > 14*256) ||
                    (fabs(Rain[r].pos.z - PlayerZ) > 14*256) )
            {
                Rain[r].pos.x = PlayerX + siRand(12*256);
                Rain[r].pos.z = PlayerZ + siRand(12*256);
                Rain[r].pos.y = Rain[r].pos.y - Rain[r].hl;
                Rain[r].hl = GetLandUpH(Rain[r].pos.x, Rain[r].pos.z);
                Rain[r].pos.y += Rain[r].hl;
            }

            if (!Rain[r].ftime)
            {
                float v = (float)(( ((RealTime + r * 23) % 800) - 400) * TimeDt) / 16000;

                Rain[r].pos.x+= ca * v;
                Rain[r].pos.z+= sa * v;

                Rain[r].pos = AddVectors(Rain[r].pos, nv);
                Rain[r].hl = GetLandUpH(Rain[r].pos.x, Rain[r].pos.z);
                Rain[r].pos.y -= TimeDt*1000/1000.f;//TimeDt*192/1000.f;

                if (Rain[r].pos.y < Rain[r].hl+8)
                {
                    if ( IsInWater( Rain[r].pos.x, Rain[r].pos.z ) )
                    {
                        AddWCircle(Rain[r].pos.x, Rain[r].pos.z, 0.6f);
                        Rain[r].ftime = 1;
                        RespawnRain(r, false);
                    }
                    else
                    {
                        Rain[r].pos.y = Rain[r].hl+8;
                        Rain[r].ftime = 1;
                    }
                }
            }
            else
            {
                Rain[r].ftime+=TimeDt;
                Rain[r].pos.y-=TimeDt*3/1000.f;
                if (Rain[r].ftime>2000)  RespawnRain(r, false);
            }
        }
    }
}


void AnimateProcesses()
{
    AnimateElements();

    if ((Takt & 63)==0)
    {
        float al2 = CameraAlpha + siRand(60) * pi / 180.f;
        float c2 = cosf(al2);
        float s2 = sinf(al2);
        float l = 1024.0f + rRand(3120);
        float xx = CameraX + s2 * l;
        float zz = CameraZ - c2 * l;
        if (GetLandUpH(xx,zz) > GetLandH(xx,zz)+256)
            AddElements(xx, GetLandH(xx,zz), zz, 4, 6 + rRand(6));
    }

    if (Takt & 1)
    {
        Wind.alpha+=siRand(16) / 4096.f;
        Wind.speed+=siRand(400) / 6400.f;
    }

    if (Wind.speed< 4.f) Wind.speed=4.f;
    if (Wind.speed>18.f) Wind.speed=18.f;

    Wind.nv.x = (float) sin(Wind.alpha);
    Wind.nv.z = (float)-cos(Wind.alpha);
    Wind.nv.y = 0.f;


    if (answtime)
    {
        answtime-=TimeDt;
        if (answtime<=0)
        {
            answtime = 0;
            int r = rRand(128) % 3;
            g_AudioDevice->addVoice( fxCall[answcall-10][r], answpos.x - CameraX, answpos.y - CameraY, answpos.z - CameraZ );
        }
    }



    if (CallLockTime)
    {
        CallLockTime-=TimeDt;
        if (CallLockTime<0) CallLockTime=0;
    }

    CheckAfraid();
    AnimateShip();
    AnimateSupplyShip();
    AnimateAmmoBag();
    if (TrophyMode)
        ProcessTrophy();

    for (int w=0; w<WCCount; w++)
    {
        if (WCircles[w].scale > 1)
            WCircles[w].FTime+=(int)(TimeDt*3 / WCircles[w].scale);
        else
            WCircles[w].FTime+=TimeDt*3;
        if (WCircles[w].FTime >= 2000)
        {
            memcpy(&WCircles[w], &WCircles[w+1], sizeof(TWCircle) * (WCCount+1-w) );
            w--;
            WCCount--;
        }
    }



    if (ExitTime)
    {
        ExitTime-=TimeDt;
        if (ExitTime<=0)
        {
            TrophyRoom.Total.time   +=TrophyRoom.Last.time;
            TrophyRoom.Total.smade  +=TrophyRoom.Last.smade;
            TrophyRoom.Total.success+=TrophyRoom.Last.success;
            TrophyRoom.Total.path   +=TrophyRoom.Last.path;

            if (MyHealth) SaveTrophy();
            else LoadTrophy();
            DoHalt( 0 );
        }
    }
}



void RemoveCurrentTrophy()
{
    int p = 0;
    if (!TrophyMode) return;
    if (!TrophyRoom.Body[TrophyBody].ctype) return;

    PrintLog("Trophy removed: ");
    PrintLog(DinoInfo[TrophyRoom.Body[TrophyBody].ctype].Name);
    PrintLog("\n");

    for (int c=0; c<TrophyBody; c++)
        if (TrophyRoom.Body[c].ctype) p++;


    TrophyRoom.Body[TrophyBody].ctype = 0;

    if (TrophyMode)
    {
        memcpy(&Characters[p],
               &Characters[p+1],
               (250-p) * sizeof(TCharacter) );
        ChCount--;
    }

    TrophyTime = 0;
    TrophyBody = -1;
}


void LoadTrophy()
{
    int pr = TrophyRoom.RegNumber;
    memset( &TrophyRoom, 0, sizeof(TrophyRoom) );
    TrophyRoom.RegNumber = pr;
    char fname[128];
    int rn = TrophyRoom.RegNumber;
    sprintf(fname, "TROPHY%d.SAV", TrophyRoom.RegNumber);

    FILE* fp = fopen(fname, "rb" );

    if ( fp == 0 )
    {
        PrintLog("===> Error loading trophy!\n");
        return;
    }

    fread( &TrophyRoom, sizeof(TrophyRoom), 1, fp );

    fread( &OptAgres, 4, 1, fp );
    fread( &OptDens , 4, 1, fp );
    fread( &OptSens , 4, 1, fp );

    fread( &OptRes, 4, 1, fp );
    fread( &g->FOGENABLE, 4, 1, fp );
    fread( &OptText , 4, 1, fp );
    fread( &OptViewR, 4, 1, fp );
    fread( &g->SHADOWS3D, 4, 1, fp );
    fread( &OptMsSens, 4, 1, fp );
    fread( &OptBrightness, 4, 1, fp );


    fread( &KeyMap, sizeof(KeyMap), 1, fp );
    fread( &g->REVERSEMS, 4, 1, fp );

    fread( &ScentMode, 4, 1, fp );
    fread( &CamoMode, 4, 1, fp );
    fread( &RadarMode, 4, 1, fp );
    fread( &Tranq    , 4, 1, fp );
    fread( &OPT_ALPHA_COLORKEY, 4, 1, fp );

    fread( &OptSys  , 4, 1, fp );
    fread( &OptSound , 4, 1, fp );
    fread( &OptRender, 4, 1, fp );


    SetupRes();

    fclose( fp );
    TrophyRoom.RegNumber = rn;
    PrintLog("Trophy Loaded.\n");
//	TrophyRoom.Score = 299;
}




void SaveTrophy()
{
    char fname[128];
    sprintf(fname, "TROPHY%d.SAV", TrophyRoom.RegNumber);

    int r = TrophyRoom.Rank;
    TrophyRoom.Rank = RANK_NOVICE;
    if (TrophyRoom.Score >= 100) TrophyRoom.Rank = RANK_ADVANCED;
    if (TrophyRoom.Score >= 300) TrophyRoom.Rank = RANK_EXPERT;
    if (TrophyRoom.Score >= 300) TrophyRoom.Rank = RANK_MASTER;


    FILE* fp = fopen( fname, "wb" );

    if ( fp == 0 )
    {
        PrintLog("==>> Error saving trophy!\n");
        return;
    }

    fwrite( &TrophyRoom, sizeof(TrophyRoom), 1, fp );

    fwrite( &OptAgres, 4, 1, fp );
    fwrite( &OptDens , 4, 1, fp );
    fwrite( &OptSens , 4, 1, fp );

    fwrite( &OptRes, 4, 1, fp );
    fwrite( &g->FOGENABLE, 4, 1, fp );
    fwrite( &OptText , 4, 1, fp );
    fwrite( &OptViewR, 4, 1, fp );
    fwrite( &g->SHADOWS3D, 4, 1, fp );
    fwrite( &OptMsSens, 4, 1, fp );
    fwrite( &OptBrightness, 4, 1, fp );

    fwrite( &KeyMap, sizeof(KeyMap), 1, fp );
    fwrite( &g->REVERSEMS, 4, 1, fp );

    fwrite( &ScentMode, 4, 1, fp );
    fwrite( &CamoMode , 4, 1, fp );
    fwrite( &RadarMode, 4, 1, fp );
    fwrite( &Tranq    , 4, 1, fp );
    fwrite( &OPT_ALPHA_COLORKEY, 4, 1, fp );

    fwrite( &OptSys   , 4, 1, fp );
    fwrite( &OptSound , 4, 1, fp );
    fwrite( &OptRender, 4, 1, fp );

    fclose( fp );
    PrintLog("Trophy Saved.\n");
}

