#define _MAIN_
#include "Hunt.h"
#include <stdio.h>
#include "VideoOptions.h"
#include "IniFile.h"

#include <math.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

Globals* g = Globals::getGlobals();

/*char* strlwr( char* str )
{
	if ( !str ) return 0;
	size_t l = strlen( str );
	for ( int i=0; i<l; ++i )
	{
		if ( str[i] >= 'A' && str[i] <= 'Z' )
		{
			str[i] -= 26;
		}
	}
	return str;
}*/
char *strlwr(char *string)
{
	char c;
	while((c = *string))
	{
		if((c >= 'A') && (c <= 'Z'))
		{
			*string += 32;
		}

		++string;
	}

	return string;
}

#include "Log.h"


float rav=0;
float rbv=0;


bool PHONG = true;
bool GOUR  = true;
bool ENVMAP = true;

bool NeedRVM = true;
float BinocularPower  = 2.5;
float wphipx = 30;
float wphipy = -30;
float wphipz = 0;
float wpshx = 0;
float wpshy = 0;
float wpshz = 0;
float wpmag = 0;
float wpdx  = 0;
float wpdy  = 0;
float wpdz  = 0;
float wpnb  = 0;
int wpnlight;

char cheatcode[16] = "DEBUGUP";
int  cheati = 0;
int  iDevCount = strlen(cDevConsole);

void HideWeapon();

void KeyboardInput( GLFWwindow *window, int key, int scancode, int action, int mods );
void CharacterInput( int, int );



//-----------------------------------------------------------------------------
// XINPUT STUFF
//-----------------------------------------------------------------------------
#include "Controller.h"

float fHeld = 0.0f;
float fThumbLX = 0;
float fThumbLY = 0;
float fThumbRX = 0;
float fThumbRY = 0;


void InitXboxController()
{
	if ( glfwJoystickPresent( GLFW_JOYSTICK_1 ) )
	{
		printf( "There is a joystick!\n" );
		int count = 0;
        glfwGetJoystickAxes( GLFW_JOYSTICK_1, &count );
		printf( "\tNumber of Axes: %d\n", count );
		glfwGetJoystickButtons( GLFW_JOYSTICK_1, &count );
		printf( "\tNumber of Buttons: %d\n", &count );
	}
}

// preliminary update state
uint32_t UpdateControllerState()
{
	memcpy( gController.lastState, gController.state, sizeof(uint8_t)*20 );
	//int retCount = glfwGetJoystickButtons( GLFW_JOYSTICK_1, gController.state, 20 );
	//int retAxes = glfwGetJoystickPos( GLFW_JOYSTICK_1, gController.m_axes, 10 );

	// Connect/Disconnect status
	if ( glfwJoystickPresent( GLFW_JOYSTICK_1 ) )
	{
		if ( !gController.bConnected ) printf( "Gamepad 1 Connected!\n" );
		gController.bConnected = true;
	}
	else
	{
		if ( gController.bConnected ) printf( "Gamepad 1 Disconnected!\n" );
		gController.bConnected = false;
		return false;
	}

	// Zero value if thumbsticks are within the dead zone
	if( ( gController.m_axes[0] < INPUT_DEADZONE &&
			gController.m_axes[0] > -INPUT_DEADZONE ) &&
			( gController.m_axes[1] < INPUT_DEADZONE &&
			  gController.m_axes[1] > -INPUT_DEADZONE ) )
	{
		gController.m_axes[0] = 0.0f;
		gController.m_axes[1] = 0.0f;
	}

	if( ( gController.m_axes[3] < INPUT_DEADZONE &&
			gController.m_axes[3] > -INPUT_DEADZONE ) &&
			( gController.m_axes[4] < INPUT_DEADZONE &&
			  gController.m_axes[4] > -INPUT_DEADZONE ) )
	{
		gController.m_axes[3] = 0.0f;
		gController.m_axes[4] = 0.0f;
	}

	// Pass through the left/right sticks
	fThumbLX = gController.m_axes[0];
	fThumbLY = gController.m_axes[1];
	fThumbRX = -gController.m_axes[3];
	fThumbRY = -gController.m_axes[4];

	return true;
}

bool ProcessShoot();
void ToggleMapMode();

void ProcessXboxControls()
{
	if (gController.bConnected)
	{

		//Camera rotation
		float rat = 2.0f;//(OptMsSens+64) / 600.f / 192.f;
		if (fThumbRX>0)
			PlayerAlpha+=DeltaT*powf(fThumbRX,2)*rat;//1.5f;
		if (fThumbRX<0)
			PlayerAlpha+=DeltaT*-powf(fThumbRX,2)*rat;//1.5f;
		if (fThumbRY>0)
			PlayerBeta += DeltaT*-powf(fThumbRY,2)*rat;
		if (fThumbRY<0)
			PlayerBeta += DeltaT*powf(fThumbRY,2)*rat;

		if (fThumbLY>0.3f) KeyFlags += kfForward;
		if (fThumbLY<-0.3f) KeyFlags += kfBackward;
		if (fThumbLX>0.3f) KeyFlags += kfSRight;
		if (fThumbLX<-0.3f) KeyFlags += kfSLeft;

		if ( gController.m_axes[XBOX_PAD_RTRIGGER] < 0.0f )
		{
			if (ProcessShoot())
			{
				//gController.vibration.wLeftMotorSpeed = 0xFFFF;
				//gController.vibration.wRightMotorSpeed = 0xFFFF;
				//XInputSetState(0,&gController.vibration);
				gController.timeRumble = 350;
			}
		}


		if ( gController.state[XBOX_PAD_Y] || gController.m_axes[XBOX_PAD_LTRIGGER] > 0.0f )
		{
			HideWeapon();
			/*wpdx  = 0-WeapInfo[CurrentWeapon].HipX;
			wpdy  = 0-WeapInfo[CurrentWeapon].HipY;
			wpdz  = 0-WeapInfo[CurrentWeapon].HipZ;
			wpmag = sqrtf(wpdx*wpdx + wpdy*wpdy + wpdz*wpdz);
			wpdx /= wpmag;
			wpdy /= wpmag;
			wpdz /= wpmag;*/
		}

		if ( gController.state[XBOX_PAD_X] )
		{
			MakeCall();
		}

		if ( gController.state[XBOX_PAD_B] )
		{
			if (g->EXITMODE ) g->EXITMODE = false;
		}

		if ( gController.state[XBOX_PAD_A] )
		{
			if ( g->EXITMODE )
			{
				if (MyHealth) ExitTime = 4000;
				else ExitTime = 1;
				g->EXITMODE = false;
			}
			else
				KeyFlags+=kfJump;
		}
		if ( gController.state[XBOX_PAD_BACK] &&
				!(gController.lastState[XBOX_PAD_BACK]))
		{
			if (!TrophyMode) ToggleMapMode();
		}
		if ( gController.state[XBOX_PAD_LB] &&
				!(gController.lastState[XBOX_PAD_LB]))
		{
			g->RunMode = !g->RunMode;
			if (g->RunMode)
				AddMessage("Run Mode ON");
			if (!g->RunMode)
				AddMessage("Run Mode OFF");
		}
		/*if (gController.state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER &&
				!(gController.lastState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER))
		{
			SaveScreenShot();
		}*/

		if ( gController.state[XBOX_PAD_LSTICK] &&
				!( gController.lastState[XBOX_PAD_LSTICK] ))
		{
			g->CrouchMode = !g->CrouchMode;
			if (g->CrouchMode)
				AddMessage("Crouch Mode ON");
			if (!g->CrouchMode)
				AddMessage("Crouch Mode OFF");
		}

		if ( gController.state[XBOX_PAD_RSTICK] &&
				!( gController.lastState[XBOX_PAD_RSTICK] ))
		{
			if ( !Weapon.state )
				g->BINMODE = !g->BINMODE;
		}

		if ( gController.m_axes[XBOX_PAD_DPADX] > 0.0f )
		{
			if (!Weapon.FTime)
			{
				int w = CurrentWeapon+1;
				if (w>TotalW) w=0;
				if (ShotsLeft[w])
				{
					TargetWeapon = w;
					if (!Weapon.state) CurrentWeapon = TargetWeapon;
					HideWeapon();
				}
				else
					AddMessage("No weapon");
			}
		}
		if ( gController.m_axes[XBOX_PAD_DPADX] < 0.0f )
		{
			if (!Weapon.FTime)
			{
				int w = CurrentWeapon-1;
				if (w<0) w=TotalW;
				if (ShotsLeft[w])
				{
					TargetWeapon = w;
					if (!Weapon.state) CurrentWeapon = TargetWeapon;
					HideWeapon();
				}
				else
					AddMessage("No weapon");
			}
		}
		/*if (gController.state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP &&
				!(gController.lastState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP))
		{
			if (TargetDino)
			{
				if (ChCallTime)
					for (int t=0; t<32; t++)
					{
						TargetCall++;
						if (TargetCall>32) TargetCall=10;
						if (TargetDino & (1<<TargetCall)) break;
					}
				ChCallTime = 2048;
			}
		}
		if (gController.state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN &&
				!(gController.lastState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN))
		{
			if (TargetDino)
			{
				if (ChCallTime)
					for (int t=32; t>0; t--)
					{
						TargetCall--;
						if (TargetCall<10) TargetCall=32;
						if (TargetDino & (1<<TargetCall)) break;
					}
				ChCallTime = 2048;
			}
		}*/
		if ( gController.state[XBOX_PAD_START] &&
				!( gController.lastState[XBOX_PAD_START] ))
			if (TrophyMode)
			{
				SaveTrophy();
				ExitTime = 1;
			}
			else
			{
				if (g->PAUSE) g->PAUSE = false;
				else g->EXITMODE = !g->EXITMODE;
				if (ExitTime) g->EXITMODE = false;
				ResetMousePos();
			}
	}
}

void ControllerRelease()
{
#ifndef _w2k
	/*	if (pDSCapture!=NULL)
		pDSCapture->Release();
		if (pDS!=NULL)
		pDS->Release();*/
#endif
}


//-----------------------------------------------------------------------------
// Carnivores
//-----------------------------------------------------------------------------

void ResetMousePos()
{
	//SetCursorPos( WindowCX, WindowCY );
	glfwSetCursorPos( GWindow, WindowCX, WindowCY );
}



float CalcFogLevel(vec3 v)
{
	if (!g->FOGON) return 0;
	bool vinfog = true;
	int cf;
	if (!g->UNDERWATER)
	{
		cf = FogsMap[ ((int)(v.z + CameraZ))>>9 ][ ((int)(v.x + CameraX))>>9 ];
		if ((!cf) && g->CAMERAINFOG)
		{
			cf = CameraFogI;
			vinfog = false;
		}
	}
	else cf = 127;


	if (! (g->CAMERAINFOG | cf) ) return 0;
	TFogEntity *fptr;
	fptr = &FogsList[cf];
	CurFogColor = fptr->fogRGB;

	float d = VectorLength(v);

	v.y+=CameraY;

	float fla= -(v.y     - fptr->YBegin*ctHScale) / ctHScale;
	if (!vinfog) if (fla>0) fla=0;

	float flb = -(CameraY - fptr->YBegin*ctHScale) / ctHScale;
	if (!g->CAMERAINFOG) if (flb>0) flb=0;

	if (fla<0 && flb<0) return 0;

	if (fla<0)
	{
		d*= flb / (flb-fla);
		fla = 0;
	}
	if (flb<0)
	{
		d*= fla / (fla-flb);
		flb = 0;
	}

	float fl = (fla + flb);

	fl *= (d+(fptr->Transp/2)) / fptr->Transp;

	return std::min(fl, fptr->FLimit);
}


/*
	Cache the terrain model and transform it appropriately.
*/
void PreCacheGroundModel()
{
	/*for (y=-(ctViewR+3); y<(ctViewR+3); y++)
	{
		for (x=-(ctViewR+3); x<(ctViewR+3); x++)
		{

			int r = af_max( af_max(y,-y), af_max(x,-x) ); // Get the square range

			if ( r>ctViewR1+4 ) // If we are outside the ViewR1
			{
				if ( (x & 1) + (y & 1) > 0) continue; // Shouldn't these be modulus?
			}

			int xx = (CCX + x) & 1023;
			int yy = (CCY + y) & 1023;

			v[0].x = xx*256 - CameraX;	// translate the coord to be 256 units and relative to the Camera
			v[0].z = yy*256 - CameraZ;	// translate the coord to be 256 units and relative to the Camera
			v[0].y = (float)((int)HMap[yy][xx])*ctHScale - CameraY;

			// Terrain Geomorphing
			if (r>ctViewR1-20 && r<ctViewR1+8)
			{
				if ( (x & 1) + (y & 1) > 0)
				{
					float y1;
					float zd = (float)sqrtf(v[0].x*v[0].x + v[0].z*v[0].z) / 256.f;
					float k = (zd - (ctViewR1-8)) / 4.f;
					if (k<0) k=0;
					if (k>1) k=1;

					if ((y & 1)==0) y1 = (float)((int)HMap[yy][xx-1]+HMap[yy][xx+1])*ctHScale/2 - CameraY;
					else if ((x & 1)==0) y1 = (float)((int)HMap[yy-1][xx]+HMap[yy+1][xx])*ctHScale/2 - CameraY;
					else
						y1 = (float)((int)HMap[yy-1][xx-1]+HMap[yy+1][xx+1])*ctHScale/2 - CameraY;

					v[0].y = ((v[0].y+2) * (1-k) + (y1+8) * k);
				}
			}
		}
	}


	return;*/


	SKYDTime = RealTime>>1;
	int x,y;

	int kx = SKYDTime & 255;
	int ky = SKYDTime & 255;
	int SKYDT = SKYDTime>>8;

	VideoCXf  = (float)VideoCX;
	VideoCYf  = (float)VideoCY;
	CameraW16 = CameraW * 16;
	CameraH16 = CameraH * 16;

	bool FogFound = false;
	g->NeedWater = false;

	MapMinY = 10241024;
	vec3 rv;


	for (y=-(ctViewR+3); y<(ctViewR+3); y++)
		for (x=-(ctViewR+3); x<(ctViewR+3); x++)
		{
			int r = std::max(( std::max(y,-y)), ( std::max(x,-x)));
			if ( r>ctViewR1+4 )
				if ( (x & 1) + (y & 1) > 0) continue;

			int xx = (CCX + x) & 1023;
			int yy = (CCY + y) & 1023;

			v[0].x = xx*256 - CameraX;
			v[0].z = yy*256 - CameraZ;
			v[0].y = (float)((int)HMap[yy][xx])*ctHScale - CameraY;


			// -- Water Section
			if ((FMap[yy][xx] & fmWaterA)>0)
			{
				rv = v[0];
				rv.y = WaterList[ WMap[yy][xx] ].wlevel*ctHScale - CameraY;

				float wdelta = (float)sinf(-pi/2 + RandomMap[yy & 31][xx & 31]/128+RealTime/200.f);

				if ( (FMap[yy][xx] & fmWater) && (r < ctViewR1-4))
				{
					rv.x+=(float)sinf(xx+yy + RealTime/200.f) * 16.f;
					rv.z+=(float)sinf(pi/2.f + xx+yy + RealTime/200.f) * 16.f;
				}

				rv = RotateVector(rv);
				VMap2[128+y][128+x].v = rv;

				if (fabsf(rv.x) > -rv.z + 1524)
				{
					VMap2[128+y][128+x].DFlags = 128; // If the absolute of rv.x is greater than -rv.z + 1524
				}
				else
				{
					g->NeedWater = true;
					VMap2[128+y][128+x].Light = 168-(int)(wdelta*24);

					float Alpha;
					if ( g->UNDERWATER)
					{
						Alpha =	160 - VectorLength(rv)* 160 / 220 / ctViewR;
						if (Alpha<10) Alpha=10;
					}
					else if (r < ctViewR1+2)
					{
						int wi = WMap[yy][xx];
						Alpha = (float)((WaterList[wi].wlevel - HMap[yy][xx])*2+4)*WaterList[wi].transp;
						Alpha+=VectorLength(rv) / 256;
						Alpha+=wdelta*2;
						if (Alpha<0) Alpha=0;
						vec3 va = v[0];
						NormVector(va,1.0f);
						va.y=-va.y;
						if (va.y<0) va.y=0;
						Alpha*=6.f/(va.y+0.1f);
						if (Alpha>255) Alpha=255.f;
					}
					else
					{
						Alpha = 255.f;
					}

					VMap2[128+y][128+x].ALPHA=(int)Alpha;
					VMap2[128+y][128+x].Fog = 0;

					if (rv.z>-256.0)
					{
						VMap2[128+y][128+x].DFlags=128;
					}
					else
					{
						int DF = 0;

						VMap2[128+y][128+x].scrx = VideoCXf - (rv.x / rv.z * (CameraW));
						VMap2[128+y][128+x].scry = VideoCYf + (rv.y / rv.z * (CameraH));

						if (VMap2[128+y][128+x].scrx < 0)        DF+=1;
						if (VMap2[128+y][128+x].scrx > WinEX)    DF+=2;
						if (VMap2[128+y][128+x].scry < 0)        DF+=4;
						if (VMap2[128+y][128+x].scry > WinEY)    DF+=8;

						VMap2[128+y][128+x].DFlags = DF;

					}
				}
			}
			// -- End of water section

			// -- Terrain geomorphing
#ifndef _soft
			if (r>ctViewR1-20 && r<ctViewR1+8)
				if ( (x & 1) + (y & 1) > 0)
				{
					float y1;
					float zd = (float)sqrtf(v[0].x*v[0].x + v[0].z*v[0].z) / 256.f;
					float k = (zd - (ctViewR1-8)) / 4.f;
					if (k<0) k=0;
					if (k>1) k=1;

					if ((y & 1)==0) y1 = (float)((int)HMap[yy][xx-1]+HMap[yy][xx+1])*ctHScale/2 - CameraY;
					else if ((x & 1)==0) y1 = (float)((int)HMap[yy-1][xx]+HMap[yy+1][xx])*ctHScale/2 - CameraY;
					else
						y1 = (float)((int)HMap[yy-1][xx-1]+HMap[yy+1][xx+1])*ctHScale/2 - CameraY;

					v[0].y = ((v[0].y+2) * (1-k) + (y1+8) * k);
				}
#endif

			rv = RotateVector(v[0]);


			if (fabs(rv.x * FOVK) > -rv.z + 1600)
			{
				VMap[128+y][128+x].v = rv;
				VMap[128+y][128+x].DFlags = 128;
				continue;
			}


			if (HARD3D)
			{
				if (  ((FMap[yy][xx] & fmWater)==0) || g->UNDERWATER)
					VMap[128+y][128+x].Fog = CalcFogLevel(v[0]);
				else
					VMap[128+y][128+x].Fog = 0;
			}

			VMap[128+y][128+x].ALPHA = 255;

			v[0]=rv;

			if (v[0].z<1024)
				if (g->FOGENABLE)
					if (FogsMap[yy>>1][xx>>1]) FogFound = true;

			VMap[128+y][128+x].v = v[0];

			int  DF = 0;
			int  db = 0;

			/*
				Cloud Shadow Map
			*/
			if (v[0].z<256)
			{
				if (g->Clouds)
				{
					int shmx = (xx + SKYDT) & 127;
					int shmy = (yy + SKYDT) & 127;

					int db1 = SkyMap[shmy * 128 + shmx ];
					int db2 = SkyMap[shmy * 128 + ((shmx+1) & 127) ];
					int db3 = SkyMap[((shmy+1) & 127) * 128 + shmx ];
					int db4 = SkyMap[((shmy+1) & 127) * 128 + ((shmx+1) & 127) ];
					db = (db1 * (256 - kx) + db2 * kx) * (256-ky) +
						 (db3 * (256 - kx) + db4 * kx) * ky;
					db>>=17;
					db = db - 40;
					if (db<0) db=0;
					if (db>48) db=48;
				}

				int clt = LMap[yy][xx];
				clt= std::max(64, clt-db);
				VMap[128+y][128+x].Light = clt;
			}



			if (v[0].z>-256.0) DF+=128;
			else
			{

				VMap[128+y][128+x].scrx = VideoCXf - (v[0].x / v[0].z * (CameraW));
				VMap[128+y][128+x].scry = VideoCYf + (v[0].y / v[0].z * CameraH);

				if (VMap[128+y][128+x].scrx < 0)        DF+=1;
				if (VMap[128+y][128+x].scrx > WinEX)    DF+=2;
				if (VMap[128+y][128+x].scry < 0)        DF+=4;
				if (VMap[128+y][128+x].scry > WinEY)    DF+=8;
			}

			VMap[128+y][128+x].DFlags = DF;
		}

	g->FOGON = FogFound || g->UNDERWATER;
}




void AddShadowCircle(int x, int y, int R, int D)
{
	if (g->UNDERWATER) return;

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
			VMap[cy+yy - CCY + 128][cx+xx - CCX + 128].Light-= D * (R-r) / R;
			if (VMap[cy+yy - CCY + 128][cx+xx - CCX + 128].Light < 32)
				VMap[cy+yy - CCY + 128][cx+xx - CCX + 128].Light = 32;
		}
}





void DrawScene()
{
	dFacesCount = 0;

	ca = (float)cosf(CameraAlpha);
	sa = (float)sinf(CameraAlpha);

	cb = (float)cosf(CameraBeta);
	sb = (float)sinf(CameraBeta);

	CCX = ((int)CameraX / 512) * 2;
	CCY = ((int)CameraZ / 512) * 2;

	PreCacheGroundModel();

#ifdef _soft
	CreateChRenderList();
#endif

	RenderSkyPlane();

	cb = (float)cosf(CameraBeta);
	sb = (float)sinf(CameraBeta);


	RenderGround();

	RenderModelsList();

	Render3DHardwarePosts();

	if (g->NeedWater) RenderWater();

	RenderElements();


}




void DrawOpticCross( int v)
{
	int sx =  VideoCX + (int)(rVertex[v].x / (-rVertex[v].z) * CameraW);
	int sy =  VideoCY - (int)(rVertex[v].y / (-rVertex[v].z) * CameraH);

	if (  (fabs(float(VideoCX - sx)) > WinW / 2) ||
			(fabs(float(VideoCY - sy)) > WinH / 4) ) return;

	Render_Cross(sx, sy);
}



void ScanLifeForms()
{
	int li = -1;
	float dm = (float)(ctViewR+2)*256;
	for (int c=0; c<ChCount; c++)
	{
		TCharacter *cptr = &Characters[c];
		if (!cptr->Health) continue;
		if (cptr->rpos.z > -512) continue;
		float d = (float)sqrt( cptr->rpos.x*cptr->rpos.x + cptr->rpos.y*cptr->rpos.y + cptr->rpos.z*cptr->rpos.z );
		if (d > ctViewR*256) continue;
		float r = (float)(fabs(cptr->rpos.x) + fabs(cptr->rpos.y)) / d;
		if (r > 0.15) continue;
		if (d<dm)
			if (!TraceLook(cptr->pos.x, cptr->pos.y+220, cptr->pos.z,
						   CameraX, CameraY, CameraZ) )
			{

				dm = d;
				li = c;
			}

	}

	if (li==-1) return;
	Render_LifeInfo(li);
}


void DrawPostObjects()
{
	float b;
	TWeapon* wptr = &Weapon;

	if (DemoPoint.DemoTime) goto SKIPWEAPON;

	{

		Hardware_ZBuffer(false);

		if (wptr->state == 0) goto SKIPWEAPON;

		wpshx = wpshy = wpshz = 0;

		g->MapMode = false;

		wptr->shakel+= TimeDt / 10000.f;
		//wptr->shakel = 0;

		if (wptr->shakel > 4.0f) wptr->shakel = 4.0f;

		if (g->CrouchMode)
			if (wptr->shakel > 0.5f)
			{
				wptr->shakel -= 0.5f;
				if (wptr->shakel < 0.5f)
					wptr->shakel = 0.5f;
			}

		if (KeyFlags & kfForward || KeyFlags & kfBackward || KeyFlags & kfSLeft || KeyFlags & kfSRight)
		{
			wpshy = (float)sin((float)RealTime / 80.f);
			if ( g->CrouchMode ) wpshy *= 0.5f;
		}
		else wpshy = 0;

		if (wptr->state == 1)
		{
			wptr->FTime+=TimeDt;
			if (wptr->FTime >= wptr->chinfo[CurrentWeapon].Animation[0].AniTime)
			{
				wptr->FTime = 0;
				wptr->state = 2;
			}
		}

		if (wptr->state == 4)
		{
			wptr->FTime+=TimeDt;
			if (wptr->FTime >= wptr->chinfo[CurrentWeapon].Animation[3].AniTime)
			{
				wptr->FTime = 0;
				wptr->state = 2;
			}
		}

		if (wptr->state == 2 && wptr->FTime>0)
		{
			wptr->FTime+=TimeDt;
			if (wptr->FTime >= wptr->chinfo[CurrentWeapon].Animation[1].AniTime)
			{
				wptr->FTime = 0;
				wptr->state = 2;


				if (WeapInfo[CurrentWeapon].Reload)
					if ( (ShotsLeft[CurrentWeapon] % WeapInfo[CurrentWeapon].Reload) == 0 )
						if ( (ShotsLeft[CurrentWeapon]>0) || (AmmoMag[CurrentWeapon]>0) )
						{
							wptr->state = 4;
							wptr->FTime = 1;
							g_AudioDevice->addVoice( wptr->chinfo[CurrentWeapon].SoundFX[3] );
						}


				if (!ShotsLeft[CurrentWeapon])
					if (AmmoMag[CurrentWeapon])
					{
						AmmoMag[CurrentWeapon]--;
						ShotsLeft[CurrentWeapon] = WeapInfo[CurrentWeapon].Shots;
						if (wptr->chinfo[CurrentWeapon].Animation[3].AniTime)
						{
							wptr->state = 4;
							wptr->FTime = 1;
							g_AudioDevice->addVoice( wptr->chinfo[CurrentWeapon].SoundFX[3] );
						}
					}
			}
		}

		if (wptr->state == 3)
		{
			wptr->FTime+=TimeDt;
			if (wptr->FTime >= wptr->chinfo[CurrentWeapon].Animation[2].AniTime)
			{
				wptr->FTime = 0;
				wptr->state = 0;
				if (CurrentWeapon != TargetWeapon)
				{
					CurrentWeapon = TargetWeapon;
					HideWeapon();
				}
				goto SKIPWEAPON;
			}
		}


		if (!ShotsLeft[CurrentWeapon])
			if (!AmmoMag[CurrentWeapon])
			{
				HideWeapon();
				for (int w=0; w<10; w++)
					if (ShotsLeft[w])
					{
						TargetWeapon=w;
						break;
					}
			}

		CreateMorphedModel(wptr->chinfo[CurrentWeapon].mptr,
						   &wptr->chinfo[CurrentWeapon].Animation[wptr->state-1], wptr->FTime, 1.0);

		b = (float)sinf((float)RealTime / 300.f) / 100.f;
		wpnDAlpha = wptr->shakel * (float)sinf((float)RealTime / 300.f+pi/2) / 200.f;
		wpnDBeta  = wptr->shakel * (float)sinf((float)RealTime / 300.f) / 400.f;
		nv.z = 0;

		//==================== render weapon ===================//


		vec3 v = Sun3dPos;
		Sun3dPos = RotateVector(Sun3dPos);
		CalcNormals(wptr->chinfo[CurrentWeapon].mptr, wptr->normals);

		wpshx = wpshx + WeapInfo[CurrentWeapon].HipX;
		wpshy = wpshy + WeapInfo[CurrentWeapon].HipY;
		wpshz = wpshz + WeapInfo[CurrentWeapon].HipZ;

		if (GOUR)
			CalcGouraud(wptr->chinfo[CurrentWeapon].mptr, wptr->normals);
		else
			for (int c=0; c<1000; c++)
				wptr->chinfo[CurrentWeapon].mptr->VLight[0][c] = 0;

		if (HARD3D) wpnlight = (int)(96 + GetLandLt(PlayerX, PlayerZ) / 4);
		else wpnlight = 200;

		RenderNearModel(wptr->chinfo[CurrentWeapon].mptr, wpshx, wpshy, wpshz, wpnlight,
						-wpnDAlpha, -wpnDBeta + wpnb);


#ifdef _soft
#else
		if (PHONG)
		{
			CalcPhongMapping(wptr->chinfo[CurrentWeapon].mptr, wptr->normals);
			RenderModelClipPhongMap(wptr->chinfo[CurrentWeapon].mptr, wpshx, wpshy, wpshz, -wpnDAlpha, -wpnDBeta+wpnb);
		}

		if (ENVMAP)
		{
			CalcEnvMapping(wptr->chinfo[CurrentWeapon].mptr, wptr->normals);
			RenderModelClipEnvMap(wptr->chinfo[CurrentWeapon].mptr, wpshx, wpshy, wpshz, -wpnDAlpha, -wpnDBeta+wpnb);
		}
#endif

		Sun3dPos = v;

		//Render_Cross(VideoCX, VideoCY);

	}

SKIPWEAPON:

	Hardware_ZBuffer(false);

	//if (DemoPoint.DemoTime) goto SKIPWIND;

	GlassL = 0;
	if (g->BINMODE)
	{
		RenderNearModel(Binocular, 0, 0, 2*(216-72 * BinocularPower), 192,  0,0);
		ScanLifeForms();
		g->MapMode = false;
	}

	//goto SKIPWIND;
	if (g->BINMODE || g->OPTICMODE) goto SKIPWIND;

	if (!TrophyMode)
		if ( !glfwGetKey( GWindow, GLFW_KEY_CAPS_LOCK) )
		{
			bool lr = g->LOWRESTX;
			g->LOWRESTX = true;
			VideoCX = WinW / 5;
			VideoCY = WinH - (WinH * 10 / 23);
			// Widescreen hack
			//if (Aspect != ASPECT_DEFAULT) VideoCY = WinH - (WinH * 10 / 18);
			VideoCXf= (float)VideoCX;
			VideoCYf= (float)VideoCY;
			CreateMorphedModel(WindModel.mptr, &WindModel.Animation[0], (int)(Wind.speed*50.f), 1.0);
			RenderNearModel(WindModel.mptr, -10, -37, -96, 192,  CameraAlpha-Wind.alpha,0);

			VideoCX = WinW - (WinW / 5);
			VideoCY = WinH - (WinH * 10 / 23);
			// Widescreen hack
			//if (Aspect != ASPECT_DEFAULT) VideoCY = WinH - (WinH * 10 / 18);
			VideoCXf= (float)VideoCX;
			VideoCYf= (float)VideoCY;
			RenderNearModel(CompasModel, +8, -38, -96, 192,  CameraAlpha,0);

			VideoCX = WinW / 2;
			VideoCY = WinH / 2;
			VideoCXf= (float)VideoCX;
			VideoCYf= (float)VideoCY;
			g->LOWRESTX = lr;
		}

SKIPWIND:

	if (ChCallTime)
	{
		ChCallTime -= TimeDt;
		if (ChCallTime<0)
		{
			ChCallTime=0;
		}
		DrawPicture(WinW - 10 - DinoInfo[ AI_to_CIndex[TargetCall] ].CallIcon.m_width, 7,
					DinoInfo[ AI_to_CIndex[TargetCall] ].CallIcon);
	}

	Hardware_ZBuffer(true);

	if (Weapon.state)
	{
		int y0 = 5;
		if (AmmoMag[CurrentWeapon])
			for (int a=0; a<AmmoMag[CurrentWeapon]; a++)
			{
				for (int bl=0; bl<WeapInfo[CurrentWeapon].Shots; bl++)
					DrawPicture(6 + bl*Weapon.BulletPic[CurrentWeapon].m_width, y0, Weapon.BulletPic[CurrentWeapon]);
				y0+=Weapon.BulletPic[CurrentWeapon].m_height+4;
			}

		for (int bl=0; bl<ShotsLeft[CurrentWeapon]; bl++)
			DrawPicture(6 + bl*Weapon.BulletPic[CurrentWeapon].m_width, y0, Weapon.BulletPic[CurrentWeapon]);
	}

	if ( g->OPTICMODE )
	{
		DrawOpticCross(wptr->chinfo[CurrentWeapon].mptr->VCount-1);
	}
	if ( WeapInfo[CurrentWeapon].CrossHair && Weapon.state == 2 )
	{
		Render_Cross( VideoCX, VideoCY );
	}


	if (TrophyMode)
		DrawPicture( VideoCX - TrophyExit.m_width / 2, 2, TrophyExit);

	if ( g->EXITMODE )
		DrawPicture( (WinW - ExitPic.m_width ) / 2, (WinH - ExitPic.m_height) / 2, ExitPic);

	if ( g->PAUSE )
		DrawPicture( (WinW - PausePic.m_width) / 2, (WinH - PausePic.m_height) / 2, PausePic);

	if (TrophyMode || TrophyTime)
		if (TrophyBody!=-1)
		{
			int x0 = WinW - TrophyPic.m_width - 16;
			int y0 = WinH - TrophyPic.m_height - 12;
			if (!TrophyMode)
				x0 = VideoCX - TrophyPic.m_width / 2;

			DrawPicture( x0, y0, TrophyPic);
			DrawTrophyText(x0, y0);

			if (TrophyTime)
			{
				TrophyTime-=TimeDt;
				if (TrophyTime<0)
				{
					TrophyTime=0;
					TrophyBody = -1;
				}
			}
		}
}





void SwitchMode(const char* lps, bool& b)
{
	b = !b;
	char buf[200];
	if (b) sprintf(buf,"%s is ON", lps);
	else sprintf(buf,"%s is OFF", lps);
	////MessageBeep(0xFFFFFFFF);
	AddMessage(buf);
}


void ChangeViewR(int d1, int d2, int d3)
{
	//char buf[200];
	ctViewR +=d1;
	ctViewR1+=d2;
	ctViewRM+=d3;
	if (ctViewR<20) ctViewR = 20;
	if (ctViewR>122) ctViewR = 122;

	if (ctViewR1 < 12) ctViewR1=12;
	if (ctViewR1 > ctViewR-10) ctViewR1=ctViewR-10;
	if (ctViewRM <  4) ctViewRM = 4;
	if (ctViewRM > 60) ctViewRM = 60;

	//sprintf(buf,"ViewR = %d (%d + %d) BMP at %d", ctViewR, ctViewR1, ctViewR-ctViewR1, ctViewRM);
	////MessageBeep(0xFFFFFFFF);
	//AddMessage(buf);

}


void ChangeCall()
{
	if (!TargetDino) return;
	if (ChCallTime)
		for (int t=0; t<32; t++)
		{
			TargetCall++;
			if (TargetCall>32) TargetCall=10;
			if (TargetDino & (1<<TargetCall)) break;
		}
	ChCallTime = 2048;
}


        void CallSupply()
        {
            if (!Supply) return;
            if (SupplyUsed) return;
            SupplyUsed = true;
            if (SupplyShip.State) return;
            g_AudioDevice->addVoice( SShipModel.SoundFX[1] );

            SupplyShip.pos.x = PlayerX - (ctViewR+4)*256;
            if (SupplyShip.pos.x < 256) SupplyShip.pos.x = PlayerX + (ctViewR+4)*256;
            SupplyShip.pos.z = PlayerZ - (ctViewR+4)*256;
            if (SupplyShip.pos.z < 256) SupplyShip.pos.z = PlayerZ + (ctViewR+4)*256;
            SupplyShip.pos.y = GetLandUpH(SupplyShip.pos.x, SupplyShip.pos.z)  + 2048;

            SupplyShip.tgpos.x = PlayerX;
            SupplyShip.tgpos.z = PlayerZ;
            SupplyShip.tgpos.y = GetLandUpH(SupplyShip.tgpos.x, SupplyShip.tgpos.z) + 2048;
            SupplyShip.State = 1;

            SupplyShip.retpos = SupplyShip.pos;
        }


void ToggleBinocular()
{
	if (Weapon.state) return;
	if (g->UNDERWATER) return;
	if (!MyHealth) return;
	g->BINMODE = !g->BINMODE;
	g->MapMode = false;
}


void ToggleRunMode()
{
	g->RunMode = !g->RunMode;
#ifdef __rus
	if (g->RunMode)
		AddMessage("Ðåæèì áåãà.");
	else AddMessage("Запустите режим отключен");//"Ðåæèì õîòüáû.");
#else
	if (g->RunMode)
		AddMessage("Run mode is ON");
	else AddMessage("Run mode is OFF");
#endif
}



void ToggleCrouchMode()
{
	g->CrouchMode = !g->CrouchMode;
	if (g->CrouchMode) AddMessage("Crouch mode is ON");
	else AddMessage("Crouch mode is OFF");
}

void ToggleMapMode()
{
	if (!MyHealth) return;
	if (g->BINMODE) return;
	if (Weapon.state) return;
	g->MapMode = !g->MapMode;
}

void ShowShifts()
{
	sprintf(logt, "X= %3.4f  Y=%3.4f  Z=%3.4f  A=%3.4f", wpshx/2, wpshy/2, wpshz/2, wpnb*180/3.1415);
	AddMessage(logt);
}

/*LONG APIENTRY MainWndProc( HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	bool A = (GetActiveWindow() == hWnd);

	if (A!=blActive)
	{
		blActive = A;

		// alacn
		if (blActive)
#ifdef __high_priority_process
			SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#else
			SetPriorityClass( GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif
		else
			SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS);

		if (!blActive)
		{
			//ShutDown3DHardware();
			NeedRVM = true;
		}

		if (blActive)
		{
			Audio_Restore();
			NeedRVM = true;
		}

	}

	return 0;
}*/

/* This is called very early on, so any initialisation to do with a window or interface elements can come in here */
bool CreateMainWindow()
{
	PrintLog( "Creating main window... " );

	if ( glfwInit() != GL_TRUE )
	{
		PrintLog( "Failed to initialise the system!\n" );
		return false;
	}

#ifdef _DEBUG
	bool fullscreen_mode = false;
#else
	bool fullscreen_mode = true;
#endif
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	//glfwOpenWindowHint( GLFW_FSAA_SAMPLES, 2 );

	if ( Windowed ) fullscreen_mode = false;

	// -- Get the size of the screen
	vec2i screen;
	GetScreenSize( &screen.x, &screen.y );

	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	if ( fullscreen_mode )
		GWindow = glfwCreateWindow( WinW, WinH, "AtmosFEAR Renderer", glfwGetPrimaryMonitor(), nullptr);
    else
        GWindow = glfwCreateWindow( WinW, WinH, "AtmosFEAR Renderer", nullptr, nullptr);

	if ( GWindow == nullptr )
	{
		PrintLog( "Failed to open/create the requested window interface!\n" );
		return false;
	}

	//glfwSetWindowTitle( GWindow, "AtmosFEAR Renderer" );

	glfwSetKeyCallback( GWindow, KeyboardInput );
	//glfwSetCharCallback( GWindow, CharacterInput );

	PrintLog("Ok.\n\n");

#if defined( AF_PLATFORM_WINDOWS )
	hdcMain = wglGetCurrentDC();
#endif

	return true;
}

bool ProcessShoot()
{
	//if (HeadBackR) return;
	if (!ShotsLeft[CurrentWeapon]) return false;
	TWeapon *wptr = &Weapon;
	if (g->UNDERWATER)
	{
		HideWeapon();
		return false;
	}

	if (wptr->state == 2 && wptr->FTime==0)
	{
		wptr->FTime = 1;
		HeadBackR=64;
		//if (HeadBackR>64) HeadBackR=64;

		g_AudioDevice->addVoice(  wptr->chinfo[CurrentWeapon].SoundFX[1] );
		TrophyRoom.Last.smade++;

		for (int s=0; s<=WeapInfo[CurrentWeapon].TraceC; s++)
		{
			float rA = 0;
			float rB = 0;

			rA = siRand(256) * ( 0.00010f * (2.f - WeapInfo[CurrentWeapon].Prec) );
			rB = siRand(256) * ( 0.00010f * (2.f - WeapInfo[CurrentWeapon].Prec) );


			float ca = (float)cosf(PlayerAlpha + wpnDAlpha + rA);
			float sa = (float)sinf(PlayerAlpha + wpnDAlpha + rA);
			float cb = (float)cosf(PlayerBeta + wpnDBeta + rB);
			float sb = (float)sinf(PlayerBeta + wpnDBeta + rB);

			nv.x=sa;
			nv.y=0;
			nv.z=-ca;

			nv.x*=cb;
			nv.y=-sb;
			nv.z*=cb;

			MakeShot(   PlayerX, PlayerY+HeadY, PlayerZ,
						PlayerX+nv.x * 256*ctViewR,
						PlayerY+nv.y * 256*ctViewR + HeadY,
						PlayerZ+nv.z * 256*ctViewR);
		}

		vec3 v;
		v.x = PlayerX;
		v.y = PlayerY;
		v.z = PlayerZ;
		MakeNoise(v, ctViewR*200 * WeapInfo[CurrentWeapon].Loud);
		ShotsLeft[CurrentWeapon]--;
		return true;
	}

	return false;
}


void ProcessSlide()
{
	if (g->NOCLIP || g->UNDERWATER) return;
	float ch = GetLandQHNoObj(PlayerX, PlayerZ);
	float mh = ch;
	float chh;
	int   sd = 0;

	chh=GetLandQHNoObj(PlayerX - 16, PlayerZ);
	if (chh<mh)
	{
		mh = chh;
		sd = 1;
	}
	chh=GetLandQHNoObj(PlayerX + 16, PlayerZ);
	if (chh<mh)
	{
		mh = chh;
		sd = 2;
	}
	chh=GetLandQHNoObj(PlayerX, PlayerZ - 16);
	if (chh<mh)
	{
		mh = chh;
		sd = 3;
	}
	chh=GetLandQHNoObj(PlayerX, PlayerZ + 16);
	if (chh<mh)
	{
		mh = chh;
		sd = 4;
	}

	chh=GetLandQHNoObj(PlayerX - 12, PlayerZ - 12);
	if (chh<mh)
	{
		mh = chh;
		sd = 5;
	}
	chh=GetLandQHNoObj(PlayerX + 12, PlayerZ - 12);
	if (chh<mh)
	{
		mh = chh;
		sd = 6;
	}
	chh=GetLandQHNoObj(PlayerX - 12, PlayerZ + 12);
	if (chh<mh)
	{
		mh = chh;
		sd = 7;
	}
	chh=GetLandQHNoObj(PlayerX + 12, PlayerZ + 12);
	if (chh<mh)
	{
		mh = chh;
		sd = 8;
	}

	if (!g->NOCLIP)
		if (mh<ch-16)
		{
			float delta = (ch-mh) / 4;
			if (sd == 1)
			{
				PlayerX -= delta;
			}
			if (sd == 2)
			{
				PlayerX += delta;
			}
			if (sd == 3)
			{
				PlayerZ -= delta;
			}
			if (sd == 4)
			{
				PlayerZ += delta;
			}

			delta*=0.7f;
			if (sd == 5)
			{
				PlayerX -= delta;
				PlayerZ -= delta;
			}
			if (sd == 6)
			{
				PlayerX += delta;
				PlayerZ -= delta;
			}
			if (sd == 7)
			{
				PlayerX -= delta;
				PlayerZ += delta;
			}
			if (sd == 8)
			{
				PlayerX += delta;
				PlayerZ += delta;
			}
		}
}



        void ProcessPlayerMovement()
        {
            if (bDevShow) return;
            vec2i rc;
            vec2d ms;

            glfwGetWindowSize( GWindow, &rc.x, &rc.y );

            WindowCX = rc.x / 2;
            WindowCY = rc.y / 2;

			glfwGetCursorPos( GWindow, &ms.x, &ms.y );
            //GetCursorPos(&ms);

            if (!g->REVERSEMS) ms.y = -ms.y+VideoCY*2;
            rav += (float)(ms.x-WindowCX) * (OptMsSens+64) / 600.f / 192.f;
            rbv += (float)(ms.y-WindowCY) * (OptMsSens+64) / 600.f / 192.f;
            if (KeyFlags & kfStrafe)
                SSpeed+= (float)rav * 10;
            else
                PlayerAlpha += rav;
            PlayerBeta  += rbv;

            rav/=(2.f + (float)TimeDt/20.f);
            rbv/=(2.f + (float)TimeDt/20.f);
            ResetMousePos();


            if ( !(KeyFlags & (kfForward | kfBackward)))
                if (VSpeed>0) VSpeed= af_max(0,VSpeed-DeltaT*2);
                else VSpeed = af_min(0,VSpeed+DeltaT*2);

            if ( !(KeyFlags & (kfSLeft | kfSRight)))
                if (SSpeed>0) SSpeed= af_max(0,SSpeed-DeltaT*2);
                else SSpeed= af_min(0,SSpeed+DeltaT*2);

            if (KeyFlags & kfForward)  if (VSpeed>0) VSpeed+=DeltaT;
                else VSpeed+=DeltaT*4;
            if (KeyFlags & kfBackward) if (VSpeed<0) VSpeed-=DeltaT;
                else VSpeed-=DeltaT*4;

            if (KeyFlags & kfSRight )  if (SSpeed>0) SSpeed+=DeltaT;
                else SSpeed+=DeltaT*4;
            if (KeyFlags & kfSLeft  )  if (SSpeed<0) SSpeed-=DeltaT;
                else SSpeed-=DeltaT*4;

            if (KeyFlags & kfForward || KeyFlags & kfSRight) if (PlayerSpeed>0) PlayerSpeed+=DeltaT;
                else PlayerSpeed+=DeltaT*4;
            if (KeyFlags & kfBackward || KeyFlags & kfSLeft) if (PlayerSpeed<0) PlayerSpeed-=DeltaT;
                else PlayerSpeed-=DeltaT*4;

            if ( g->SWIM)
            {
                if (VSpeed > 0.25f) VSpeed = 0.25f;
                if (VSpeed <-0.25f) VSpeed =-0.25f;
                if (SSpeed > 0.25f) SSpeed = 0.25f;
                if (SSpeed <-0.25f) SSpeed =-0.25f;
                if (PlayerSpeed > 0.25f) PlayerSpeed = 0.25f;
                if (PlayerSpeed <-0.25f) PlayerSpeed =-0.25f;
            }
            if ( g->RunMode && (HeadY > 190.f) && (Weapon.state==0))
            {
                if (VSpeed > 0.8f) VSpeed = 0.8f;
                if (VSpeed <-0.8f) VSpeed =-0.8f;
                if (SSpeed > 0.8f) SSpeed = 0.8f;
                if (SSpeed <-0.8f) SSpeed =-0.8f;
                if (PlayerSpeed > 0.9f) PlayerSpeed = 0.8f;
                if (PlayerSpeed <-0.9f) PlayerSpeed =-0.8f;
            }
            else
            {
                if (VSpeed > 0.4f) VSpeed = 0.4f;
                if (VSpeed <-0.4f) VSpeed =-0.4f;
                if (SSpeed > 0.4f) SSpeed = 0.4f;
                if (SSpeed <-0.4f) SSpeed =-0.4f;
                if (PlayerSpeed > 0.4f) PlayerSpeed = 0.4f;
                if (PlayerSpeed <-0.4f) PlayerSpeed =-0.4f;
            }
            if (g->CrouchMode)
            {
                if (VSpeed > 0.2f) VSpeed = 0.2f;
                if (VSpeed <-0.2f) VSpeed =-0.2f;
                if (SSpeed > 0.2f) SSpeed = 0.2f;
                if (SSpeed <-0.2f) SSpeed =-0.2f;
                if (PlayerSpeed > 0.2f) PlayerSpeed = 0.2f;
                if (PlayerSpeed <-0.2f) PlayerSpeed =-0.2f;
            }

            if ( glfwGetMouseButton( GWindow, GLFW_MOUSE_BUTTON_1 ) )//KeyboardState [KeyMap.fkFire] & 128)
            {
                ProcessShoot();
            }


            if ( glfwGetMouseButton( GWindow, GLFW_MOUSE_BUTTON_2 ) )//KeyboardState [KeyMap.fkShow] & 128)
                HideWeapon();

            if (g->BINMODE)
            {
                //if (KeyboardState[VK_ADD     ] & 128) BinocularPower+=BinocularPower * TimeDt / 4000.f;
                //if (KeyboardState[VK_SUBTRACT] & 128) BinocularPower-=BinocularPower * TimeDt / 4000.f;
                if (BinocularPower < 1.5f) BinocularPower = 1.5f;
                if (BinocularPower > 3.0f) BinocularPower = 3.0f;
            }

            // === Make animal call === //
            //if (KeyFlags & kfCall)
            if ( glfwGetKey( GWindow, GLFW_KEY_LEFT_ALT ) == GLFW_PRESS )
            {
                MakeCall();
            }

            if (g->DEBUG)
            {

                if ( glfwGetKey( GWindow, GLFW_KEY_LEFT_CONTROL ) )
                    if (KeyFlags & kfBackward) VSpeed =-8;
                    else VSpeed = 8;

            }

			// Jump up, make a noise for AI to hear.
            if (KeyFlags & kfJump)
                if (YSpeed == 0 && !g->SWIM)
                {
                    YSpeed = 600 + (float)fabs(VSpeed) * 600;
                    g_AudioDevice->addVoice( fxJump );
                    vec3 v; v.x = PlayerX, v.y = PlayerY; v.z = PlayerZ;
                    MakeNoise( v, ctViewR*200 * 0.05f );
                }

//=========  rotation =========//
            if (KeyFlags & kfRight)  PlayerAlpha+=DeltaT*1.5f;
            if (KeyFlags & kfLeft )  PlayerAlpha-=DeltaT*1.5f;
            if (KeyFlags & kfLookUp) PlayerBeta-=DeltaT;
            if (KeyFlags & kfLookDn) PlayerBeta+=DeltaT;


//========= movement ==========//

            ca = (float)cosf(PlayerAlpha);
            sa = (float)sinf(PlayerAlpha);
            cb = (float)cosf(PlayerBeta);
            sb = (float)sinf(PlayerBeta);

            nv.x=sa;
            nv.y=0;
            nv.z=-ca;


            PlayerNv = nv;
            if (g->UNDERWATER || g->FLY)
            {
                nv.x*=cb;
                nv.y=-sb;
                nv.z*=cb;
                PlayerNv = nv;
            }
            else
            {
                PlayerNv.x*=cb;
                PlayerNv.y=-sb;
                PlayerNv.z*=cb;
            }

            vec3 sv = nv;
            nv.x*=(float)TimeDt*VSpeed;
            nv.y*=(float)TimeDt*VSpeed;
            nv.z*=(float)TimeDt*VSpeed;

            sv.x*=(float)TimeDt*SSpeed;
            sv.y=0;
            sv.z*=(float)TimeDt*SSpeed;

            if (!TrophyMode)
            {
                TrophyRoom.Last.path+=(TimeDt*VSpeed) / 128.f;
                TrophyRoom.Last.time+=TimeDt/1000.f;
            }

//if (SWIM & (VSpeed>0.1) & (sb>0.60)) HeadY-=40;

            int mvi = 1 + TimeDt / 16;

            for (int mvc = 0; mvc<mvi; mvc++)
            {
                PlayerX+=nv.x / mvi;
                PlayerY+=nv.y / mvi;
                PlayerZ+=nv.z / mvi;

                PlayerX-=sv.z / mvi;
                PlayerZ+=sv.x / mvi;

                if (!g->NOCLIP) CheckCollision(PlayerX, PlayerZ);

                if (PlayerY <= GetLandQHNoObj(PlayerX, PlayerZ)+16)
                {
                    ProcessSlide();
                    ProcessSlide();
                }
            }

            if (PlayerY <= GetLandQHNoObj(PlayerX, PlayerZ)+16)
            {
                ProcessSlide();
                ProcessSlide();
            }
//===========================================================
        }


        void ProcessDemoMovement()
        {
            // == This is the camera orbit when the player dies == //

            g->BINMODE = false;

            g->PAUSE = false;
            g->MapMode = false;

            if (DemoPoint.DemoTime>6*1000)
                if (!g->PAUSE)
                {
                    g->EXITMODE = true;
                    ResetMousePos();
                }

            if (DemoPoint.DemoTime>12*1000)
            {
                //ResetMousePos();
                //DemoPoint.DemoTime = 0;
                //LoadTrophy();
                DoHalt("");
                return;
            }

            VSpeed = 0.f;

            DemoPoint.pos = Characters[DemoPoint.CIndex].pos;
#ifdef _iceage // alacn
            if (Characters[DemoPoint.CIndex].AI == AI_MAMM) DemoPoint.pos.y+=512;
#else
            if (Characters[DemoPoint.CIndex].AI==AI_TREX) DemoPoint.pos.y+=512;
#endif
            DemoPoint.pos.y+=256;

            vec3 nv = SubVectors(DemoPoint.pos,  CameraPos);
            vec3 pp = DemoPoint.pos;
            pp.y = CameraPos.y;
            float l = VectorLength( SubVectors(pp,  CameraPos) );
            float base = 824;
#ifdef _iceage // alacn
            if (Characters[DemoPoint.CIndex].AI == AI_MAMM) base+=512;
#else
            if (Characters[DemoPoint.CIndex].AI==AI_TREX) base=1424;
#endif

            if (DemoPoint.DemoTime==1)
                if (l < base) DemoPoint.DemoTime = 2;
            NormVector(nv, 1.0f);

            if (DemoPoint.DemoTime == 1)
            {
                DeltaFunc(CameraX, DemoPoint.pos.x, (float)fabs(nv.x) * TimeDt * 3.f);
                DeltaFunc(CameraZ, DemoPoint.pos.z, (float)fabs(nv.z) * TimeDt * 3.f);
            }
            else
            {
                DemoPoint.DemoTime+=TimeDt;
                CameraAlpha+=TimeDt / 1224.f;
                ca = (float)cos(CameraAlpha);
                sa = (float)sin(CameraAlpha);
                //float k = (base - l) / 350.f;
                DeltaFunc(CameraX, DemoPoint.pos.x  - sa * base, (float)TimeDt );
                DeltaFunc(CameraZ, DemoPoint.pos.z  + ca * base, (float)TimeDt );
            }

            float b = FindVectorAlpha( (float)
                                       sqrt ( (DemoPoint.pos.x - CameraX)*(DemoPoint.pos.x - CameraX) +
                                              (DemoPoint.pos.z - CameraZ)*(DemoPoint.pos.z - CameraZ) ),
                                       DemoPoint.pos.y - CameraY - 400.f);
            if (b>pi) b = b - 2*pi;
            DeltaFunc(CameraBeta, -b , TimeDt / 4000.f);



            float h = GetLandQH(CameraX, CameraZ);
            DeltaFunc(CameraY, h+128, TimeDt / 8.f);
            if (CameraY < h + 256) CameraY = h + 256; //80
        }


        void ProcessControls()
        {
            if (bDevShow) return;
            int _KeyFlags = KeyFlags;
            KeyFlags = 0;
            //GetKeyboardState(KeyboardState);

            //Start -> Xbox Controller
            ProcessXboxControls();
            //End -> Xbox Controller


            if ( glfwGetKey (GWindow, KeyMap.fkStrafe) ) KeyFlags+=kfStrafe;

            if ( glfwGetKey (GWindow, KeyMap.fkForward ) ) KeyFlags+=kfForward;
            if ( glfwGetKey (GWindow, KeyMap.fkBackward) ) KeyFlags+=kfBackward;

            if ( glfwGetKey (GWindow, KeyMap.fkUp   ) )  KeyFlags+=kfLookUp;
            if ( glfwGetKey (GWindow, KeyMap.fkDown ) )  KeyFlags+=kfLookDn;

            if (KeyFlags & kfStrafe)
            {
                if ( glfwGetKey (GWindow, KeyMap.fkLeft ) )  KeyFlags+=kfSLeft;
                if ( glfwGetKey (GWindow, KeyMap.fkRight) ) KeyFlags+=kfSRight;
            }
            else
            {
                if ( glfwGetKey(GWindow, KeyMap.fkLeft ) )  KeyFlags+=kfLeft;
                if ( glfwGetKey(GWindow, KeyMap.fkRight) ) KeyFlags+=kfRight;
            }

            if ( glfwGetKey(GWindow, KeyMap.fkSLeft)  ) KeyFlags+=kfSLeft;
            if ( glfwGetKey(GWindow, KeyMap.fkSRight) ) KeyFlags+=kfSRight;


            if ( glfwGetKey(GWindow, KeyMap.fkJump) ) KeyFlags+=kfJump;

            if ( glfwGetKey(GWindow, KeyMap.fkCall) )
                if (!(_KeyFlags & kfCall)) KeyFlags+=kfCall;

            DeltaT = (float)TimeDt / 1000.f;

            if ( DemoPoint.DemoTime) ProcessDemoMovement();
            if (!DemoPoint.DemoTime) ProcessPlayerMovement();


//======= Y movement ===========//
            HeadAlpha = HeadBackR / 20000;
            HeadBeta =-HeadBackR / 10000;
            if (HeadBackR)
            {
                HeadBackR-=DeltaT*(80 + (32-(float)fabs(HeadBackR - 32))*4);
                if (HeadBackR<=0)
                {
                    HeadBackR = 0;
                    HeadBSpeed = 0;
                }
            }

            if (g->CrouchMode | (g->UNDERWATER) )
            {
                if (HeadY<110.f) HeadY = 110.f;
                HeadY-=DeltaT*(60 + (HeadY-110)*5);
                if (HeadY<110.f) HeadY = 110.f;
            }
            else
            {
                if (HeadY>220.f) HeadY = 220.f;
                HeadY+=DeltaT*(60 + (220 - HeadY) * 5);
                if (HeadY>220.f) HeadY = 220.f;
            }


            float h  = GetLandQH(PlayerX, PlayerZ);
            float hu = GetLandCeilH(PlayerX, PlayerZ)-64;
            float hwater = GetLandUpH(PlayerX, PlayerZ);

            if (DemoPoint.DemoTime) goto SKIPYMOVE;

            if (!g->UNDERWATER)
            {
                if (PlayerY>h) YSpeed-=DeltaT*3000;
            }
            else if (YSpeed<0)
            {
                YSpeed+=DeltaT*4000;
                if (YSpeed>0) YSpeed=0;
            }

            if (g->FLY) YSpeed=0;
            PlayerY+=YSpeed*DeltaT;


            if (PlayerY+HeadY>hu)
            {
                if (YSpeed>0) YSpeed=-1;
                PlayerY = hu - HeadY;
                if (PlayerY<h)
                {
                    PlayerY = h;
                    HeadY = hu - PlayerY;
                    if (HeadY<110) HeadY = 110;
                }
            }

            if (PlayerY<h)
            {
                if (YSpeed<-800) HeadY+=YSpeed/100;
                if (PlayerY < h-80) PlayerY = h - 80;
                PlayerY+=(h-PlayerY+32)*DeltaT*4;
                if (PlayerY>h) PlayerY = h;
                if (YSpeed<-600)
                    g_AudioDevice->addVoice( fxStep[(RealTime % 3)] ); // v == 64 / 256
                YSpeed = 0;
            }

SKIPYMOVE:

            g->SWIM = false;
            if (!g->UNDERWATER && (KeyFlags & kfJump) )
                if (PlayerY<hwater-148)
                {
                    g->SWIM = true;
                    PlayerY = hwater-148;
                    YSpeed = 0;
                }

            float _s = stepdy;

            if (g->SWIM)  stepdy = (float)sin((float)RealTime / 360) * 20;
            else stepdy = (float)af_min(1.f,fabs(VSpeed) + fabs(SSpeed)) * (float)sin((float)RealTime / 80.f) * 22.f;
            float d = stepdy - _s;

            if (!g->UNDERWATER)
                if (PlayerY<h+64)
                    if (d<0 && stepdd >= 0)
                        if (g->ONWATER)
                        {
                            MakeNoise( vec3( PlayerX, PlayerY, PlayerZ ), ctViewR*200 * 0.12f );
                            AddWCircle(CameraX, CameraZ, 1.2f);
                            g_AudioDevice->addVoice( fxStepW[(RealTime % 3)] ); //64+(int)(VSpeed*30.f));
                        }
                        else
                        {
                            MakeNoise( vec3( PlayerX, PlayerY, PlayerZ ), ctViewR*200 * 0.05f );
                            g_AudioDevice->addVoice(fxStep[(RealTime % 3)] ); //24+(int)(VSpeed*50.f));
                        }
            stepdd = d;

            if (PlayerBeta> 1.46f) PlayerBeta= 1.46f;
            if (PlayerBeta<-1.26f) PlayerBeta=-1.26f;


//======== set camera pos ===================//

            if (!DemoPoint.DemoTime)
            {
                CameraAlpha = PlayerAlpha + HeadAlpha;
                CameraBeta  = PlayerBeta  + HeadBeta;

                CameraX = PlayerX - sa * HeadBackR;
                CameraY = PlayerY + HeadY + stepdy;// + 2024;
                CameraZ = PlayerZ + ca * HeadBackR;
            }

            if (g->CLIP3D)
            {
                if (sb<0) BackViewR = 320.f - 1024.f * sb;
                else BackViewR = 320.f + 512.f * sb;
                BackViewRR = 380 + (int)(1024 * fabs(sb));
                if (g->UNDERWATER) BackViewR -= 512.f * (float)af_min(0,sb);
            }
            else
            {
                BackViewR = 300;
                BackViewRR = 380;
            }


//==================== SWIM & g->UNDERWATER =========================//
            g->ONWATER = (GetLandUpH(CameraX, CameraZ) > GetLandH(CameraX, CameraZ)) &&
                      (PlayerY < GetLandUpH(CameraX, CameraZ));

            if (g->UNDERWATER)
            {
                g->UNDERWATER = (GetLandUpH(CameraX, CameraZ)-4>= CameraY);
                if (!g->UNDERWATER)
                {
                    HeadY+=20;
                    CameraY+=20;
                    //AddVoicev(fxWaterOut.length, fxWaterOut.lpData, 256);
                    AddWCircle(CameraX, CameraZ, 2.0);
                }
            }
            else
            {
                g->UNDERWATER = (GetLandUpH(CameraX, CameraZ)+28 >= CameraY);
                if (g->UNDERWATER)
                {
                    HeadY-=20;
                    CameraY-=20;
                    g->BINMODE = false;
                    //AddVoicev(fxWaterIn.length, fxWaterIn.lpData, 256);
                    AddWCircle(CameraX, CameraZ, 2.0);
                }
            }

            if (MyHealth)
                if (g->UNDERWATER)
                {
                    MyHealth-=TimeDt*14;
                    //if ( !(Takt & 31))
                    AddElements(CameraX + sa*64*cb, CameraY - 32 - sb*64, CameraZ - ca*64*cb, partBubble, 1);
                    if (MyHealth<=0)
                    {
                        AddElements(CameraX + sa*64*cb, CameraY - 32 - sb*64, CameraZ - ca*64*cb, partBubble, 20);
                        AddDeadBody(NULL, HUNT_BREATH);
                    }
                }

            if (g->UNDERWATER && Weapon.state) HideWeapon();

            if (!g->UNDERWATER) UnderWaterT = 0;
            else if (UnderWaterT<512) UnderWaterT += TimeDt;
            else UnderWaterT = 512;

            if (g->UNDERWATER)
            {
                //CameraW = (float)VideoCX*(1.25f + (1.f+(float)cos(RealTime/180.f)) / 30  + (1.f - (float)sin(UnderWaterT/512.f*pi/2)) / 1.5f  );
                //CameraH = (float)VideoCX*(1.25f + (1.f+(float)sin(RealTime/180.f)) / 30  - (1.f - (float)sin(UnderWaterT/512.f*pi/2)) / 16.f  );
                //CameraH *=(WinH*1.3333f / WinW);

                float aspect = (float)WinW / (float)WinH;

                // -- Works for widescreen
                CameraH = float( (WinH * aspect) / 2.0f );
                CameraW = CameraH * (WinH * aspect / WinW);// * 1.25f;

                if ( ( WinW == 640  && WinH == 480  ) ||
                        ( WinW == 800  && WinH == 600  ) ||
                        ( WinW == 1024 && WinH == 768  ) ||
                        ( WinW == 1280 && WinH == 1024 ) )
                {
                    CameraH = float( (WinH * aspect) / 2.0f ) * 1.25f;
                    CameraW = CameraH * (WinH * aspect / WinW);
                }

                CameraW *= (1.0f  + (1.f+(float)cos(RealTime/180.f)) / 30  + (1.f - (float)sin(UnderWaterT/512.f*pi/2)) / 1.5f  );
                CameraH *= (1.0f + (1.f+(float)sin(RealTime/180.f)) / 30  - (1.f - (float)sin(UnderWaterT/512.f*pi/2)) / 16.0f  );

                CameraAlpha+=(float)cos(RealTime/360.f) / 120;
                CameraBeta +=(float)sin(RealTime/360.f) / 100;
                CameraY-=(float)sin(RealTime/360.f) * 4;
                int w = WMap[(((int)(CameraZ))>>8) ][ (((int)(CameraX))>>8) ];
                FogsList[127].YBegin = (float)WaterList[w].wlevel;
                FogsList[127].fogRGB = WaterList[w].fogRGB;
            }
            else
            {
                float aspect = (float)WinW / (float)WinH;

                // -- Works for widescreen
                CameraH = float( (WinH * aspect) / 2.0f );
                CameraW = CameraH * (WinH * aspect / WinW);// * 1.25f;

                //CameraH = float(VideoCX) * aspect;//float( (WinH * aspect) / 2.0f ) * 1.25f;
                //CameraW = float(VideoCX) * aspect;//CameraH * (WinH * aspect / WinW);

                if ( ( WinW == 640  && WinH == 480  ) ||
                        ( WinW == 800  && WinH == 600  ) ||
                        ( WinW == 1024 && WinH == 768  ) ||
                        ( WinW == 1280 && WinH == 1024 ) )
                {
                    CameraH = float(VideoCX) * 1.25f;//float( (WinH * aspect) / 2.0f ) * 1.25f;
                    CameraW = float(VideoCX) * 1.25f;//CameraH * (WinH * aspect / WinW);
                }

                //CameraW = (float)VideoCX*1.25f;
                //CameraH = (float)VideoCX*1.25f;
            }


            if (g->BINMODE)
            {
                CameraW*=BinocularPower;
                CameraH*=BinocularPower;
            }
            else if (g->OPTICMODE)
            {
                CameraW*=3.0f;
                CameraH*=3.0f;
            }
            else if ( Weapon.state == 2 )
            {
                CameraW *= WeapInfo[CurrentWeapon].Zoom;
                CameraH *= WeapInfo[CurrentWeapon].Zoom;
            }

            FOVK = CameraW / (VideoCX*1.25f);

            InitClips();

            if (g->SWIM)
            {
                if (!(Takt & 31)) AddWCircle(CameraX, CameraZ, 1.5);
                CameraBeta -=(float)cos(RealTime/360.f) / 80;
                PlayerX+=DeltaT*32;
                PlayerZ+=DeltaT*32;
            }


            CameraFogI = FogsMap [((int)CameraZ)>>9][((int)CameraX)>>9];
            if (g->UNDERWATER) CameraFogI=127;
            if (FogsList[CameraFogI].YBegin*ctHScale> CameraY)
                g->CAMERAINFOG = (CameraFogI>0);
            else
                g->CAMERAINFOG = false;

            if (g->CAMERAINFOG && MyHealth)
                if (FogsList[CameraFogI].Mortal)
                {
                    if (MyHealth>100000) MyHealth = 100000;
                    MyHealth-=TimeDt*64;
                    if (MyHealth<=0) AddDeadBody(NULL, HUNT_EAT);
                }

            int CameraAmb = AmbMap [((int)CameraZ)>>9][((int)CameraX)>>9];


            if (g->UNDERWATER)
            {
                //SetAmbient( fxUnderwater ); //240);
                Audio_SetEnvironment(8, ctViewR*256.0f);
            }
            else
            {
                SetAmbient(Ambient[CameraAmb].sfx.length,
                           Ambient[CameraAmb].sfx.lpData,
                           Ambient[CameraAmb].AVolume);
                Audio_SetEnvironment(Ambient[CameraAmb].rdata[0].REnvir, ctViewR*256.0f);


                if (Ambient[CameraAmb].RSFXCount)
                {
                    Ambient[CameraAmb].RndTime-=TimeDt;
                    if (Ambient[CameraAmb].RndTime<=0)
                    {
                        Ambient[CameraAmb].RndTime = (Ambient[CameraAmb].rdata[0].RFreq / 2 + rRand(Ambient[CameraAmb].rdata[0].RFreq)) * 1000;
                        int rr = (rand() % Ambient[CameraAmb].RSFXCount);
                        int r = Ambient[CameraAmb].rdata[rr].RNumber;
                        AddVoice3dv(RandSound[r].length, RandSound[r].lpData,
                                    CameraX + siRand(4096),
                                    CameraY + siRand(256),
                                    CameraZ + siRand(4096) ,
                                    Ambient[CameraAmb].rdata[rr].RVolume);
                    }
                }
            }


            if (g->NOCLIP) CameraY+=1024;
            //======= results ==========//
            if (CameraBeta> 1.46f) CameraBeta= 1.46f;
            if (CameraBeta<-1.26f) CameraBeta=-1.26f;

            PlayerPos.x = PlayerX;
            PlayerPos.y = PlayerY;
            PlayerPos.z = PlayerZ;

            CameraPos.x = CameraX;
            CameraPos.y = CameraY;
            CameraPos.z = CameraZ;

        }















void ProcessGame()
{
	if (RestartMode)
	{
		Activate3DHardware();
		AudioStop();
		NeedRVM = true;
	}

	if (!_GameState)
	{
		PrintLog("Entered game\n");
		ReInitGame();
		//while (ShowCursor(false)>=0);
	}

	_GameState = 1;

	if (NeedRVM)
	{
		//SetFocus(hwndMain);
		//NeedRVM = false;
	}

	ProcessSyncro();

	if (!g->PAUSE || !MyHealth)
	{
		UpdateControllerState();//<-Xbox Controller
		ProcessControls();
		AudioSetCameraPos(CameraX, CameraY, CameraZ, CameraAlpha, CameraBeta);
		AnimateCharacters();
		AnimateProcesses();
	}

	if (g->DEBUG || ObservMode || TrophyMode)
		if (MyHealth) MyHealth = MAX_HEALTH;
	if (g->DEBUG) ShotsLeft[CurrentWeapon] = WeapInfo[CurrentWeapon].Shots;

	DrawScene();

	if (!TrophyMode && g->MapMode) DrawHMap();

	DrawPostObjects();

	ShowControlElements();

	ShowVideo();

	double sec = glfwGetTime();
	if ( sec - StepStart >=1)
	{
		StepStart = sec;
		FramesPerSecond = Frames;
		Frames = 0;
		//printf( "FPS: %u | %f\n", FramesPerSecond, (sec - StepStart)*1000 );
	}
	Frames++;
}




int main( int argc, char *argv[] )
{
    // -- Unix needs us to set the working directory to the directory where the binary is.
    InitWorkingDirectory();

	// -- Lord knows what I was doing with this!
	AFLog console( );

	// -- Store the args
	g->argc = argc;
	g->argv = argv;

	// -- Create the Log file
	CreateLog();

	// -- Process the command line
	ProcessCommandLine();

	// -- Create the main window
	if ( !CreateMainWindow() )
	{
		goto main_badwindow;
	}

	// Zero the pointers
	for ( int i=0; i<1024; ++i )
	{
		Textures[i] = 0;
		if ( i < 255 ) MObjects[i].model = 0;
	}

	memset( MessageList, 0, sizeof(TMessageList)*32 );

	Init3DHardware();
	InitXboxController();
	InitEngine();

	// -- Create the Audio Renderer and initialise it
	g_AudioDevice = new AudioAL( );
	g_AudioDevice->init( 32 );

	StartLoading();
	PrintLoad("Loading...");

	PrintLog("== Loading resources ==\n");


	PrintLog("Loading common resources:");
	PrintLoad("Loading common resources: 3DF Files");

	if (OptDayNight==2)
		LoadModelEx(SunModel,    "HUNTDAT/MOON.3DF");
	else
		LoadModelEx(SunModel,    "HUNTDAT/SUN2.3DF");

	LoadModelEx(CompasModel, "HUNTDAT/COMPAS.3DF");
	LoadModelEx(Binocular,   "HUNTDAT/BINOCUL.3DF");
	//LoadModelEx(Binocular,   "HUNTDAT/BINOCUL[16x9].3DF");

	PrintLoad("Loading common resources: CAR Info");

	LoadCharacterInfo(WCircleModel , "HUNTDAT/WCIRCLE2.CAR");
	LoadCharacterInfo(ShipModel, "HUNTDAT/SHIP2A.CAR");
	LoadCharacterInfo(WindModel, "HUNTDAT/WIND.CAR");
	LoadCharacterInfo(SShipModel, "HUNTDAT/SSHIP1.CAR");
	LoadCharacterInfo(AmmoModel, "HUNTDAT/BAG1.CAR");

	PrintLoad("Loading common resources: WAV Files");

	fxUnderwater = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/A_UNDERW.WAV" );

	fxJump = 0xFFFFFFFF;

	fxStep[0] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/STEPS/HWALK1.WAV" );
	fxStep[1] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/STEPS/HWALK2.WAV" );
	fxStep[2] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/STEPS/HWALK3.WAV" );

	fxStepW[0] = g_AudioDevice->loadSound("HUNTDAT/SOUNDFX/STEPS/FOOTW1.WAV" );
	fxStepW[1] = g_AudioDevice->loadSound("HUNTDAT/SOUNDFX/STEPS/FOOTW2.WAV" );
	fxStepW[2] = g_AudioDevice->loadSound("HUNTDAT/SOUNDFX/STEPS/FOOTW3.WAV" );

	fxScream[0] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/HUM_DIE1.WAV" );
	fxScream[1] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/HUM_DIE2.WAV" );
	fxScream[2] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/HUM_DIE3.WAV" );
	fxScream[3] = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/HUM_DIE4.WAV" );

	mscMenu = g_AudioDevice->loadSound( "HUNTDAT/SOUNDFX/MENUTHEME.WAV" );
	//g_AudioDevice->startLoopedVoice( mscMenu );

	PrintLoad("Loading common resources: Image Files");

	LoadPictureTGA(PausePic,   "HUNTDAT/MENU/PAUSE.TGA");
	conv_pic(PausePic);
	LoadPictureTGA(ExitPic,    "HUNTDAT/MENU/EXIT.TGA");
	conv_pic(ExitPic);
	LoadPictureTGA(TrophyExit, "HUNTDAT/MENU/TROPHY_E.TGA");
	conv_pic(TrophyExit);
	LoadPictureTGA(MapPic,     "HUNTDAT/MENU/MAPFRAME.TGA");
	conv_pic(MapPic);

	LoadPictureTGA(TFX_ENVMAP,    "HUNTDAT/FX/ENVMAP.TGA");
	ApplyAlphaFlags((uint16_t*)(TFX_ENVMAP.m_data), TFX_ENVMAP.m_width*TFX_ENVMAP.m_width);
	LoadPictureTGA(TFX_SPECULAR,  "HUNTDAT/FX/SPECULAR.TGA");
	ApplyAlphaFlags((uint16_t*)(TFX_SPECULAR.m_data), TFX_SPECULAR.m_width*TFX_SPECULAR.m_width);


	PrintLog(" Done.\n");

	PrintLoad("Loading area...");
	LoadResources();

	PrintLoad( "Starting Game Network..." );
	InitializeNetwork();

	PrintLoad( "Starting Game..." );
	PrintLog("Loading area: Done.\n");

	EndLoading();

	ProcessSyncro();
	blActive = true;

	PrintLog("Entered game\n");
	ReInitGame();
	//glfwDisable( GLFW_MOUSE_CURSOR );

	Activate3DHardware();

	PrintLog("Entering messages loop.\n");

	StepStart = glfwGetTime()/1000.0;
	FramesPerSecond = 0;
	Frames = 0;

	while( true )
	{
		/*
		Menu Process
		*/
		if (QUITMODE==1)
		{
			ShutDown3DHardware();
			g_AudioDevice->shutdown();
			delete g_AudioDevice;
			QUITMODE=2;
			break;
		}
		if (!QUITMODE)
		{
			if (true)//glfwGetWindowParam(GLFW_ACTIVE))
			{
				//glfwEnable( GLFW_MOUSE_CURSOR );
				ProcessGame();
			}
			else
			{
				usleep( 100000 ); // Sleep for 0,1 second
			}
		}
	}


#if defined( USE_XBOXCONTROLLER )
	ControllerRelease();
#endif

	ShutdownNetwork();
	ShutDownEngine();
	glfwSetInputMode( GWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
	PrintLog("Game normal shutdown.\n");

	CloseLog();

	glfwTerminate();

	// Gratz, we safely escaped the code monster!
	return 0;

main_badwindow:
	glfwTerminate();
	return 1000;	// <- Bad Window Error
}


void CharacterInput( int key, int action )
{
	return;
	if ( action == GLFW_PRESS )
	{
		printf( "CharPressed = %d ( A == %d )\n", key, int('A') );
		if ( key >= 97 && key <= 122 && bDevShow )
		{
			cDevConsole[iDevCount++] = key;
		}
	}
}

void KeyboardInput( GLFWwindow *window, int key, int scancode, int action, int mods )
{
    if ( action == GLFW_PRESS )
    {
    	if ( key == GLFW_KEY_KP_ADD ) SetWireframe( true );
    	if ( key == GLFW_KEY_KP_SUBTRACT ) SetWireframe( false );
    	if ( key == GLFW_KEY_KP_0 ) g->CLIP3D != g->CLIP3D;
        if (key == KeyMap.fkBinoc && !bDevShow) ToggleBinocular();
        if (key == KeyMap.fkCCall && !bDevShow) ChangeCall();
        //if (key == KeyMap.fkSupply && !bDevShow) CallSupply();
        if ( key == GLFW_KEY_RIGHT_CONTROL ) CallSupply();
        //if (key == KeyMap.fkRun    && !bDevShow)
        if ( key == GLFW_KEY_LEFT_SHIFT ) ToggleRunMode();
        if (key == KeyMap.fkCrouch && !bDevShow) ToggleCrouchMode();

        if ( (key == GLFW_KEY_F1) || (key == 96) ) // VK_GRAVE == 192 GLFW_KEY_GRAVE == 96
        {
            bDevShow = !bDevShow;
            memset( cDevConsole, 0, sizeof(cDevConsole) );
        }
        if ( key == GLFW_KEY_ENTER )
        {
            // -> Dev Command Console
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount<2) goto ENDKEYS;

            char *p = strlwr(cDevConsole);
            char *tok = strtok( cDevConsole, " " );

            if ( tok != NULL )
                if (!strcmp( tok, "enable" ))
                {
                    char *next = strtok( 0, " " );
                    if ( next == NULL ) goto END_CONSOLE;

                    if (!strcmp( next, "rain" ))
                    {
                        next = strtok( 0, " " );
                        g->RAIN = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "snow" ))
                    {
                        next = strtok( 0, " " );
                        g->SNOW = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "debug" ))
                    {
                        next = strtok( 0, " " );
                        g->DEBUG = (bool)atoi( next ) & 1;
                        g->CHEATED = true;
                    }
                    else if (!strcmp( next, "clouds" ))
                    {
                        next = strtok( 0, " " );
                        g->Clouds = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "objects" ))
                    {
                        next = strtok( 0, " " );
                        g->MODELS = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "fog" ))
                    {
                        next = strtok( 0, " " );
                        g->FOGENABLE = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "shadows" ))
                    {
                        next = strtok( 0, " " );
                        g->SHADOWS3D = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "particles" ))
                    {
                        next = strtok( 0, " " );
                        g->PARTICLES = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "transparency" ))
                    {
                        next = strtok( 0, " " );
                        g->TRANSPARENCY = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "alphakey" ))
                    {
                        next = strtok( 0, " " );
                        g->ALPHAKEY = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "gourad" ))
                    {
                        next = strtok( 0, " " );
                        GOUR = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "envmap" ))
                    {
                        next = strtok( 0, " " );
                        ENVMAP = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "phong" ))
                    {
                        next = strtok( 0, " " );
                        PHONG = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "waterani" ))
                    {
                        next = strtok( 0, " " );
                        g->WATERANI = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "output" ))
                    {
                        next = strtok( 0, " " );
                        g->TIMER = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "fly" ))
                    {
                        next = strtok( 0, " " );
                        g->FLY = (bool)atoi( next ) & 1;
                    }
                    else if (!strcmp( next, "slow" ))
                    {
                        next = strtok( 0, " " );
                        g->SLOW = (bool)atoi( next ) & 1;
                    }
                    else AddMessage( "Invalid parameter!" );
                }
                else if (!strcmp( tok, "set" ))
                {
                    char *next = strtok( 0, " " );
                    if ( next == NULL ) goto END_CONSOLE;

                    if (!strcmp( next, "ammo" ) && g->DEBUG)
                    {
                        next = strtok( 0, " " );
                        int val = atoi( next );

                        if ( val > 1 )
                        {
                            AddMessage("Cheat: Ammo Refill");
                            g->CHEATED = true;

                            for (int w=0; w<TotalW; w++)
                                if ( WeaponPres & (1<<w) )
                                {
                                    ShotsLeft[w] = WeapInfo[w].Shots;
                                    if (DoubleAmmo) AmmoMag[w] = val;
                                    if (TargetWeapon==-1) TargetWeapon=w;
                                }
                        }
                    }
                    else if (!strcmp( next, "wind.speed" ))
                    {
                        next = strtok( 0, " " );
                        if ( next ) Wind.speed = (float)atof( next );
                    }
                    else if (!strcmp( next, "view.range" ))
                    {
                        next = strtok( 0, " " );
                        if ( next ) ctViewR = atoi( next );
                    }
                    else if (!strcmp( next, "view.ground" ))
                    {
                        next = strtok( 0, " " );
                        if ( next ) ctViewR1 = atoi( next );
                    }
                    else if (!strcmp( next, "view.bitmap" ))
                    {
                        next = strtok( 0, " " );
                        if ( next ) ctViewRM = atoi( next );
                    }
                    else if (!strcmp( next, "resolution" ))
                    {
                        next = strtok( 0, " " );
                        if ( next ) WinW = atoi( next );
                        next = strtok( 0, " " );
                        if ( next ) WinH = atoi( next );
                        CustomRes = true;

                        Activate3DHardware();
                    }
                    else if (!strcmp( next, "windowed" ))
                    {
                        next = strtok( 0, " " );
                        if ( next ) Windowed = (bool)atoi( next );

                        Activate3DHardware();
                    }
                    else if (!strcmp( next, "anisotropy" ))
                    {
                        VideoOptions* vid = GlobalVideoOptions::SharedVariable();
                        next = strtok( 0, " " );
                        if ( next ) vid->mAnisotropyLevels = (float)atof( next );
                    }
                }
                else if (!strcmp( tok, "get" ))
                {

                }
                else if (!strcmp( tok, "render" ))
                {
                    char *next = strtok( 0, " " );

                    if (!strcmp( next, "wire" ))		SetWireframe( true );
                    else if (!strcmp( next, "solid" ))	SetWireframe( false );
                    else AddMessage( "Invalid parameter!" );
                }
                else if (!strcmp( tok, "mode" ))
                {
                    char *next = strtok( 0, " " );

                    if (!strcmp( next, "hunt" ))			SurviveMode = false;
                    else if (!strcmp( next, "survive" ))	SurviveMode = true;
                    else AddMessage( "Invalid parameter!" );
                }
                else if (!strcmp( tok, "version" ))
                {
                	char tstr[256];
                	sprintf( tstr, "Version: %u.%u.%u r%u", Version::MAJOR, Version::MINOR, Version::BUILD, Version::REVISION );
                    AddMessage( tstr );
                }
                else if (!strcmp( tok, "clear" ) || !strcmp( tok, "cls" ))
                {
                	memset( MessageList, 0, sizeof(TMessageList)*32 );
                }
                else if (!strcmp( tok, "help" ) || !strcmp( tok, "?" ))
                {
                    char* next = strtok( 0, " " );
                    if ( next )
                    {
                        if (!strcmp( next, "set" ) )
                        {
                            AddMessage( "paramter :: resolution | wind.speed | view.range | view.bitmap | view.ground | ammo" , 10.0f );
                        }
                    }
                    else
                    {
                        AddMessage( "enable [name] [bool]" );
                        AddMessage( "render [type]" );
                        AddMessage( "version" );
                        AddMessage( "mode [type]" );
                        AddMessage( "set [var] [value]" );
                        AddMessage( "get [var] [value]" );
                        AddMessage( "version [var] [value]" );
                    }
                }
                else AddMessage( "Invalid call!" );
END_CONSOLE:

            //Empty the string
            //MessageBeep(0xFFFF);
            iDevCount = 0;
            memset(cDevConsole,0,256);
        }

        if ( key == GLFW_KEY_BACKSPACE )
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount<1) goto ENDKEYS;
            cDevConsole[iDevCount-1] = 0;
            iDevCount--;
        }

        if ( key >= 65 && key <= 90 )//if ( key >= 'A' && key <= 'Z' )
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount>=256)
            {
                //MessageBeep(0xFFFF);
                goto ENDKEYS;
            }

            char v = key;

            if ( !glfwGetKey( GWindow, GLFW_KEY_LEFT_SHIFT ) ) v += 32;

            cDevConsole[iDevCount] = char( v );
            iDevCount++;

        }

        if ( (key >= int('0')) && (key <= int('9')) )
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount>=256)
            {
                //MessageBeep(0xFFFF);
                goto ENDKEYS;
            }
            cDevConsole[iDevCount] = char(key);
            iDevCount++;
        }
        if ( key == int(189) && ( glfwGetKey( GWindow, GLFW_KEY_LEFT_SHIFT ) ) )
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount>=256)
            {
                //MessageBeep(0xFFFF);
                goto ENDKEYS;
            }
            cDevConsole[iDevCount] = '_';
            iDevCount++;
        }
        if (key == int(189))
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount>=256)
            {
                //MessageBeep(0xFFFF);
                goto ENDKEYS;
            }
            cDevConsole[iDevCount] = '-';
            iDevCount++;
        }
        if (key == int('.'))
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount>=256)
            {
                //MessageBeep(0xFFFF);
                goto ENDKEYS;
            }
            cDevConsole[iDevCount] = '.';
            iDevCount++;
        }
        if (key == GLFW_KEY_SPACE)
        {
            if (!bDevShow) goto ENDKEYS;
            if (iDevCount>=256)
            {
                //MessageBeep(0xFFFF);
                goto ENDKEYS;
            }
            cDevConsole[iDevCount] = ' ';
            iDevCount++;
        }
ENDKEYS:
        ;
    }


	bool CTRL = ( glfwGetKey( GWindow, GLFW_KEY_LEFT_SHIFT ) );

	if ( action == GLFW_PRESS )
	switch (key)
	{
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		if (Weapon.FTime) break;
		if ( bDevShow ) break;
		int w = ( key ) - '1';
		if (!ShotsLeft[w])
		{
			AddMessage("No weapon");
			break;
		}
		TargetWeapon = w;
		if (!Weapon.state)
			CurrentWeapon = TargetWeapon;
		HideWeapon();
		break;
	}

	case 'U':
		if (g->DEBUG) ChangeViewR(0, 0, -2);
		break;
	case 'I':
		if (g->DEBUG) ChangeViewR(0, 0, +2);
		break;
	case 'O':
		if (g->DEBUG) ChangeViewR(0, -2, 0);
		break;
		//case 'P': if (g->DEBUG) ChangeViewR(0, +2, 0); break;
	case '[':
		if (g->DEBUG) ChangeViewR(-2, 0, 0);
		break;
	case ']':
		if (g->DEBUG) ChangeViewR(+2, 0, 0);
		break;

	case 'S':
		if (g->DEBUG && CTRL) SwitchMode("Slow mode",g->SLOW);
		break;
	case 'T':
		if (g->DEBUG && CTRL) SwitchMode("Timer",g->TIMER);
		break;


	case 'M':
		if (g->DEBUG && CTRL) SwitchMode("Draw 3D models",g->MODELS);
		break;
	case 'F':
		if (g->DEBUG && CTRL) SwitchMode("V.Fog",g->FOGENABLE);
		break;
	case 'L':
		if (g->DEBUG && CTRL) SwitchMode("Fly",g->FLY);
		break;
	case 'C':
		if (g->DEBUG && CTRL) SwitchMode("Clouds shadow",g->Clouds);
		break;

	case 'E':
		if (g->DEBUG && CTRL) SwitchMode("Env.Mapping",ENVMAP);
		break;
	case 'G':
		if (g->DEBUG && CTRL) SwitchMode("Gour.Mapping",GOUR);
		break;
	case 'P':
		if (g->DEBUG && CTRL) SwitchMode("Phong Mapping",PHONG);
		else if (g->DEBUG) ChangeViewR(0, +2, 0);
		break;

	case GLFW_KEY_TAB:
		if (!TrophyMode) ToggleMapMode();
		break;

	case GLFW_KEY_PAUSE:
		g->PAUSE = !g->PAUSE;
		g->EXITMODE = false;
		ResetMousePos();
		break;

	case 'N':
		if (g->EXITMODE) g->EXITMODE = false;
		break;

	case GLFW_KEY_ESCAPE:
		if (TrophyMode)
		{
			SaveTrophy();
			ExitTime = 1;
		}
		else
		{
			if (g->PAUSE) g->PAUSE = false;
			else g->EXITMODE = !g->EXITMODE;
			if (ExitTime) g->EXITMODE = false;
			ResetMousePos();
		}
		break;

	case 'Y':
	case GLFW_KEY_ENTER:
		if ( g->EXITMODE )
		{
			if (MyHealth) ExitTime = 4000;
			else ExitTime = 1;
			g->EXITMODE = false;
		}
		break;

	case 'R':
		if (TrophyBody!=-1) RemoveCurrentTrophy();
		if (g->EXITMODE)
		{
			LoadTrophy();
			RestartMode = true;
			_GameState = 0;
			//DoHalt( 0 ); // What the hell is this doing here?
		}
		break;

	case GLFW_KEY_F9:
#if defined( AF_DEBUG )
		ShutDown3DHardware();
		AudioStop();
		DoHalt( "Emergency Debug Halt" );
#endif
		break;

	case GLFW_KEY_F12:
		SaveScreenShot();
		break;
	}
}
