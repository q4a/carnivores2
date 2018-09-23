#include "Hunt.h"
#include <stdio.h>

//bool NewPhase;

#define fx_DIE    0


#define partBlood   1
#define partWater   2
#define partGround  3
#define partBubble  4



#include "AI.h"



int CurDino;

void SetNewTargetPlace(TCharacter *cptr, float R);


void ProcessPrevPhase(TCharacter *cptr)
{
	if(!cptr->pinfo)
	{
		return;    // Dummy check to see if the cptr is valid
	}

	cptr->PPMorphTime += TimeDt;

	if(cptr->PPMorphTime > PMORPHTIME)
	{
		cptr->PrevPhase = cptr->Phase;
	}

	cptr->PrevPFTime += TimeDt;
	cptr->PrevPFTime %= cptr->pinfo->Animation[cptr->PrevPhase].AniTime; // SIGSEGV
}


void ActivateCharacterFx(TCharacter *cptr)
{
	Globals *g = Globals::getGlobals();

	if(cptr->AI && g->UNDERWATER)
	{
		return;
	}

	int fx = cptr->pinfo->Anifx[cptr->Phase];

	if(fx == -1)
	{
		return;
	}

	if(VectorLength(SubVectors(PlayerPos, cptr->pos)) > 68 * 256)
	{
		return;
	}

	g_AudioDevice->addVoice( cptr->pinfo->SoundFX[fx], 4.0f, 4.0, 0);
}


void ResetCharacter(TCharacter *cptr)
{
	cptr->AI     = DinoInfo[cptr->CType].AI;
	cptr->pinfo  = &ChInfo[cptr->CType];
	cptr->State  =  0;
	cptr->StateF =  0;
	cptr->Phase  =  0;
	cptr->FTime  =  0;
	cptr->PrevPhase   = 0;
	cptr->PrevPFTime  = 0;
	cptr->PPMorphTime = 0;
	cptr->beta    =  0;
	cptr->gamma   =  0;
	cptr->tggamma =  0;
	cptr->bend    =  0;
	cptr->rspeed  =  0;
	cptr->AfraidTime = 0;
	cptr->BloodTTime = 0;
	cptr->BloodTime  = 0;

	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	cptr->Health = DinoInfo[cptr->CType].Health0;

	if(OptAgres > 128)
	{
		cptr->Health = (cptr->Health * OptAgres) / 128;
	}

	cptr->scale = (float)(DinoInfo[cptr->CType].Scale0 + rRand(DinoInfo[cptr->CType].ScaleA)) / 1000.f;
}


void AddDeadBody(TCharacter *cptr, int phase)
{
	Globals *g = Globals::getGlobals();

	if(!MyHealth)
	{
		return;
	}

	if(ExitTime)
#ifdef __rus
		AddMessage("Транспортировка отменена.");

#else
		AddMessage("Transportation cancelled.");
#endif
	ExitTime = 0;

	g->OPTICMODE = false;
	g->BINMODE = false;
	Characters[ChCount].CType = 0;
	Characters[ChCount].alpha = CameraAlpha;
	ResetCharacter(&Characters[ChCount]);

	//int v = rRand(3);

	if(phase != HUNT_BREATH)
	{
		//AddVoicev( fxScream[r].length, fxScream[r].lpData, 256 );
		g_AudioDevice->addVoice( fxScream[r] );
	}

	Characters[ChCount].Health = 0;
	DemoPoint.CIndex = ChCount;

	MyHealth = 0;

	if(cptr)
	{
#ifdef _iceage // alacn

		if(!(cptr->StateF & csONWATER))
		{
			cptr->Phase = cptr->pinfo->AniCount - 1;
		}

		Characters[ChCount].scale = cptr->scale;
		Characters[ChCount].pos   = cptr->pos;
		Characters[ChCount].alpha = cptr->alpha;
		Characters[ChCount].beta  = cptr->beta;
		phase = HUNT_KILL + (cptr->AI - 10);
		cptr->bend = 0;
#else // _iceage
		float pl = 170;

		if(cptr->AI == AI_SPINO)
		{
			pl = 200.f;
		}

		if(cptr->AI == AI_CERAT)
		{
			pl = 320.f;
		}

		if(cptr->AI == AI_TREX)
		{
			pl = 0;
		}

		Characters[ChCount].pos.x = cptr->pos.x + cptr->lookx * pl * cptr->scale;
		Characters[ChCount].pos.z = cptr->pos.z + cptr->lookz * pl * cptr->scale;
		Characters[ChCount].pos.y = GetLandQH(Characters[ChCount].pos.x, Characters[ChCount].pos.z);
#endif
		DemoPoint.CIndex = CurDino;
	}

	else
	{
		Characters[ChCount].pos.x = PlayerX;
		Characters[ChCount].pos.z = PlayerZ;
		Characters[ChCount].pos.y = PlayerY;
	}

	Characters[ChCount].Phase = phase;
	Characters[ChCount].PrevPhase = phase;

	ActivateCharacterFx(&Characters[ChCount]);


	DemoPoint.pos = Characters[ChCount].pos;
	DemoPoint.DemoTime = 1;


	ChCount++;
}



float AngleDifference(float a, float b)
{
	a -= b;
	a = (float)fabs(a);

	if(a > pi)
	{
		a = 2 * pi - a;
	}

	return a;
}

float CorrectedAlpha(float a, float b)
{
	float d = (float)fabs(a - b);

	if(d < pi)
	{
		return (a + b) / 2;
	}

	else
	{
		d = (a + pi * 2 - b);
	}

	if(d < 0)
	{
		d += 2 * pi;
	}

	if(d > 2 * pi)
	{
		d -= 2 * pi;
	}

	return d;
}

void ThinkY_Beta_Gamma(TCharacter *cptr, float blook, float glook, float blim, float glim)
{
	cptr->pos.y = GetLandH(cptr->pos.x, cptr->pos.z);

	//=== beta ===//
	float hlook  = GetLandH(cptr->pos.x + cptr->lookx * blook, cptr->pos.z + cptr->lookz * blook);
	float hlook2 = GetLandH(cptr->pos.x - cptr->lookx * blook, cptr->pos.z - cptr->lookz * blook);
	DeltaFunc(cptr->beta, (hlook2 - hlook) / (blook * 3.2f), TimeDt / 800.f);

	if(cptr->beta > blim)
	{
		cptr->beta = blim;
	}

	if(cptr->beta < -blim)
	{
		cptr->beta = -blim;
	}

	//=== gamma ===//
	hlook = GetLandH(cptr->pos.x + cptr->lookz * glook, cptr->pos.z - cptr->lookx * glook);
	hlook2 = GetLandH(cptr->pos.x - cptr->lookz * glook, cptr->pos.z + cptr->lookx * glook);
	cptr->tggamma = (hlook - hlook2) / (glook * 3.2f);

	if(cptr->tggamma > glim)
	{
		cptr->tggamma = glim;
	}

	if(cptr->tggamma < -glim)
	{
		cptr->tggamma = -glim;
	}

	/*
	    if (DEBUG) cptr->tggamma = 0;
	    if (DEBUG) cptr->beta    = 0;
	    */
}




int CheckPlaceCollisionP(vec3 &v)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	if(ccx < 4 || ccz < 4 || ccx > 1008 || ccz > 1008)
	{
		return 1;
	}

	int F = (FMap[ccz][ccx - 1] | FMap[ccz - 1][ccx] | FMap[ccz - 1][ccx - 1] |
	         FMap[ccz][ccx] |
	         FMap[ccz + 1][ccx] | FMap[ccz][ccx + 1] | FMap[ccz + 1][ccx + 1]);

	if(F & (fmWater + fmNOWAY))
	{
		return 1;
	}


	float h = GetLandH(v.x, v.z);
	v.y = h;

	float hh = GetLandH(v.x - 164, v.z - 164);

	if(fabs(hh - h) > 160)
	{
		return 1;
	}

	hh = GetLandH(v.x + 164, v.z - 164);

	if(fabs(hh - h) > 160)
	{
		return 1;
	}

	hh = GetLandH(v.x - 164, v.z + 164);

	if(fabs(hh - h) > 160)
	{
		return 1;
	}

	hh = GetLandH(v.x + 164, v.z + 164);

	if(fabs(hh - h) > 160)
	{
		return 1;
	}

	for(int z = -2; z <= 2; z++)
		for(int x = -2; x <= 2; x++)
			if(OMap[ccz + z][ccx + x] != 255)
			{
				int ob = OMap[ccz + z][ccx + x];

				if(MObjects[ob].info.Radius < 10)
				{
					continue;
				}

				float CR = (float)MObjects[ob].info.Radius + 64;

				float oz = (ccz + z) * 256.f + 128.f;
				float ox = (ccx + x) * 256.f + 128.f;

				float r = (float) sqrt((ox - v.x) * (ox - v.x) + (oz - v.z) * (oz - v.z));

				if(r < CR)
				{
					return 1;
				}
			}

	return 0;
}




int CheckPlaceCollision(vec3 &v, bool wc, bool mc)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	if(ccx < 4 || ccz < 4 || ccx > 1018 || ccz > 1018)
	{
		return 1;
	}

	if(wc)
		if((FMap[ccz][ccx - 1] | FMap[ccz - 1][ccx] | FMap[ccz - 1][ccx - 1] |
		        FMap[ccz][ccx] |
		        FMap[ccz + 1][ccx] | FMap[ccz][ccx + 1] | FMap[ccz + 1][ccx + 1]) & fmWater)
		{
			return 1;
		}


	float h = GetLandH(v.x, v.z);

	if(!(FMap[ccz][ccx] & fmWater))
		if(fabs(h - v.y) > 64)
		{
			return 1;
		}

	v.y = h;

	float hh = GetLandH(v.x - 64, v.z - 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	hh = GetLandH(v.x + 64, v.z - 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	hh = GetLandH(v.x - 64, v.z + 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	hh = GetLandH(v.x + 64, v.z + 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	if(mc)
		for(int z = -2; z <= 2; z++)
			for(int x = -2; x <= 2; x++)
				if(OMap[ccz + z][ccx + x] != 255)
				{
					int ob = OMap[ccz + z][ccx + x];

					if(MObjects[ob].info.Radius < 10)
					{
						continue;
					}

					float CR = (float)MObjects[ob].info.Radius + 64;

					float oz = (ccz + z) * 256.f + 128.f;
					float ox = (ccx + x) * 256.f + 128.f;

					float r = (float) sqrt((ox - v.x) * (ox - v.x) + (oz - v.z) * (oz - v.z));

					if(r < CR)
					{
						return 1;
					}
				}

	return 0;
}






int CheckPlaceCollision2(vec3 &v, bool wc)
{
	int ccx = (int)v.x / 256;
	int ccz = (int)v.z / 256;

	if(ccx < 4 || ccz < 4 || ccx > 1018 || ccz > 1018)
	{
		return 1;
	}

	if(wc)
		if((FMap[ccz][ccx - 1] | FMap[ccz - 1][ccx] | FMap[ccz - 1][ccx - 1] |
		        FMap[ccz][ccx] |
		        FMap[ccz + 1][ccx] | FMap[ccz][ccx + 1] | FMap[ccz + 1][ccx + 1]) & fmWater)
		{
			return 1;
		}

	float h = GetLandH(v.x, v.z);
	/*if (! (FMap[ccz][ccx] & fmWater) )
	  if (fabs(h - v.y) > 64) return 1;*/
	v.y = h;

	float hh = GetLandH(v.x - 64, v.z - 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	hh = GetLandH(v.x + 64, v.z - 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	hh = GetLandH(v.x - 64, v.z + 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	hh = GetLandH(v.x + 64, v.z + 64);

	if(fabs(hh - h) > 100)
	{
		return 1;
	}

	return 0;
}



int CheckPossiblePath(TCharacter *cptr, bool wc, bool mc)
{
	vec3 p = cptr->pos;
	float lookx = (float)cos(cptr->tgalpha);
	float lookz = (float)sin(cptr->tgalpha);
	int c = 0;

	for(int t = 0; t < 20; t++)
	{
		p.x += lookx * 64.f;
		p.z += lookz * 64.f;

		if(CheckPlaceCollision(p, wc, mc))
		{
			c++;
		}
	}

	return c;
}


void LookForAWay(TCharacter *cptr, bool wc, bool mc)
{
	float alpha = cptr->tgalpha;
	float dalpha = 15.f;
	float afound = alpha;
	int maxp = 16;
	int curp;

	if(!CheckPossiblePath(cptr, wc, mc))
	{
		cptr->NoWayCnt = 0;
		return;
	}

	cptr->NoWayCnt++;

	for(int i = 0; i < 12; i++)
	{
		cptr->tgalpha = alpha + dalpha * pi / 180.f;
		curp = CheckPossiblePath(cptr, wc, mc) + (i >> 1);

		if(!curp)
		{
			return;
		}

		if(curp < maxp)
		{
			maxp = curp;
			afound = cptr->tgalpha;
		}

		cptr->tgalpha = alpha - dalpha * pi / 180.f;
		curp = CheckPossiblePath(cptr, wc, mc) + (i >> 1);

		if(!curp)
		{
			return;
		}

		if(curp < maxp)
		{
			maxp = curp;
			afound = cptr->tgalpha;
		}

		dalpha += 15.f;
	}

	cptr->tgalpha = afound;
}




bool ReplaceCharacterForward(TCharacter *cptr)
{
	float al = CameraAlpha + (float)siRand(2048) / 2048.f;
	float sa = (float)sin(al);
	float ca = (float)cos(al);
	vec3 p;
	p.x = PlayerX + sa * (ctViewR + rRand(10)) * 256;
	p.z = PlayerZ - ca * (ctViewR + rRand(10)) * 256;
	p.y = GetLandH(p.x, p.z);

	if(p.x <  16 * 256)
	{
		return false;
	}

	if(p.z <  16 * 256)
	{
		return false;
	}

	if(p.x > 1000 * 256)
	{
		return false;
	}

	if(p.z > 1000 * 256)
	{
		return false;
	}

	if(CheckPlaceCollisionP(p))
	{
		return false;
	}

	cptr->State = 0;
	cptr->pos = p;
	//cptr->tgx = cptr->pos.x + siRand(2048);
	//cptr->tgz = cptr->pos.z + siRand(2048);
	SetNewTargetPlace(cptr, 2048);

#ifdef _iceage // alacn

	if(cptr->AI == AI_ARCHEO) //===== dimor ========//
#else
	if(cptr->AI == AI_DIMOR) //===== dimor ========//
#endif
		cptr->pos.y += 1048.f;

	return true;
}


void Characters_AddSecondaryOne(int ctype)
{
	if(ChCount > 64)
	{
		return;
	}

	Characters[ChCount].CType = ctype;
	int tr = 0;
replace1:
	tr++;

	if(tr > 128)
	{
		return;
	}

	Characters[ChCount].pos.x = PlayerX + siRand(20040);
	Characters[ChCount].pos.z = PlayerZ + siRand(20040);
	Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
	                                     Characters[ChCount].pos.z);

	if(CheckPlaceCollisionP(Characters[ChCount].pos))
	{
		goto replace1;
	}

	if(fabs(Characters[ChCount].pos.x - PlayerX) +
	        fabs(Characters[ChCount].pos.z - PlayerZ) < 256 * 40)
	{
		goto replace1;
	}

	Characters[ChCount].tgx = Characters[ChCount].pos.x;
	Characters[ChCount].tgz = Characters[ChCount].pos.z;
	Characters[ChCount].tgtime = 0;

	ResetCharacter(&Characters[ChCount]);
	ChCount++;
}



void MoveCharacter(TCharacter *cptr, float dx, float dz, bool wc, bool mc)
{
	//  return;
	vec3 p = cptr->pos;

	if(CheckPlaceCollision2(p, wc))
	{
		cptr->pos.x += dx / 2;
		cptr->pos.z += dz / 2;
		return;
	}

	p.x += dx;
	p.z += dz;

	if(!CheckPlaceCollision2(p, wc))
	{
		cptr->pos = p;
		return;
	}

	p = cptr->pos;
	p.x += dx / 2;
	p.z += dz / 2;

	if(!CheckPlaceCollision2(p, wc))
	{
		cptr->pos = p;
	}

	p = cptr->pos;

	p.x += dx / 4;
	//if (!CheckPlaceCollision2(p)) cptr->pos = p;
	p.z += dz / 4;
	//if (!CheckPlaceCollision2(p)) cptr->pos = p;
	cptr->pos = p;
}




void MoveCharacter2(TCharacter *cptr, float dx, float dz)
{
	cptr->pos.x += dx;
	cptr->pos.z += dz;
}




void SetNewTargetPlace(TCharacter *cptr, float R)
{
	vec3 p;
	int tr = 0;
replace:
	p.x = cptr->pos.x + siRand((int)R);

	if(p.x < 512)
	{
		p.x = 512;
	}

	if(p.x > 1018 * 256)
	{
		p.x = 1018 * 256;
	}

	p.z = cptr->pos.z + siRand((int)R);

	if(p.z < 512)
	{
		p.z = 512;
	}

	if(p.z > 1018 * 256)
	{
		p.z = 1018 * 256;
	}

	p.y = GetLandH(p.x, p.z);
	tr++;

	if(tr < 128)
		if(fabs(p.x - cptr->pos.x) + fabs(p.z - cptr->pos.z) < R / 2.f)
		{
			goto replace;
		}

	R += 512;

	if(tr < 256)
		if(CheckPlaceCollisionP(p))
		{
			goto replace;
		}

	cptr->tgtime = 0;
	cptr->tgx = p.x;
	cptr->tgz = p.z;
}



void SetNewTargetPlace_Brahi(TCharacter *cptr, float R)
{
	vec3 p;
	int tr = 0;
replace:
	p.x = cptr->pos.x + siRand((int)R);

	if(p.x < 512)
	{
		p.x = 512;
	}

	if(p.x > 1018 * 256)
	{
		p.x = 1018 * 256;
	}

	p.z = cptr->pos.z + siRand((int)R);

	if(p.z < 512)
	{
		p.z = 512;
	}

	if(p.z > 1018 * 256)
	{
		p.z = 1018 * 256;
	}

	tr++;

	if(tr < 16)
		if(fabs(p.x - cptr->pos.x) + fabs(p.z - cptr->pos.z) < R / 2.f)
		{
			goto replace;
		}

	p.y = GetLandH(p.x, p.z);
	float wy = GetLandUpH(p.x, p.z) - p.y;

	if(tr < 128)
	{
		if(wy > 400)
		{
			goto replace;
		}

		if(wy < 200)
		{
			goto replace;
		}
	}

	cptr->tgtime = 0;
	cptr->tgx = p.x;
	cptr->tgz = p.z;
}



void AnimateHuntDead(TCharacter *cptr)
{

	//if (!cptr->FTime) ActivateCharacterFx(cptr);

	ProcessPrevPhase(cptr);
	//bool NewPhase = false;

	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		//NewPhase = true;

		if(cptr->Phase == 2)
		{
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
		}

		else
		{
			cptr->FTime = 0;
		}

		if(cptr->Phase == 1)
		{
			cptr->FTime = 0;
			cptr->Phase = 2;
		}

		ActivateCharacterFx(cptr);
	}


	float h = GetLandH(cptr->pos.x, cptr->pos.z);
	DeltaFunc(cptr->pos.y, h, TimeDt / 5.f);

	if(cptr->Phase == 2)
		if(cptr->pos.y > h + 3)
		{
			cptr->FTime = 0;
			////MessageBeep(0xFFFFFFFF);
		}


	if(cptr->pos.y < h + 256)
	{
		//=== beta ===//
		float blook = 256;
		float hlook  = GetLandH(cptr->pos.x + cptr->lookx * blook, cptr->pos.z + cptr->lookz * blook);
		float hlook2 = GetLandH(cptr->pos.x - cptr->lookx * blook, cptr->pos.z - cptr->lookz * blook);
		DeltaFunc(cptr->beta, (hlook2 - hlook) / (blook * 3.2f), TimeDt / 1800.f);

		if(cptr->beta > 0.4f)
		{
			cptr->beta = 0.4f;
		}

		if(cptr->beta < -0.4f)
		{
			cptr->beta = -0.4f;
		}

		//=== gamma ===//
		float glook = 256;
		hlook = GetLandH(cptr->pos.x + cptr->lookz * glook, cptr->pos.z - cptr->lookx * glook);
		hlook2 = GetLandH(cptr->pos.x - cptr->lookz * glook, cptr->pos.z + cptr->lookx * glook);
		cptr->tggamma = (hlook - hlook2) / (glook * 3.2f);

		if(cptr->tggamma > 0.4f)
		{
			cptr->tggamma = 0.4f;
		}

		if(cptr->tggamma < -0.4f)
		{
			cptr->tggamma = -0.4f;
		}

		DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1800.f);
	}


	TCharacter *cptr2 = &Characters[DemoPoint.CIndex];
#ifdef _iceage // alacn

	if(cptr2->AI >= 10)
	{
		cptr->pos = cptr2->pos;
		cptr->FTime = cptr2->FTime;
		cptr->alpha = cptr2->alpha;
		cptr->beta  = cptr2->beta;
		cptr->gamma = cptr2->gamma;
	}

#else

	if(cptr2->AI == AI_TREX)  //==== T-rex =======//
	{
		cptr->pos = cptr2->pos;
		cptr->FTime = cptr2->FTime;
		cptr->beta = cptr2->beta;
		cptr->gamma = cptr2->gamma;
	}

#endif

}


#ifdef _iceage // alacn


void AnimateDiatrDead(TCharacter *cptr)
{

	if(cptr->Phase != DIA_DIE && cptr->Phase != DIA_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = DIA_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = DIA_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimatePigDead(TCharacter *cptr)
{

	if(cptr->Phase != PIG_DIE && cptr->Phase != PIG_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = PIG_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = PIG_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateDimorDead(TCharacter *cptr)
{

	if(cptr->Phase != DIM_FALL && cptr->Phase != DIM_DIE)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = DIM_FALL;
		cptr->rspeed = 0;
		ActivateCharacterFx(cptr);
		return;
	}

	ProcessPrevPhase(cptr);

	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		if(cptr->Phase == DIM_DIE)
		{
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
		}

		else
		{
			cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		}


	//======= movement ===========//
	if(cptr->Phase == DIM_DIE)
	{
		DeltaFunc(cptr->vspeed, 0, TimeDt / 400.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, 0, TimeDt / 1200.f);
	}

	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	if(cptr->Phase == DIM_FALL)
	{
		float wh = GetLandUpH(cptr->pos.x, cptr->pos.z);
		float lh = GetLandH(cptr->pos.x, cptr->pos.z);
		bool OnWater = (wh > lh);

		if(OnWater)
			if(cptr->pos.y >= wh && (cptr->pos.y + cptr->rspeed * TimeDt / 1024) < wh)
			{
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 2.0);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 2.5);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 3.0);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 3.5);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 3.0);
			}

		cptr->pos.y += cptr->rspeed * TimeDt / 1024;
		cptr->rspeed -= TimeDt * 2.56f;

		if(cptr->pos.y < lh)
		{
			cptr->pos.y = lh;

			if(OnWater)
			{
				AddElements(cptr->pos.x + siRand(128), lh, cptr->pos.z + siRand(128), 4, 10);
				AddElements(cptr->pos.x + siRand(128), lh, cptr->pos.z + siRand(128), 4, 10);
				AddElements(cptr->pos.x + siRand(128), lh, cptr->pos.z + siRand(128), 4, 10);
			}

			if(cptr->PPMorphTime > 128)
			{
				cptr->PrevPhase = cptr->Phase;
				cptr->PrevPFTime  = cptr->FTime;
				cptr->PPMorphTime = 0;
			}

			cptr->Phase = DIM_DIE;
			cptr->FTime = 0;
			ActivateCharacterFx(cptr);
		}
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 140, 126, 0.6f, 0.5f);
		DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
	}
}


void AnimateBronDead(TCharacter *cptr)
{

	if(cptr->Phase != BRO_DIE && cptr->Phase != BRO_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = BRO_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = BRO_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateHogDead(TCharacter *cptr)
{

	if(cptr->Phase != BRO_DIE && cptr->Phase != BRO_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = BRO_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = BRO_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateRhinoDead(TCharacter *cptr)
{

	if(cptr->Phase != BRO_DIE && cptr->Phase != BRO_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = BRO_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = BRO_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateSmiloDead(TCharacter *cptr)
{

	if(cptr->Phase != BRO_DIE && cptr->Phase != BRO_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = BRO_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = BRO_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateDeerDead(TCharacter *cptr)
{

	if(cptr->Phase != DER_DIE && cptr->Phase != DER_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = DER_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = DER_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateDiatr(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);

	if(cptr->State == 2)
	{
		if(cptr->Phase != DIA_JUMP)
		{
			NewPhase = true;
		}

		cptr->State = 1;
	}


	if(GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 140 * cptr->scale)
	{
		cptr->StateF |= csONWATER;
	}

	else
	{
		cptr->StateF &= (!csONWATER);
	}

	if(cptr->Phase == DIA_EAT)
	{
		goto NOTHINK;
	}

	//============================================//
	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		if(pdist > ctViewR * 140 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;

			if(cptr->AfraidTime <= 0)
			{
				cptr->AfraidTime = 0;
				cptr->State = 0;
			}
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}

		if(!(cptr->StateF & csONWATER))
			if(pdist < 1324 * cptr->scale && pdist > 900 * cptr->scale)
				if(AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
				{
					cptr->Phase = DIA_JUMP;
				}

		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 120) < 256)
			{
				if(!(cptr->StateF & csONWATER))
				{
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = DIA_EAT;
				}

				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:

	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->State && pdist > 1648)
		{
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 4.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}

	LookForAWay(cptr, false, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 16 + rRand(20);
	}


	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == DIA_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase && _Phase == DIA_JUMP)
	{
		cptr->Phase = DIA_RUN;
		goto ENDPSELECT;
	}

	if(cptr->Phase == DIA_JUMP)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
			if(rRand(128) > 110)
			{
				cptr->Phase = DIA_IDLE1 + rRand(1);
				goto ENDPSELECT;
			}

			else
			{
				cptr->Phase = DIA_WALK;
			}

		else
		{
			cptr->Phase = DIA_RUN;
		}

	if(cptr->Phase != DIA_IDLE1 && cptr->Phase != DIA_IDLE2)
		if(!cptr->State)
		{
			cptr->Phase = DIA_WALK;
		}

		else if(fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
		        fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
		{
			cptr->Phase = DIA_RUN;
		}

		else
		{
			cptr->Phase = DIA_WALK;
		}

	if(cptr->StateF & csONWATER)
	{
		cptr->Phase = DIA_SWIM;
	}

	if(cptr->Slide > 40)
	{
		cptr->Phase = DIA_SLIDE;
	}


ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		//==== set proportional FTime for better morphing =//
		if(g->MORPHP)
			if(_Phase <= 3 && cptr->Phase <= 3)
			{
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			}

			else if(!NewPhase)
			{
				cptr->FTime = 0;
			}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == DIA_JUMP  || cptr->Phase == DIA_EAT ||
	        cptr->Phase == DIA_IDLE1 || cptr->Phase == DIA_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 2.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->StateF & csONWATER) || cptr->Phase == DIA_WALK)
	{
		currspeed /= 1.4f;
	}

	if(cptr->AfraidTime)
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	}

	else
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);
	}

	tgbend = drspd / 3;

	if(tgbend > pi / 5)
	{
		tgbend = pi / 5;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//======= set slide mode ===========//
	if(!cptr->Slide && cptr->vspeed > 0.6 && cptr->Phase != DIA_JUMP)
		if(AngleDifference(cptr->tgalpha, cptr->alpha) > pi * 2 / 3.f)
		{
			cptr->Slide = (int)(cptr->vspeed * 700.f);
			cptr->slidex = cptr->lookx;
			cptr->slidez = cptr->lookz;
			cptr->vspeed = 0;
		}



	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == DIA_RUN)
	{
		curspeed = 0.640f * 2;
	}

	if(cptr->Phase == DIA_JUMP)
	{
		curspeed = 0.600f * 2;
	}

	//if (cptr->Phase == DIA_SWIM) curspeed = 0.300*2;
	if(cptr->Phase == DIA_WALK)
	{
		curspeed = 0.224f * 2;
	}

	if(cptr->Phase == DIA_EAT)
	{
		curspeed = 0.0f;
	}

	if(cptr->Phase == DIA_RUN && cptr->Slide)
	{
		curspeed /= 8;

		if(drspd > pi / 2.f)
		{
			curspeed = 0;
		}

		else if(drspd > pi / 4.f)
		{
			curspeed *= 2.f - 4.f * drspd / pi;
		}
	}

	else if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);

	if(cptr->Phase == DIA_JUMP)
	{
		cptr->vspeed = 1.1f;
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
	              cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, false, true);


	//========== slide ==============//
	if(cptr->Slide)
	{
		MoveCharacter(cptr, cptr->slidex * cptr->Slide / 600.f * TimeDt * cptr->scale,
		              cptr->slidez * cptr->Slide / 600.f * TimeDt * cptr->scale, false, true);

		cptr->Slide -= TimeDt;

		if(cptr->Slide < 0)
		{
			cptr->Slide = 0;
		}
	}


	//============ Y movement =================//
	if(cptr->StateF & csONWATER)
	{
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 160 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 98, 84, 0.4f, 0.3f);
	}

	//=== process to tggamma ===//
	if(cptr->Phase == DIA_WALK)
	{
		cptr->tggamma += cptr->rspeed / 9.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 6.0f;
	}

	if(cptr->Phase == DIA_JUMP)
	{
		cptr->tggamma = 0;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//

}


void AnimatePig(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(!cptr->AfraidTime)
		{
			if(pdist < 2048.f)
			{
				cptr->AfraidTime = (5 + rRand(5)) * 1024;
			}

			if(!cptr->AfraidTime)
				if(pdist > 4096.f)
				{
					cptr->State = 0;
					SetNewTargetPlace(cptr, 2048.f);
					goto TBEGIN;
				}
		}


		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}

	if(pdist > (ctViewR + 20) * 256)
		if(ReplaceCharacterForward(cptr))
		{
			goto TBEGIN;
		}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 812.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (5 + rRand(5)) * 1024;
			cptr->Phase = PIG_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 2048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);    //FindVectorAlpha(targetdx, targetdz);
	}

	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 8 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == PIG_IDLE1 || cptr->Phase == PIG_IDLE2 || cptr->Phase == PIG_IDLE3)
			{
				if(rRand(128) > 96)
				{
					cptr->Phase = PIG_WALK;
				}

				else
				{
					cptr->Phase = PIG_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = PIG_IDLE1;
			}

			else
			{
				cptr->Phase = PIG_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = PIG_RUN;
		}

		else
		{
			cptr->Phase = PIG_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == PIG_IDLE1 || cptr->Phase == PIG_IDLE2 || cptr->Phase == PIG_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.8f + drspd * 1.4f;
		}

		else
		{
			currspeed = -0.8f - drspd * 1.4f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == PIG_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 260.f);

	tgbend = drspd / 2;

	if(tgbend > pi / 2)
	{
		tgbend = pi / 2;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == PIG_RUN)
	{
		curspeed = 0.6f;
	}

	if(cptr->Phase == PIG_WALK)
	{
		curspeed = 0.2f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 64, 32, 0.7f, 0.4f);

	if(cptr->Phase == PIG_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateBron(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 16 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
			if(pdist > 12 * 256)
			{
				cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

				if(cptr->tgalpha < 0)
				{
					cptr->tgalpha += 2 * pi;
				}

				if(cptr->tgalpha > 2 * pi)
				{
					cptr->tgalpha -= 2 * pi;
				}
			}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == BRO_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				if(rRand(128) > 64 && cptr->Phase == BRO_IDLE3)
				{
					cptr->Phase = BRO_WALK;
				}

				else
				{
					cptr->Phase = BRO_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1;
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 0.768f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.168f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateBear(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 20 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == BRO_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				if(rRand(128) > 64 && cptr->Phase == BRO_IDLE3)
				{
					cptr->Phase = BRO_WALK;
				}

				else
				{
					cptr->Phase = BRO_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1;
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 0.896f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.240f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateFoot(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 40 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 800)
			if(fabs(PlayerY - cptr->pos.y - 160) < 600)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == BRO_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				if(rRand(128) > 64 && cptr->Phase == BRO_IDLE3)
				{
					cptr->Phase = BRO_WALK;
				}

				else
				{
					cptr->Phase = BRO_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1;
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 1.344f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.448f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateWolf(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		if(cptr->Phase != WLF_JMP)
		{
			NewPhase = true;
		}

		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		if(pdist > ctViewR * 140 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;

			if(cptr->AfraidTime <= 0)
			{
				cptr->AfraidTime = 0;
				cptr->State = 0;
			}
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}


	if(!(cptr->StateF & csONWATER))
		if(pdist < 1324 * cptr->scale && pdist > 900 * cptr->scale)
			if(AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
			{
				cptr->Phase = WLF_JMP;
			}


	if(pdist < 300)
		if(fabs(PlayerY - cptr->pos.y - 160) < 256)
		{
			cptr->State = 1;
			AddDeadBody(cptr, HUNT_EAT);
		}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
			if(pdist > 12 * 256)
			{
				cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

				if(cptr->tgalpha < 0)
				{
					cptr->tgalpha += 2 * pi;
				}

				if(cptr->tgalpha > 2 * pi)
				{
					cptr->tgalpha -= 2 * pi;
				}
			}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == WLF_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase && _Phase == WLF_JMP)
	{
		cptr->Phase = BRO_RUN;
		goto ENDPSELECT;
	}

	if(cptr->Phase == WLF_JMP)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				if(rRand(128) > 64)
				{
					cptr->Phase = BRO_WALK;
				}

				else
				{
					cptr->Phase = BRO_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1 + rRand(2);
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(cptr->Phase == WLF_JMP || cptr->Phase == WLF_EAT)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.4f + drspd * 1.5f;
		}

		else
		{
			currspeed = -0.4f - drspd * 1.5f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 1.024f * 2;
	}

	if(cptr->Phase == WLF_JMP)
	{
		curspeed = 1.024f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.160f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateHog(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 20 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
			if(pdist > 12 * 256)
			{
				cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

				if(cptr->tgalpha < 0)
				{
					cptr->tgalpha += 2 * pi;
				}

				if(cptr->tgalpha > 2 * pi)
				{
					cptr->tgalpha -= 2 * pi;
				}
			}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == BRO_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				if(rRand(128) > 64 && cptr->Phase == BRO_IDLE3)
				{
					cptr->Phase = BRO_WALK;
				}

				else
				{
					cptr->Phase = BRO_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1;
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.3f + drspd * 1.4f;
		}

		else
		{
			currspeed = -0.3f - drspd * 1.4f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 0.576f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.112f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateRhino(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 18 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
			if(pdist > 12 * 256)
			{
				cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

				if(cptr->tgalpha < 0)
				{
					cptr->tgalpha += 2 * pi;
				}

				if(cptr->tgalpha > 2 * pi)
				{
					cptr->tgalpha -= 2 * pi;
				}
			}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == BRO_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				if(rRand(128) > 64 && cptr->Phase == BRO_IDLE3)
				{
					cptr->Phase = BRO_WALK;
				}

				else
				{
					cptr->Phase = BRO_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1;
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.3f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.3f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 0.576f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.168f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateSmilo(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		if(cptr->Phase != WLF_JMP)
		{
			NewPhase = true;
		}

		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > ctViewR * 140 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}


	if(!(cptr->StateF & csONWATER))
		if(pdist < 1524 * cptr->scale && pdist > 900 * cptr->scale)
			if(AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
			{
				cptr->Phase = WLF_JMP;
			}

	if(pdist < 300)
		if(fabs(PlayerY - cptr->pos.y - 160) < 256)
		{
			cptr->State = 1;
			AddDeadBody(cptr, HUNT_EAT);
		}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
			if(pdist > 12 * 256)
			{
				cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

				if(cptr->tgalpha < 0)
				{
					cptr->tgalpha += 2 * pi;
				}

				if(cptr->tgalpha > 2 * pi)
				{
					cptr->tgalpha -= 2 * pi;
				}
			}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == WLF_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase && _Phase == WLF_JMP)
	{
		cptr->Phase = BRO_RUN;
		goto ENDPSELECT;
	}

	if(cptr->Phase == WLF_JMP)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
			{
				cptr->Phase = BRO_WALK;
				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = BRO_IDLE1 + rRand(2);
			}

			else
			{
				cptr->Phase = BRO_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = BRO_RUN;
		}

		else
		{
			cptr->Phase = BRO_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == BRO_IDLE1 || cptr->Phase == BRO_IDLE2 || cptr->Phase == BRO_IDLE3)
	{
		goto SKIPROT;
	}

	if(cptr->Phase == WLF_EAT   || cptr->Phase == WLF_JMP)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.3f + drspd * 1.5f;
		}

		else
		{
			currspeed = -0.3f - drspd * 1.5f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == BRO_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRO_RUN)
	{
		curspeed = 1.024f * 2;
	}

	if(cptr->Phase == WLF_JMP)
	{
		curspeed = 1.024f * 2;
	}

	if(cptr->Phase == BRO_WALK)
	{
		curspeed = 0.320f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == BRO_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateDeer(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 4000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 10 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 500)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 32 + rRand(60);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == DER_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == DER_IDLE1 || cptr->Phase == DER_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == DER_IDLE2)
				{
					cptr->Phase = DER_WALK;
				}

				else
				{
					cptr->Phase = DER_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = DER_IDLE1;
			}

			else
			{
				cptr->Phase = DER_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = DER_RUN;
		}

		else
		{
			cptr->Phase = DER_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == DER_IDLE1 || cptr->Phase == DER_IDLE2 || cptr->Phase == DER_EAT)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == DER_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 2.f;

	if(tgbend > pi / 3.f)
	{
		tgbend = pi / 3.f;
	}

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 2000.f);



	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == DER_RUN)
	{
		curspeed = 0.768f * 2;
	}

	if(cptr->Phase == DER_WALK)
	{
		curspeed = 0.160f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == DER_WALK)
	{
		cptr->tggamma += cptr->rspeed / 16.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 10.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateMamm(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//
	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 4000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 16 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 500)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 1;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//
	if(pdist < 3048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 32 + rRand(60);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == MAM_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
		if(!cptr->State)
		{
			if(cptr->Phase == MAM_IDLE1 || cptr->Phase == MAM_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == MAM_IDLE2)
				{
					cptr->Phase = MAM_WALK;
				}

				else
				{
					cptr->Phase = MAM_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = MAM_IDLE1;
			}

			else
			{
				cptr->Phase = MAM_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = MAM_RUN;
		}

		else
		{
			cptr->Phase = MAM_WALK;
		}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == MAM_IDLE1 || cptr->Phase == MAM_IDLE2  || cptr->Phase == MAM_EAT)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == MAM_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 2.f;

	if(tgbend > pi / 3.f)
	{
		tgbend = pi / 3.f;
	}

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 2000.f);



	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == MAM_RUN)
	{
		curspeed = 0.640f * 2;
	}

	if(cptr->Phase == MAM_WALK)
	{
		curspeed = 0.320f * 2;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == MAM_WALK)
	{
		cptr->tggamma += cptr->rspeed / 16.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 10.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateDimor(TCharacter *cptr)
{
	NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(pdist > (ctViewR + 20) * 256)
		if(ReplaceCharacterForward(cptr))
		{
			goto TBEGIN;
		}


	//======== exploring area ===============//
	if(tdist < 1024)
	{
		SetNewTargetPlace(cptr, 4048.f);
		goto TBEGIN;
	}


	//============================================//


	cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(cptr->Phase == DIM_FLY)
			if(cptr->pos.y > GetLandH(cptr->pos.x, cptr->pos.z) + 2800)
			{
				cptr->Phase = DIM_FLYP;
			}

			else
			{
				;
			}

		else if(cptr->Phase == DIM_FLYP)
			if(cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z) + 1800)
			{
				cptr->Phase = DIM_FLY;
			}
	}




	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
		if((rand() & 1023) > 980)
		{
			ActivateCharacterFx(cptr);
		}

	if(_Phase != cptr->Phase)
	{
		if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}


	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;


	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 460.f);

	tgbend = drspd / 2.f;

	if(tgbend > pi / 2)
	{
		tgbend = pi / 2;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == DIM_FLY)
	{
		curspeed = 1.5f;
	}

	if(cptr->Phase == DIM_FLYP)
	{
		curspeed = 1.3f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	if(cptr->Phase == DIM_FLY)
	{
		DeltaFunc(cptr->pos.y, GetLandH(cptr->pos.x, cptr->pos.z) + 4048, TimeDt / 6.f);
	}

	else
	{
		DeltaFunc(cptr->pos.y, GetLandH(cptr->pos.x, cptr->pos.z), TimeDt / 16.f);
	}


	if(cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z) + 236)
	{
		cptr->pos.y = GetLandH(cptr->pos.x, cptr->pos.z) + 256;
	}



	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 2024.f);

	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	cptr->tggamma = cptr->rspeed / 4.0f;

	if(cptr->tggamma > pi / 6.f)
	{
		cptr->tggamma = pi / 6.f;
	}

	if(cptr->tggamma < -pi / 6.f)
	{
		cptr->tggamma = -pi / 6.f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


#else // _iceage


void AnimateRaptorDead(TCharacter *cptr)
{

	if(cptr->Phase != RAP_DIE && cptr->Phase != RAP_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = RAP_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = RAP_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateVeloDead(TCharacter *cptr)
{

	if(cptr->Phase != VEL_DIE && cptr->Phase != VEL_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = VEL_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = VEL_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateSpinDead(TCharacter *cptr)
{

	if(cptr->Phase != SPN_DIE && cptr->Phase != SPN_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = SPN_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = SPN_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateCeraDead(TCharacter *cptr)
{

	if(cptr->Phase != CER_DIE && cptr->Phase != CER_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = CER_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = CER_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateTRexDead(TCharacter *cptr)
{

	if(cptr->Phase != REX_DIE)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = REX_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 200, 196, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateMoshDead(TCharacter *cptr)
{

	if(cptr->Phase != MOS_DIE && cptr->Phase != MOS_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = MOS_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = MOS_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateDimetDead(TCharacter *cptr)
{

	if(cptr->Phase != DMT_DIE && cptr->Phase != DMT_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = DMT_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = DMT_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateGallDead(TCharacter *cptr)
{

	if(cptr->Phase != GAL_DIE && cptr->Phase != GAL_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = GAL_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = GAL_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateDimorDead(TCharacter *cptr)
{

	if(cptr->Phase != DIM_FALL && cptr->Phase != DIM_DIE)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = DIM_FALL;
		cptr->rspeed = 0;
		ActivateCharacterFx(cptr);
		return;
	}

	ProcessPrevPhase(cptr);

	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		if(cptr->Phase == DIM_DIE)
		{
			cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
		}

		else
		{
			cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		}
	}


	//======= movement ===========//
	if(cptr->Phase == DIM_DIE)
	{
		DeltaFunc(cptr->vspeed, 0, TimeDt / 400.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, 0, TimeDt / 1200.f);
	}

	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	if(cptr->Phase == DIM_FALL)
	{
		float wh = GetLandUpH(cptr->pos.x, cptr->pos.z);
		float lh = GetLandH(cptr->pos.x, cptr->pos.z);
		bool OnWater = (wh > lh);

		if(OnWater)
			if(cptr->pos.y >= wh && (cptr->pos.y + cptr->rspeed * TimeDt / 1024) < wh)
			{
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 2.0);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 2.5);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 3.0);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 3.5);
				AddWCircle(cptr->pos.x + siRand(128), cptr->pos.z + siRand(128), 3.0);
			}

		cptr->pos.y += cptr->rspeed * TimeDt / 1024;
		cptr->rspeed -= TimeDt * 2.56f;

		if(cptr->pos.y < lh)
		{
			cptr->pos.y = lh;

			if(OnWater)
			{
				AddElements(cptr->pos.x + siRand(128), lh, cptr->pos.z + siRand(128), 4, 10);
				AddElements(cptr->pos.x + siRand(128), lh, cptr->pos.z + siRand(128), 4, 10);
				AddElements(cptr->pos.x + siRand(128), lh, cptr->pos.z + siRand(128), 4, 10);
			}

			if(cptr->PPMorphTime > 128)
			{
				cptr->PrevPhase = cptr->Phase;
				cptr->PrevPFTime  = cptr->FTime;
				cptr->PPMorphTime = 0;
			}

			cptr->Phase = DIM_DIE;
			cptr->FTime = 0;
			ActivateCharacterFx(cptr);
		}
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 140, 126, 0.6f, 0.5f);
		DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
	}
}


void AnimateTricDead(TCharacter *cptr)
{

	if(cptr->Phase != TRI_DIE && cptr->Phase != TRI_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = TRI_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = TRI_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimatePacDead(TCharacter *cptr)
{

	if(cptr->Phase != PAC_DIE && cptr->Phase != PAC_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = PAC_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = PAC_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateAnkyDead(TCharacter *cptr)
{

	if(cptr->Phase != ANK_DIE && cptr->Phase != ANK_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = ANK_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = ANK_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateStegDead(TCharacter *cptr)
{

	if(cptr->Phase != STG_DIE && cptr->Phase != STG_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = STG_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = STG_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateParDead(TCharacter *cptr)
{

	if(cptr->Phase != PAR_DIE && cptr->Phase != PAR_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = PAR_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = PAR_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateIguDead(TCharacter *cptr)
{

	if(cptr->Phase != IGU_DIE && cptr->Phase != IGU_SLP)
	{
		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = cptr->Phase;
			cptr->PrevPFTime  = cptr->FTime;
			cptr->PPMorphTime = 0;
		}

		cptr->FTime = 0;
		cptr->Phase = IGU_DIE;
		ActivateCharacterFx(cptr);
	}

	else
	{
		ProcessPrevPhase(cptr);

		cptr->FTime += TimeDt;

		if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
		{
			if(Tranq)
			{
				cptr->FTime = 0;
				cptr->Phase = IGU_SLP;
				ActivateCharacterFx(cptr);
			}

			else
			{
				cptr->FTime = cptr->pinfo->Animation[cptr->Phase].AniTime - 1;
			}
		}
	}

	//======= movement ===========//
	DeltaFunc(cptr->vspeed, 0, TimeDt / 800.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	ThinkY_Beta_Gamma(cptr, 100, 96, 0.6f, 0.5f);

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1600.f);
}


void AnimateRaptor(TCharacter *cptr)
{
	Globals *g = Globals::getGlobals();

	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 100 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 100 * cptr->scale;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);

	if(cptr->State == 2)
	{
		if(cptr->Phase != RAP_JUMP)
		{
			NewPhase = true;
		}

		cptr->State = 1;
	}


	if(GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 180 * cptr->scale)
	{
		cptr->StateF |= csONWATER;
	}

	else
	{
		cptr->StateF &= (!csONWATER);
	}

	if(cptr->Phase == RAP_EAT)
	{
		goto NOTHINK;
	}

	//============================================//
	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		//==Default
		if(pdist > ctViewR * 128 + OptAgres / 4)
		{
			//==Scared
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;

			if(cptr->AfraidTime <= 0)
			{
				cptr->AfraidTime = 0;
				cptr->State = 0;
			}

		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}

		//==Survival override
		if(SurviveMode && MyHealth > 5)
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
			//if (cptr->Phase = RAP_WALK) cptr->Phase = RAP_RUN;
		}

		if(!(cptr->StateF & csONWATER))
			if(pdist < 1324 * cptr->scale && pdist > 900 * cptr->scale)
				if(AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
				{
					cptr->Phase = RAP_JUMP;
				}

		if(pdist < 256)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				if(!(cptr->StateF & csONWATER))
				{
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = RAP_EAT;
				}

				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if(!cptr->State)
	{
		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:

	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->State && pdist > 1648)
		{
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 2.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}

	LookForAWay(cptr, false, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 16 + rRand(20);
	}


	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	// == Is the Animation over?
	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == RAP_EAT)
	{
		goto ENDPSELECT;
	}

	// == Perform Running Animation if ending a Jump Animation
	if(NewPhase && _Phase == RAP_JUMP)
	{
		cptr->Phase = RAP_RUN;
		goto ENDPSELECT;
	}


	if(cptr->Phase == RAP_JUMP)
	{
		goto ENDPSELECT;
	}

	if(!cptr->State)
	{
		cptr->Phase = RAP_WALK;
	}

	else if((fabs(cptr->tgalpha - cptr->alpha) < 1.0) || (fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0))
	{
		cptr->Phase = RAP_RUN;
	}

	else
	{
		cptr->Phase = RAP_WALK;
	}

	// == Perform swimming Animation if in water
	if(cptr->StateF & csONWATER)
	{
		cptr->Phase = RAP_SWIM;
	}

	// == Perform sliding Animation if turning at high speed
	if(cptr->Slide > 40)
	{
		cptr->Phase = RAP_SLIDE;
	}


ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		//==== set proportional FTime for better morphing =//
		if(g->MORPHP)
		{
			if(_Phase <= 3 && cptr->Phase <= 3)
			{
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			}

			else if(!NewPhase)
			{
				cptr->FTime = 0;
			}
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == RAP_JUMP || cptr->Phase == RAP_EAT)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
	{
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}
	}
	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 2.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->StateF & csONWATER) || cptr->Phase == RAP_WALK)
	{
		currspeed /= 1.4f;
	}

	if(cptr->AfraidTime)
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	}

	else
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);
	}

	tgbend = drspd / 2;

	if(tgbend > pi / 5)
	{
		tgbend = pi / 5;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//======= set slide mode ===========//
	if(!cptr->Slide && cptr->vspeed > 0.6 && cptr->Phase != RAP_JUMP)
		if(AngleDifference(cptr->tgalpha, cptr->alpha) > pi * 2 / 3.f)
		{
			cptr->Slide = (int)(cptr->vspeed * 700.f);
			cptr->slidex = cptr->lookx;
			cptr->slidez = cptr->lookz;
			cptr->vspeed = 0;
		}



	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == RAP_RUN)
	{
		curspeed = 1.2f;
	}

	if(cptr->Phase == RAP_JUMP)
	{
		curspeed = 1.1f;
	}

	if(cptr->Phase == RAP_WALK)
	{
		curspeed = 0.428f;
	}

	if(cptr->Phase == RAP_SWIM)
	{
		curspeed = 0.4f;
	}

	if(cptr->Phase == RAP_EAT)
	{
		curspeed = 0.0f;
	}

	if(cptr->Phase == RAP_RUN && cptr->Slide)
	{
		curspeed /= 8;

		if(drspd > pi / 2.f)
		{
			curspeed = 0;
		}

		else if(drspd > pi / 4.f)
		{
			curspeed *= 2.f - 4.f * drspd / pi;
		}
	}

	else if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);

	if(cptr->Phase == RAP_JUMP)
	{
		cptr->vspeed = 1.1f;
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
	              cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, false, true);


	//========== slide ==============//
	if(cptr->Slide)
	{
		MoveCharacter(cptr, cptr->slidex * cptr->Slide / 600.f * TimeDt * cptr->scale,
		              cptr->slidez * cptr->Slide / 600.f * TimeDt * cptr->scale, false, true);

		cptr->Slide -= TimeDt;

		if(cptr->Slide < 0)
		{
			cptr->Slide = 0;
		}
	}


	//============ Y movement =================//
	if(cptr->StateF & csONWATER)
	{
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 200 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 64, 32, 0.5f, 0.4f);
	}

	//=== process to tggamma ===//
	if(cptr->Phase == RAP_WALK)
	{
		cptr->tggamma += cptr->rspeed / 10.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	if(cptr->Phase == RAP_JUMP)
	{
		cptr->tggamma = 0;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//

}


void AnimateVelo(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);

	if(cptr->State == 2)
	{
		if(cptr->Phase != VEL_JUMP)
		{
			NewPhase = true;
		}

		cptr->State = 1;
	}


	if(GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 140 * cptr->scale)
	{
		cptr->StateF |= csONWATER;
	}

	else
	{
		cptr->StateF &= (!csONWATER);
	}

	if(cptr->Phase == VEL_EAT)
	{
		goto NOTHINK;
	}

	//============================================//
	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		if(pdist > ctViewR * 160 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;

			if(cptr->AfraidTime <= 0)
			{
				cptr->AfraidTime = 0;
				cptr->State = 0;
			}
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}

		if(!(cptr->StateF & csONWATER))
			if(pdist < 1324 * cptr->scale && pdist > 900 * cptr->scale)
				if(AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
				{
					cptr->Phase = VEL_JUMP;
				}

		if(pdist < 256)
			if(fabs(PlayerY - cptr->pos.y - 120) < 256)
			{
				if(!(cptr->StateF & csONWATER))
				{
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = VEL_EAT;
				}

				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:

	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->State && pdist > 1648)
		{
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 2.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}

	LookForAWay(cptr, false, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 16 + rRand(20);
	}


	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == VEL_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase && _Phase == VEL_JUMP)
	{
		cptr->Phase = VEL_RUN;
		goto ENDPSELECT;
	}


	if(cptr->Phase == VEL_JUMP)
	{
		goto ENDPSELECT;
	}

	if(!cptr->State)
	{
		cptr->Phase = VEL_WALK;
	}

	else if(fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
	        fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
	{
		cptr->Phase = VEL_RUN;
	}

	else
	{
		cptr->Phase = VEL_WALK;
	}

	if(cptr->StateF & csONWATER)
	{
		cptr->Phase = VEL_SWIM;
	}

	if(cptr->Slide > 40)
	{
		cptr->Phase = VEL_SLIDE;
	}


ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		//==== set proportional FTime for better morphing =//
		if(g->MORPHP)
		{
			if(_Phase <= 3 && cptr->Phase <= 3)
			{
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			}

			else if(!NewPhase)
			{
				cptr->FTime = 0;
			}
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == VEL_JUMP || cptr->Phase == VEL_EAT)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 2.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->StateF & csONWATER) || cptr->Phase == VEL_WALK)
	{
		currspeed /= 1.4f;
	}

	if(cptr->AfraidTime)
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	}

	else
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);
	}

	tgbend = drspd / 3;

	if(tgbend > pi / 5)
	{
		tgbend = pi / 5;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//======= set slide mode ===========//
	if(!cptr->Slide && cptr->vspeed > 0.6 && cptr->Phase != VEL_JUMP)
		if(AngleDifference(cptr->tgalpha, cptr->alpha) > pi * 2 / 3.f)
		{
			cptr->Slide = (int)(cptr->vspeed * 700.f);
			cptr->slidex = cptr->lookx;
			cptr->slidez = cptr->lookz;
			cptr->vspeed = 0;
		}



	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == VEL_RUN)
	{
		curspeed = 1.2f;
	}

	if(cptr->Phase == VEL_JUMP)
	{
		curspeed = 1.1f;
	}

	if(cptr->Phase == VEL_WALK)
	{
		curspeed = 0.428f;
	}

	if(cptr->Phase == VEL_SWIM)
	{
		curspeed = 0.4f;
	}

	if(cptr->Phase == VEL_EAT)
	{
		curspeed = 0.0f;
	}

	if(cptr->Phase == VEL_RUN && cptr->Slide)
	{
		curspeed /= 8;

		if(drspd > pi / 2.f)
		{
			curspeed = 0;
		}

		else if(drspd > pi / 4.f)
		{
			curspeed *= 2.f - 4.f * drspd / pi;
		}
	}

	else if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);

	if(cptr->Phase == VEL_JUMP)
	{
		cptr->vspeed = 1.1f;
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
	              cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, false, true);


	//========== slide ==============//
	if(cptr->Slide)
	{
		MoveCharacter(cptr, cptr->slidex * cptr->Slide / 600.f * TimeDt * cptr->scale,
		              cptr->slidez * cptr->Slide / 600.f * TimeDt * cptr->scale, false, true);

		cptr->Slide -= TimeDt;

		if(cptr->Slide < 0)
		{
			cptr->Slide = 0;
		}
	}


	//============ Y movement =================//
	if(cptr->StateF & csONWATER)
	{
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 160 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 48, 24, 0.5f, 0.4f);
	}

	//=== process to tggamma ===//
	if(cptr->Phase == VEL_WALK)
	{
		cptr->tggamma += cptr->rspeed / 7.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 5.0f;
	}

	if(cptr->Phase == VEL_JUMP)
	{
		cptr->tggamma = 0;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//

}


void AnimateSpin(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);

	if(cptr->State == 2)
	{
		if(cptr->Phase != SPN_JUMP)
		{
			NewPhase = true;
		}

		cptr->State = 1;
		cptr->Phase = SPN_RUN;
	}


	if(GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 140 * cptr->scale)
	{
		cptr->StateF |= csONWATER;
	}

	else
	{
		cptr->StateF &= (!csONWATER);
	}

	if(cptr->Phase == SPN_EAT)
	{
		goto NOTHINK;
	}

	//============================================//
	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		if(pdist > ctViewR * 140 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;

			if(cptr->AfraidTime <= 0)
			{
				cptr->AfraidTime = 0;
				cptr->State = 0;
			}
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}

		if(!(cptr->StateF & csONWATER))
			if(pdist < 1324 * cptr->scale && pdist > 900 * cptr->scale)
				if(AngleDifference(cptr->alpha, FindVectorAlpha(playerdx, playerdz)) < 0.2f)
				{
					cptr->Phase = SPN_JUMP;
				}

		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 120) < 256)
			{
				if(!(cptr->StateF & csONWATER))
				{
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = SPN_EAT;
				}

				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:

	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->State && pdist > 1648)
		{
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 4.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}

	LookForAWay(cptr, false, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 16 + rRand(20);
	}


	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == SPN_EAT)
	{
		goto ENDPSELECT;
	}

	if(NewPhase && _Phase == SPN_JUMP)
	{
		cptr->Phase = SPN_RUN;
		goto ENDPSELECT;
	}

	if(cptr->Phase == SPN_JUMP)
	{
		goto ENDPSELECT;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(rRand(128) > 110)
			{
				cptr->Phase = SPN_IDLE1 + rRand(1);
				goto ENDPSELECT;
			}

			else
			{
				cptr->Phase = SPN_WALK;
			}
		}

		else
		{
			cptr->Phase = SPN_RUN;
		}
	}

	if(cptr->Phase != SPN_IDLE1 && cptr->Phase != SPN_IDLE2)
	{
		if(!cptr->State)
		{
			cptr->Phase = SPN_WALK;
		}

		else if((fabsf(cptr->tgalpha - cptr->alpha) < 1.0) || (fabsf(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0))
		{
			cptr->Phase = SPN_RUN;
		}

		else
		{
			cptr->Phase = SPN_WALK;
		}
	}

	if(cptr->StateF & csONWATER)
	{
		cptr->Phase = SPN_SWIM;
	}

	if(cptr->Slide > 40)
	{
		cptr->Phase = SPN_SLIDE;
	}


ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		//==== set proportional FTime for better morphing =//
		if(g->MORPHP)
		{
			if(_Phase <= 3 && cptr->Phase <= 3)
			{
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			}

			else if(!NewPhase)
			{
				cptr->FTime = 0;
			}
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == SPN_JUMP  || cptr->Phase == SPN_EAT ||
	        cptr->Phase == SPN_IDLE1 || cptr->Phase == SPN_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 2.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->StateF & csONWATER) || cptr->Phase == SPN_WALK)
	{
		currspeed /= 1.4f;
	}

	if(cptr->AfraidTime)
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	}

	else
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);
	}

	tgbend = drspd / 3;

	if(tgbend > pi / 5)
	{
		tgbend = pi / 5;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//======= set slide mode ===========//
	if(!cptr->Slide && cptr->vspeed > 0.6 && cptr->Phase != SPN_JUMP)
		if(AngleDifference(cptr->tgalpha, cptr->alpha) > pi * 2 / 3.f)
		{
			cptr->Slide = (int)(cptr->vspeed * 700.f);
			cptr->slidex = cptr->lookx;
			cptr->slidez = cptr->lookz;
			cptr->vspeed = 0;
		}



	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == SPN_RUN)
	{
		curspeed = 1.6f;
	}

	if(cptr->Phase == SPN_JUMP)
	{
		curspeed = 1.4f;
	}

	if(cptr->Phase == SPN_WALK)
	{
		curspeed = 0.70f;
	}

	if(cptr->Phase == SPN_SWIM)
	{
		curspeed = 0.5f;
	}

	if(cptr->Phase == SPN_EAT)
	{
		curspeed = 0.0f;
	}

	if(cptr->Phase == SPN_RUN && cptr->Slide)
	{
		curspeed /= 8;

		if(drspd > pi / 2.f)
		{
			curspeed = 0;
		}

		else if(drspd > pi / 4.f)
		{
			curspeed *= 2.f - 4.f * drspd / pi;
		}
	}

	else if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);

	if(cptr->Phase == SPN_JUMP)
	{
		cptr->vspeed = 1.1f;
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
	              cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, false, true);


	//========== slide ==============//
	if(cptr->Slide)
	{
		MoveCharacter(cptr, cptr->slidex * cptr->Slide / 600.f * TimeDt * cptr->scale,
		              cptr->slidez * cptr->Slide / 600.f * TimeDt * cptr->scale, false, true);

		cptr->Slide -= TimeDt;

		if(cptr->Slide < 0)
		{
			cptr->Slide = 0;
		}
	}


	//============ Y movement =================//
	if(cptr->StateF & csONWATER)
	{
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 160 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 98, 84, 0.4f, 0.3f);
	}

	//=== process to tggamma ===//
	if(cptr->Phase == SPN_WALK)
	{
		cptr->tggamma += cptr->rspeed / 9.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 6.0f;
	}

	if(cptr->Phase == SPN_JUMP)
	{
		cptr->tggamma = 0;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//

}


void AnimateCera(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
		cptr->Phase = CER_RUN;
	}


	if(GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z) > 140 * cptr->scale)
	{
		cptr->StateF |= csONWATER;
	}

	else
	{
		cptr->StateF &= (!csONWATER);
	}

	if(cptr->Phase == CER_EAT)
	{
		goto NOTHINK;
	}

	//============================================//
	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		if(pdist > ctViewR * 200 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
			cptr->AfraidTime -= TimeDt;

			if(cptr->AfraidTime <= 0)
			{
				cptr->AfraidTime = 0;
				cptr->State = 0;
			}
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}



		if(pdist < 350)
			if(fabs(PlayerY - cptr->pos.y - 120) < 256)
			{
				if(!(cptr->StateF & csONWATER))
				{
					cptr->vspeed /= 8.0f;
					cptr->State = 1;
					cptr->Phase = CER_EAT;
				}

				AddDeadBody(cptr, HUNT_EAT);
			}
	}

	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}

NOTHINK:

	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->State && pdist > 1648)
		{
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 4.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}

	LookForAWay(cptr, false, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 16 + rRand(20);
	}


	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);


	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == CER_EAT)
	{
		goto ENDPSELECT;
	}


	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(rRand(128) > 110)
			{
				cptr->Phase = CER_IDLE1 + rRand(2);
				goto ENDPSELECT;
			}

			else
			{
				cptr->Phase = CER_WALK;
			}
		}

		else
		{
			cptr->Phase = CER_RUN;
		}
	}

	if((cptr->Phase != (CER_IDLE1 && cptr->Phase)) != (CER_IDLE2 && (cptr->Phase != CER_IDLE3)))
	{
		if(!cptr->State)
		{
			cptr->Phase = CER_WALK;
		}

		else if(fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
		        fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
		{
			cptr->Phase = CER_RUN;
		}

		else
		{
			cptr->Phase = CER_WALK;
		}
	}

	if(cptr->StateF & csONWATER)
	{
		cptr->Phase = CER_SWIM;
	}


ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		//==== set proportional FTime for better morphing =//
		if(g->MORPHP)
		{
			if(_Phase <= 3 && cptr->Phase <= 3)
			{
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			}

			else if(!NewPhase)
			{
				cptr->FTime = 0;
			}
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == CER_IDLE1 || cptr->Phase == CER_EAT ||
	        cptr->Phase == CER_IDLE2 || cptr->Phase == CER_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 2.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->StateF & csONWATER) || cptr->Phase == CER_WALK)
	{
		currspeed /= 1.4f;
	}

	if(cptr->AfraidTime)
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 160.f);
	}

	else
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 180.f);
	}

	tgbend = drspd / 3;

	if(tgbend > pi / 5)
	{
		tgbend = pi / 5;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 600.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:




	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == CER_RUN)
	{
		curspeed = 2.2f;
	}

	if(cptr->Phase == CER_WALK)
	{
		curspeed = 0.75f;
	}

	if(cptr->Phase == CER_EAT)
	{
		curspeed = 0.0f;
	}

	//if (drspd > pi / 2.f) curspeed*=2.f - 2.f*drspd / pi;

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 500.f);


	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
	              cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, false, true);


	//============ Y movement =================//
	if(cptr->StateF & csONWATER)
	{
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 160 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 348, 324, 0.5f, 0.4f);
	}

	//=== process to tggamma ===//
	if(cptr->Phase == CER_WALK)
	{
		cptr->tggamma += cptr->rspeed / 9.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 6.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 1624.f);


	//==================================================//

}

void AnimateTRex(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	bool LookMode = false;



TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 108;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 108;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);
	float palpha = FindVectorAlpha(playerdx, playerdz);

	//if (cptr->State==2) { NewPhase=true; cptr->State=1; }
	if(cptr->State == 5)
	{
		NewPhase = true;
		cptr->State = 1;
		cptr->Phase = REX_WALK;
		cptr->FTime = 0;
		cptr->tgx = PlayerX;
		cptr->tgz = PlayerZ;
		goto TBEGIN;
	}

	float hdiff = GetLandUpH(cptr->pos.x, cptr->pos.z) - GetLandH(cptr->pos.x, cptr->pos.z);

	if(hdiff > 0)
	{
		AddWCircle(cptr->pos.x, cptr->pos.z, cptr->scale);
		g_AudioDevice->addVoice( fxStepW[ (RealTime % 3) ] );
	}

	if(hdiff > 560 * cptr->scale)
	{
		cptr->StateF |= csONWATER;
	}

	else
	{
		cptr->StateF &= (!csONWATER);
	}

	if(cptr->Phase == REX_EAT)
	{
		goto NOTHINK;
	}

	//============================================//
	if(!MyHealth)
	{
		cptr->State = 0;
	}

	if(cptr->State)
	{
		cptr->tgx = PlayerX;
		cptr->tgz = PlayerZ;
		cptr->tgtime = 0;

		if(cptr->State > 1)
			if(AngleDifference(cptr->alpha, palpha) < 0.4f)
			{
				if(cptr->State == 2)
				{
					cptr->Phase = REX_SEE1 + rRand(1);
				}

				else
				{
					cptr->Phase = REX_SMEL + rRand(1);
				}

				cptr->State = 1;
				cptr->rspeed = 0;
			}

		if(pdist < 380)
			if(fabs(PlayerY - cptr->pos.y) < 256)
			{
				cptr->vspeed /= 8.0f;
				cptr->State = 1;
				cptr->Phase = REX_EAT;
				AddDeadBody(cptr, HUNT_KILL);
				Characters[ChCount - 1].scale = cptr->scale;
				Characters[ChCount - 1].alpha = cptr->alpha;
				cptr->bend = 0;
				DemoPoint.CIndex = CurDino;
			}
	}

	if(!cptr->State)
		if(tdist < 1224)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}


NOTHINK:

	if(pdist < 2048)
	{
		cptr->NoFindCnt = 0;
	}

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->State && pdist > 5648)
		{
			cptr->tgalpha += (float)sin(RealTime / 824.f) / 6.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}

	LookForAWay(cptr, false, !cptr->State);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 16 + rRand(20);
	}


	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);


	//======== select new phase =======================//
	if(cptr->Phase == REX_SEE  || cptr->Phase == REX_SEE1 ||
	        cptr->Phase == REX_SMEL || cptr->Phase == REX_SMEL1)
	{
		LookMode = true;
	}

	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(cptr->Phase == REX_EAT)
	{
		goto ENDPSELECT;
	}

	if(!NewPhase)
		if(cptr->Phase == REX_SCREAM)
		{
			goto ENDPSELECT;
		}

	if(!cptr->State)
		if(NewPhase)
			if(rRand(128) > 110)
			{
				if(rRand(128) > 64)
				{
					cptr->Phase = REX_SEE1 + rRand(1);
				}

				else
				{
					cptr->Phase = REX_SMEL + rRand(1);
				}

				goto ENDPSELECT;
			}


	if(!NewPhase) if(LookMode)
		{
			goto ENDPSELECT;
		}

	if(cptr->State)
		if(NewPhase && LookMode)
		{
			cptr->Phase = REX_SCREAM;
			goto ENDPSELECT;
		}

	if(!cptr->State || cptr->State > 1)
	{
		cptr->Phase = REX_WALK;
	}

	else if(fabs(cptr->tgalpha - cptr->alpha) < 1.0 ||
	        fabs(cptr->tgalpha - cptr->alpha) > 2 * pi - 1.0)
	{
		cptr->Phase = REX_RUN;
	}

	else
	{
		cptr->Phase = REX_WALK;
	}

	if(cptr->StateF & csONWATER)
	{
		cptr->Phase = REX_SWIM;
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		//==== set proportional FTime for better morphing =//
		if(g->MORPHP)
		{
			if(_Phase <= 1 && cptr->Phase <= 1)
			{
				cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
			}

			else if(!NewPhase)
			{
				cptr->FTime = 0;
			}
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == REX_SCREAM || cptr->Phase == REX_EAT)
	{
		goto SKIPROT;
	}

	if(LookMode)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.7f + drspd * 1.4f;
		}

		else
		{
			currspeed = -0.7f - drspd * 1.4f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 2.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if(cptr->State)
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 440.f);
	}

	else
	{
		DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 620.f);
	}

	tgbend = drspd / 2;

	if(tgbend > pi / 6.f)
	{
		tgbend = pi / 6.f;
	}

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1800.f);




	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == REX_RUN)
	{
		curspeed = 1.0f;    //2.49f;
	}

	if(cptr->Phase == REX_WALK)
	{
		curspeed = 0.76f;
	}

	if(cptr->Phase == REX_SWIM)
	{
		curspeed = 0.70f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//

	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 200.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt * cptr->scale,
	              cptr->lookz * cptr->vspeed * TimeDt * cptr->scale, false, true);

	//============ Y movement =================//
	if(cptr->StateF & csONWATER)
	{
		cptr->pos.y = GetLandUpH(cptr->pos.x, cptr->pos.z) - 540 * cptr->scale;
		cptr->beta /= 2;
		cptr->tggamma = 0;
	}

	else
	{
		ThinkY_Beta_Gamma(cptr, 348, 324, 0.5f, 0.4f);
	}



	//=== process to tggamma ===//
	if(cptr->Phase == REX_WALK)
	{
		cptr->tggamma += cptr->rspeed / 16.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2024.f);


	//==================================================//

}


void AnimateMosh(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = af_max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(!cptr->AfraidTime)
		{
			if(pdist < 2048.f)
			{
				cptr->AfraidTime = (5 + rRand(5)) * 1024;
			}

			if(!cptr->AfraidTime)
				if(pdist > 4096.f)
				{
					cptr->State = 0;
					SetNewTargetPlace(cptr, 2048.f);
					goto TBEGIN;
				}
		}


		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}

	if(pdist > (ctViewR + 20) * 256)
		if(ReplaceCharacterForward(cptr))
		{
			goto TBEGIN;
		}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 812.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (5 + rRand(5)) * 1024;
			cptr->Phase = MOS_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 2048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);    //FindVectorAlpha(targetdx, targetdz);
	}

	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 8 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == MOS_IDLE1 || cptr->Phase == MOS_IDLE2)
			{
				if(rRand(128) > 76 && cptr->Phase == MOS_IDLE2)
				{
					cptr->Phase = MOS_WALK;
				}

				else
				{
					cptr->Phase = MOS_IDLE1 + rRand(3) / 3;
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = MOS_IDLE1;
			}

			else
			{
				cptr->Phase = MOS_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = MOS_RUN;
		}

		else
		{
			cptr->Phase = MOS_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == MOS_IDLE1 || cptr->Phase == MOS_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.8f + drspd * 1.4f;
		}

		else
		{
			currspeed = -0.8f - drspd * 1.4f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == MOS_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 260.f);

	tgbend = drspd / 2;

	if(tgbend > pi / 2)
	{
		tgbend = pi / 2;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == MOS_RUN)
	{
		curspeed = 0.6f;
	}

	if(cptr->Phase == MOS_WALK)
	{
		curspeed = 0.3f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 64, 32, 0.7f, 0.4f);

	if(cptr->Phase == MOS_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateBrahi(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

TBEGIN:
	cptr->tgtime = 0;
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);


	if(tdist < 256)
	{
		SetNewTargetPlace_Brahi(cptr, 2048.f);
		goto TBEGIN;
	}

	//============================================//
	cptr->tgalpha = FindVectorAlpha(targetdx, targetdz);

	//============================================//

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(cptr->Phase > BRA_WALK)
		{
			if(rRand(128) > 90)
			{
				cptr->Phase = BRA_WALK;
			}

			else
			{
				cptr->Phase = BRA_IDLE1 + rRand(2);
			}

			goto ENDPSELECT;
		}

		if(rRand(128) > 64)
		{
			cptr->Phase = BRA_IDLE1;
		}

		else
		{
			cptr->Phase = BRA_WALK;
		}
	}

	//cptr->Phase=BRA_WALK;

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase > BRA_WALK)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 0.2f;
		}

		else
		{
			currspeed = -0.2f - drspd * 0.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 600.f);

	tgbend = drspd / 4;

	if(tgbend > pi / 4)
	{
		tgbend = pi / 4;
	}

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 3200.f);

	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}

	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == BRA_WALK)
	{
		curspeed = 0.2f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;
	/*
	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	                   cptr->lookz * cptr->vspeed * TimeDt, true, true);
	*/

	ThinkY_Beta_Gamma(cptr, 256, 128, 0.1f, 0.2f);
	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 4048.f);
}


void AnimateDimet(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = af_max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrtf(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(!cptr->AfraidTime)
		{
			if(pdist < 2048.f)
			{
				cptr->AfraidTime = (5 + rRand(5)) * 1024;
			}

			if(!cptr->AfraidTime)
				if(pdist > 4096.f)
				{
					cptr->State = 0;
					SetNewTargetPlace(cptr, 2048.f);
					goto TBEGIN;
				}
		}


		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}

	if(pdist > (ctViewR + 20) * 256)
		if(ReplaceCharacterForward(cptr))
		{
			goto TBEGIN;
		}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 812.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (5 + rRand(5)) * 1024;
			cptr->Phase = DMT_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 2048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);
	}

	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 8 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == DMT_IDLE1 || cptr->Phase == DMT_IDLE2)
			{
				if(rRand(128) > 76 && cptr->Phase == DMT_IDLE2)
				{
					cptr->Phase = DMT_WALK;
				}

				else
				{
					cptr->Phase = DMT_IDLE1 + rRand(3) / 3;
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = DMT_IDLE1;
			}

			else
			{
				cptr->Phase = DMT_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = DMT_RUN;
		}

		else
		{
			cptr->Phase = DMT_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == DMT_IDLE1 || cptr->Phase == DMT_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.8f + drspd * 1.4f;
		}

		else
		{
			currspeed = -0.8f - drspd * 1.4f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == DMT_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 260.f);

	tgbend = drspd / 2;

	if(tgbend > pi / 2)
	{
		tgbend = pi / 2;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == DMT_RUN)
	{
		curspeed = 0.6f;
	}

	if(cptr->Phase == DMT_WALK)
	{
		curspeed = 0.3f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 64, 32, 0.7f, 0.4f);

	if(cptr->Phase == DMT_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateTric(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = af_max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	//float playerdx = PlayerX - cptr->pos.x;
	//float playerdz = PlayerZ - cptr->pos.z;

	float playerdx = PlayerX - cptr->pos.x - cptr->lookx * 300 * cptr->scale;
	float playerdz = PlayerZ - cptr->pos.z - cptr->lookz * 300 * cptr->scale;

	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(pdist < 6000)
		{
			cptr->AfraidTime = 8000;
		}

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		if(pdist > 256 * 16 + OptAgres / 8)
		{
			nv.x = playerdx;
			nv.z = playerdz;
			nv.y = 0;
			NormVector(nv, 2048.f);
			cptr->tgx = cptr->pos.x - nv.x;
			cptr->tgz = cptr->pos.z - nv.z;
			cptr->tgtime = 0;
		}

		else
		{
			cptr->tgx = PlayerX;
			cptr->tgz = PlayerZ;
			cptr->tgtime = 0;
		}
	}

	if(MyHealth)
		if(pdist < 300)
			if(fabs(PlayerY - cptr->pos.y - 160) < 256)
			{
				cptr->State = 0;
				AddDeadBody(cptr, HUNT_EAT);
			}

	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 48 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == TRI_IDLE1 || cptr->Phase == TRI_IDLE2 || cptr->Phase == TRI_IDLE3)
			{
				if(rRand(128) > 64 && cptr->Phase == TRI_IDLE3)
				{
					cptr->Phase = TRI_WALK;
				}

				else
				{
					cptr->Phase = TRI_IDLE1 + rRand(2);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 124)
			{
				cptr->Phase = TRI_IDLE1;
			}

			else
			{
				cptr->Phase = TRI_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = TRI_RUN;
		}

		else
		{
			cptr->Phase = TRI_WALK;
		}
	}
ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 1 && cptr->Phase <= 1)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == TRI_IDLE1 || cptr->Phase == TRI_IDLE2 || cptr->Phase == TRI_IDLE3)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == TRI_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.5f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == TRI_RUN)
	{
		curspeed = 1.2f;
	}

	if(cptr->Phase == TRI_WALK)
	{
		curspeed = 0.30f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.3f);

	if(cptr->Phase == MOS_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimatePac(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = std::max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 1024.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = PAC_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 6048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 32 + rRand(60);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == PAC_IDLE1 || cptr->Phase == PAC_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == PAC_IDLE2)
				{
					cptr->Phase = PAC_WALK;
				}

				else
				{
					cptr->Phase = PAC_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = PAC_IDLE1;
			}

			else
			{
				cptr->Phase = PAC_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = PAC_RUN;
		}

		else
		{
			cptr->Phase = PAC_WALK;
		}
	}
ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == PAC_IDLE1 || cptr->Phase == PAC_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == PAC_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.0f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == PAC_RUN)
	{
		curspeed = 1.6f;
	}

	if(cptr->Phase == PAC_WALK)
	{
		curspeed = 0.40f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == PAC_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateAnky(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = std::max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 1024.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = ANK_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 32 + rRand(60);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == ANK_IDLE1 || cptr->Phase == ANK_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == ANK_IDLE2)
				{
					cptr->Phase = ANK_WALK;
				}

				else
				{
					cptr->Phase = ANK_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = ANK_IDLE1;
			}

			else
			{
				cptr->Phase = ANK_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = ANK_RUN;
		}

		else
		{
			cptr->Phase = ANK_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == ANK_IDLE1 || cptr->Phase == ANK_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == ANK_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 2.f;

	if(tgbend > pi / 3.f)
	{
		tgbend = pi / 3.f;
	}

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 2000.f);



	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == ANK_RUN)
	{
		curspeed = 0.75f;
	}

	if(cptr->Phase == ANK_WALK)
	{
		curspeed = 0.28f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == ANK_WALK)
	{
		cptr->tggamma += cptr->rspeed / 16.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 10.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateSteg(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = af_max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 1024.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = STG_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 12)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 32 + rRand(60);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == STG_IDLE1 || cptr->Phase == STG_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == STG_IDLE2)
				{
					cptr->Phase = STG_WALK;
				}

				else
				{
					cptr->Phase = STG_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = STG_IDLE1;
			}

			else
			{
				cptr->Phase = STG_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = STG_RUN;
		}

		else
		{
			cptr->Phase = STG_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == STG_IDLE1 || cptr->Phase == STG_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == STG_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 2.f;

	if(tgbend > pi / 3.f)
	{
		tgbend = pi / 3.f;
	}

	tgbend *= SGN(currspeed);
	DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 2000.f);



	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == STG_RUN)
	{
		curspeed = 0.96f;
	}

	if(cptr->Phase == STG_WALK)
	{
		curspeed = 0.36f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == STG_WALK)
	{
		cptr->tggamma += cptr->rspeed / 16.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 10.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimatePar(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = af_max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 1024.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = PAR_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 44 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == PAR_IDLE1 || cptr->Phase == PAR_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == PAR_IDLE2)
				{
					cptr->Phase = PAR_WALK;
				}

				else
				{
					cptr->Phase = PAR_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = PAR_IDLE1;
			}

			else
			{
				cptr->Phase = PAR_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = PAR_RUN;
		}

		else
		{
			cptr->Phase = PAR_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == PAR_IDLE1 || cptr->Phase == PAR_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == PAR_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.0f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == PAR_RUN)
	{
		curspeed = 1.6f;
	}

	if(cptr->Phase == PAR_WALK)
	{
		curspeed = 0.40f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == PAR_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateIgu(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = af_max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrtf(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrtf(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{

		if(!cptr->AfraidTime)
		{
			cptr->State = 0;
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 1024.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = IGU_RUN;
			goto TBEGIN;
		}


		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

		if(cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime / 1024.f) / 3.f;

			if(cptr->tgalpha < 0)
			{
				cptr->tgalpha += 2 * pi;
			}

			if(cptr->tgalpha > 2 * pi)
			{
				cptr->tgalpha -= 2 * pi;
			}
		}
	}


	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 44 + rRand(80);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == PAR_IDLE1 || cptr->Phase == IGU_IDLE2)
			{
				if(rRand(128) > 64 && cptr->Phase == IGU_IDLE2)
				{
					cptr->Phase = IGU_WALK;
				}

				else
				{
					cptr->Phase = IGU_IDLE1 + rRand(1);
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = IGU_IDLE1;
			}

			else
			{
				cptr->Phase = IGU_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = IGU_RUN;
		}

		else
		{
			cptr->Phase = IGU_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(cptr->Phase == IGU_IDLE1 || cptr->Phase == IGU_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
	{
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.2f + drspd * 1.0f;
		}

		else
		{
			currspeed = -0.2f - drspd * 1.0f;
		}
	}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == IGU_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

	tgbend = drspd / 3.0f;

	if(tgbend > pi / 2.f)
	{
		tgbend = pi / 2.f;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);
	}


	rspd = cptr->rspeed * TimeDt / 612.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == IGU_RUN)
	{
		curspeed = 1.6f;
	}

	if(cptr->Phase == IGU_WALK)
	{
		curspeed = 0.40f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;

	if(curspeed > cptr->vspeed)
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
	}

	else
	{
		DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);
	}

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);

	if(cptr->Phase == PAR_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateGall(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;

	if(cptr->AfraidTime)
	{
		cptr->AfraidTime = std::max(0, cptr->AfraidTime - TimeDt);
	}

	if(cptr->State == 2)
	{
		NewPhase = true;
		cptr->State = 1;
	}

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(cptr->State)
	{
		if(!cptr->AfraidTime)
		{
			if(pdist < 2048.f)
			{
				cptr->State = 1;
				cptr->AfraidTime = (5 + rRand(5)) * 1024;
			}

			if(pdist > 4096.f)
			{
				cptr->State = 0;
				SetNewTargetPlace(cptr, 2048.f);
				goto TBEGIN;
			}
		}

		nv.x = playerdx;
		nv.z = playerdz;
		nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}

	if(pdist > (ctViewR + 20) * 256)
		if(ReplaceCharacterForward(cptr))
		{
			goto TBEGIN;
		}



	//======== exploring area ===============//
	if(!cptr->State)
	{
		cptr->AfraidTime = 0;

		if(pdist < 812.f)
		{
			cptr->State = 1;
			cptr->AfraidTime = (5 + rRand(5)) * 1024;
			cptr->Phase = MOS_RUN;
			goto TBEGIN;
		}

		if(tdist < 456)
		{
			SetNewTargetPlace(cptr, 2048.f);
			goto TBEGIN;
		}
	}


	//============================================//

	if(cptr->NoFindCnt > 0)
	{
		cptr->NoFindCnt--;
	}

	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);    //FindVectorAlpha(targetdx, targetdz);
	}

	LookForAWay(cptr, true, true);

	if(cptr->NoWayCnt > 8)
	{
		cptr->NoWayCnt = 0;
		cptr->NoFindCnt = 8 + rRand(40);
	}

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(!cptr->State)
		{
			if(cptr->Phase == GAL_IDLE1 || cptr->Phase == GAL_IDLE2)
			{
				if(rRand(128) > 76 && cptr->Phase == GAL_IDLE2)
				{
					cptr->Phase = GAL_WALK;
				}

				else
				{
					cptr->Phase = GAL_IDLE1 + rRand(3) / 3;
				}

				goto ENDPSELECT;
			}

			if(rRand(128) > 120)
			{
				cptr->Phase = GAL_IDLE1;
			}

			else
			{
				cptr->Phase = GAL_WALK;
			}
		}

		else if(cptr->AfraidTime)
		{
			cptr->Phase = GAL_RUN;
		}

		else
		{
			cptr->Phase = GAL_WALK;
		}
	}

ENDPSELECT:

	//====== process phase changing ===========//

	if((_Phase != cptr->Phase) || NewPhase)
	{
		ActivateCharacterFx(cptr);
	}

	if(_Phase != cptr->Phase)
	{
		if(_Phase <= 2 && cptr->Phase <= 2)
		{
			cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
		}

		else if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}

	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}

	if(cptr->Phase == GAL_IDLE1 || cptr->Phase == GAL_IDLE2)
	{
		goto SKIPROT;
	}

	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.8f + drspd * 1.4f;
		}

		else
		{
			currspeed = -0.8f - drspd * 1.4f;
		}

	else
	{
		currspeed = 0;
	}

	if(cptr->AfraidTime)
	{
		currspeed *= 1.5;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	if((cptr->State & csONWATER) || cptr->Phase == GAL_WALK)
	{
		currspeed /= 1.4f;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 260.f);

	tgbend = drspd / 3;

	if(tgbend > pi / 2)
	{
		tgbend = pi / 2;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

SKIPROT:

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == GAL_RUN)
	{
		curspeed = 0.9f;
	}

	if(cptr->Phase == GAL_WALK)
	{
		curspeed = 0.32f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);

	MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	              cptr->lookz * cptr->vspeed * TimeDt, true, true);

	ThinkY_Beta_Gamma(cptr, 64, 32, 0.7f, 0.4f);

	if(cptr->Phase == GAL_WALK)
	{
		cptr->tggamma += cptr->rspeed / 12.0f;
	}

	else
	{
		cptr->tggamma += cptr->rspeed / 8.0f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


void AnimateDimor(TCharacter *cptr)
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;


TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	float tdist = (float)sqrt(targetdx * targetdx + targetdz * targetdz);

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;
	float pdist = (float)sqrt(playerdx * playerdx + playerdz * playerdz);


	//=========== run away =================//

	if(pdist > (ctViewR + 20) * 256)
		if(ReplaceCharacterForward(cptr))
		{
			goto TBEGIN;
		}


	//======== exploring area ===============//
	if(tdist < 1024)
	{
		SetNewTargetPlace(cptr, 4048.f);
		goto TBEGIN;
	}


	//============================================//


	cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);//FindVectorAlpha(targetdx, targetdz);

	if(cptr->tgalpha < 0)
	{
		cptr->tgalpha += 2 * pi;
	}

	if(cptr->tgalpha > 2 * pi)
	{
		cptr->tgalpha -= 2 * pi;
	}

	//===============================================//

	ProcessPrevPhase(cptr);

	//======== select new phase =======================//
	cptr->FTime += TimeDt;

	if(cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if(NewPhase)
	{
		if(cptr->Phase == DIM_FLY)
			if(cptr->pos.y > GetLandH(cptr->pos.x, cptr->pos.z) + 2800)
			{
				cptr->Phase = DIM_FLYP;
			}

			else
			{
				;
			}

		else if(cptr->Phase == DIM_FLYP)
			if(cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z) + 1800)
			{
				cptr->Phase = DIM_FLY;
			}
	}




	//====== process phase changing ===========//
	if((_Phase != cptr->Phase) || NewPhase)
		if(((rand()) & 1023) > 980)
		{
			ActivateCharacterFx(cptr);
		}

	if(_Phase != cptr->Phase)
	{
		if(!NewPhase)
		{
			cptr->FTime = 0;
		}

		if(cptr->PPMorphTime > 128)
		{
			cptr->PrevPhase = _Phase;
			cptr->PrevPFTime  = _FTime;
			cptr->PPMorphTime = 0;
		}
	}


	cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;


	//========== rotation to tgalpha ===================//

	float rspd, currspeed, tgbend;
	float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
	float drspd = dalpha;

	if(drspd > pi)
	{
		drspd = 2 * pi - drspd;
	}


	if(drspd > 0.02)
		if(cptr->tgalpha > cptr->alpha)
		{
			currspeed = 0.6f + drspd * 1.2f;
		}

		else
		{
			currspeed = -0.6f - drspd * 1.2f;
		}

	else
	{
		currspeed = 0;
	}

	if(dalpha > pi)
	{
		currspeed *= -1;
	}

	DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 460.f);

	tgbend = drspd / 2.f;

	if(tgbend > pi / 2)
	{
		tgbend = pi / 2;
	}

	tgbend *= SGN(currspeed);

	if(fabs(tgbend) > fabs(cptr->bend))
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 800.f);
	}

	else
	{
		DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 400.f);
	}


	rspd = cptr->rspeed * TimeDt / 1024.f;

	if(drspd < fabs(rspd))
	{
		cptr->alpha = cptr->tgalpha;
	}

	else
	{
		cptr->alpha += rspd;
	}


	if(cptr->alpha > pi * 2)
	{
		cptr->alpha -= pi * 2;
	}

	if(cptr->alpha < 0)
	{
		cptr->alpha += pi * 2;
	}

	//========== movement ==============================//
	cptr->lookx = (float)cos(cptr->alpha);
	cptr->lookz = (float)sin(cptr->alpha);

	float curspeed = 0;

	if(cptr->Phase == DIM_FLY)
	{
		curspeed = 1.5f;
	}

	if(cptr->Phase == DIM_FLYP)
	{
		curspeed = 1.3f;
	}

	if(drspd > pi / 2.f)
	{
		curspeed *= 2.f - 2.f * drspd / pi;
	}

	if(cptr->Phase == DIM_FLY)
	{
		DeltaFunc(cptr->pos.y, GetLandH(cptr->pos.x, cptr->pos.z) + 4048, TimeDt / 6.f);
	}

	else
	{
		DeltaFunc(cptr->pos.y, GetLandH(cptr->pos.x, cptr->pos.z), TimeDt / 16.f);
	}


	if(cptr->pos.y < GetLandH(cptr->pos.x, cptr->pos.z) + 236)
	{
		cptr->pos.y = GetLandH(cptr->pos.x, cptr->pos.z) + 256;
	}



	//========== process speed =============//
	curspeed *= cptr->scale;
	DeltaFunc(cptr->vspeed, curspeed, TimeDt / 2024.f);

	cptr->pos.x += cptr->lookx * cptr->vspeed * TimeDt;
	cptr->pos.z += cptr->lookz * cptr->vspeed * TimeDt;

	cptr->tggamma = cptr->rspeed / 4.0f;

	if(cptr->tggamma > pi / 6.f)
	{
		cptr->tggamma = pi / 6.f;
	}

	if(cptr->tggamma < -pi / 6.f)
	{
		cptr->tggamma = -pi / 6.f;
	}

	DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}


#endif // _iceage



void AnimateCharacters()
{
	if(bDevShow)
	{
		return;
	}

	//if (!RunMode) return;
	TCharacter *cptr;

	if(TrophyMode)
	{
		return;
	}

	for(CurDino = 0; CurDino < ChCount; CurDino++)
	{
		cptr = &Characters[CurDino];

		if(cptr->StateF == 0xFF)
		{
			continue;
		}

		cptr->tgtime += TimeDt;

		if(cptr->tgtime > 30 * 1000)
		{
			SetNewTargetPlace(cptr, 2048);
		}

		if(cptr->Health)
			if(cptr->BloodTTime)
			{
				cptr->BloodTTime -= TimeDt;

				if(cptr->BloodTTime < 0)
				{
					cptr->BloodTTime = 0;
				}

				float k = (20000.f + cptr->BloodTTime) / 90000.f;

				if(k > 1.5)
				{
					k = 1.5;
				}

				cptr->BloodTime += (int)((float)TimeDt * k);

				if(cptr->BloodTime > 600)
				{
					cptr->BloodTime = rRand(228);
					AddBloodTrail(cptr);

					if(rRand(128) > 96)
					{
						AddBloodTrail(cptr);
					}
				}
			}


		switch(cptr->AI)
		{

#ifdef _iceage // alacn

			case AI_PIG    :

				if(cptr->Health)
				{
					AnimatePig(cptr);
				}

				else
				{
					AnimatePigDead(cptr);
				}

				break;
			case AI_ARCHEO :

				if(cptr->Health)
				{
					AnimateDimor(cptr);
				}

				else
				{
					AnimateDimorDead(cptr);
				}

				break;


			case AI_BRONT:

				if(cptr->Health)
				{
					AnimateBron(cptr);
				}

				else
				{
					AnimateBronDead(cptr);
				}

				break;
			case AI_HOG  :

				if(cptr->Health)
				{
					AnimateHog(cptr);
				}

				else
				{
					AnimateHogDead(cptr);
				}

				break;
			case AI_WOLF :

				if(cptr->Health)
				{
					AnimateWolf(cptr);
				}

				else
				{
					AnimateBronDead(cptr);
				}

				break;
			case AI_RHINO:

				if(cptr->Health)
				{
					AnimateRhino(cptr);
				}

				else
				{
					AnimateRhinoDead(cptr);
				}

				break;
			case AI_DIATR:

				if(cptr->Health)
				{
					AnimateDiatr(cptr);
				}

				else
				{
					AnimateDiatrDead(cptr);
				}

				break;
			case AI_DEER :

				if(cptr->Health)
				{
					AnimateDeer(cptr);
				}

				else
				{
					AnimateDeerDead(cptr);
				}

				break;
			case AI_SMILO:

				if(cptr->Health)
				{
					AnimateSmilo(cptr);
				}

				else
				{
					AnimateSmiloDead(cptr);
				}

				break;
			case AI_MAMM :

				if(cptr->Health)
				{
					AnimateMamm(cptr);
				}

				else
				{
					AnimateDeerDead(cptr);
				}

				break;
			case AI_BEAR :

				if(cptr->Health)
				{
					AnimateBear(cptr);
				}

				else
				{
					AnimateBronDead(cptr);
				}

				break;
			case AI_BIGFOOT:

				if(cptr->Health)
				{
					AnimateFoot(cptr);
				}

				else
				{
					AnimateBronDead(cptr);
				}

				break;

#else // _iceage

			case AI_MOSH :

				if(cptr->Health)
				{
					AnimateMosh(cptr);
				}

				else
				{
					AnimateMoshDead(cptr);
				}

				break;
			case AI_GALL :

				if(cptr->Health)
				{
					AnimateGall(cptr);
				}

				else
				{
					AnimateGallDead(cptr);
				}

				break;
			case AI_DIMOR:

				if(cptr->Health)
				{
					AnimateDimor(cptr);
				}

				else
				{
					AnimateDimorDead(cptr);
				}

				break;
			case AI_PTERA:

				if(cptr->Health)
				{
					AnimateDimor(cptr);
				}

				else
				{
					AnimateDimorDead(cptr);
				}

				break;
			case AI_DIMET:

				if(cptr->Health)
				{
					AnimateDimet(cptr);
				}

				else
				{
					AnimateDimetDead(cptr);
				}

				break;
			case AI_BRACH:
				AnimateBrahi(cptr);
				break;


			case AI_PARA :

				if(cptr->Health)
				{
					AnimatePar(cptr);
				}

				else
				{
					AnimateParDead(cptr);
				}

				break;
			case AI_ANKY :

				if(cptr->Health)
				{
					AnimateAnky(cptr);
				}

				else
				{
					AnimateAnkyDead(cptr);
				}

				break;
			case AI_STEGO:

				if(cptr->Health)
				{
					AnimateSteg(cptr);
				}

				else
				{
					AnimateStegDead(cptr);
				}

				break;

			case AI_ALLO :

				if(cptr->Health)
				{
					AnimateRaptor(cptr);
				}

				else
				{
					AnimateRaptorDead(cptr);
				}

				break;
			case AI_CHASM:

				if(cptr->Health)
				{
					AnimateTric(cptr);
				}

				else
				{
					AnimateTricDead(cptr);
				}

				break;

			case AI_PACH:

				if(cptr->Health)
				{
					AnimatePac(cptr);
				}

				else
				{
					AnimatePacDead(cptr);
				}

				break;

			case AI_VELO :

				if(cptr->Health)
				{
					AnimateVelo(cptr);
				}

				else
				{
					AnimateVeloDead(cptr);
				}

				break;
			case AI_SPINO:

				if(cptr->Health)
				{
					AnimateSpin(cptr);
				}

				else
				{
					AnimateSpinDead(cptr);
				}

				break;
			case AI_CERAT:

				if(cptr->Health)
				{
					AnimateCera(cptr);
				}

				else
				{
					AnimateCeraDead(cptr);
				}

				break;
			case AI_TREX :

				if(cptr->Health)
				{
					AnimateTRex(cptr);
				}

				else
				{
					AnimateTRexDead(cptr);
				}

				break;

			case AI_IGU :

				if(cptr->Health)
				{
					AnimateIgu(cptr);
				}

				else
				{
					AnimateIguDead(cptr);
				}

				break;

#endif // _iceage

			case 0       :
				AnimateHuntDead(cptr);
				break;
		}
	}
}



/*
    MakeNoise( Position, Range )
    Make a noise that animals can hear and react to
*/
void MakeNoise(vec3 pos, float range)
{
	for(int c = 0; c < ChCount; c++)
	{
		TCharacter *cptr = &Characters[c];

		/* If we are dead, skip the code */
		if(!cptr->Health)
		{
			continue;
		}

		/* Something to do with range, earshot maybe */
		float l = VectorLength(SubVectors(cptr->pos, pos));

		/* If we are out of range, ignore */
		if(l > range)
		{
			continue;
		}

#ifndef _iceage // alacn

		if(cptr->AI == AI_TREX)   //===== T-Rex
		{
			/* TRex always goes to state 2 if in state 0 */
			if(!cptr->State)
			{
				cptr->State = 2;
			}
		}

		else
		{
#endif
			/* All animals except TRex do this */
			cptr->AfraidTime = (int)(10.f + (range - l) / 256.f) * 1024;
			cptr->State = 2;
			cptr->NoFindCnt = 0;

#ifndef _iceage // alacn
		}

#endif
	}
}


void CheckAfraid()
{
	/* If the player is dead, then ignore */
	if(!MyHealth)
	{
		return;
	}

	/* If we are in trophy mode, ignore */
	if(TrophyMode)
	{
		return;
	}

	vec3 ppos, plook, clook, wlook, rlook;
	ppos = PlayerPos;

	/* IF we are in debug mode, player is underwater or in observer mode, ignore */
	if(g->DEBUG || g->UNDERWATER || ObservMode)
	{
		return;
	}

	plook.y = 0;
	plook.x = (float) sinf(CameraAlpha);
	plook.z = (float) - cosf(CameraAlpha);

	wlook = Wind.nv;

	float kR, kwind, klook, kstand;

	float kmask  = 1.0f;
	//float kskill = 1.0f;
	float kscent = 1.0f;

	if(CamoMode)
	{
		kmask *= 1.5;
	}

	if(ScentMode)
	{
		kscent *= 1.5;
	}

	for(int c = 0; c < ChCount; c++)
	{
		TCharacter *cptr = &Characters[c];

		if(!cptr->Health)
		{
			continue;
		}

		if(cptr->AI < 10)
		{
			continue;
		}

		if(cptr->AfraidTime || cptr->State == 1)
		{
			continue;
		}

		rlook = SubVectors(ppos, cptr->pos);
		kR = VectorLength(rlook) / 256.f / (32.f + ctViewR / 2);
		NormVector(rlook, 1.0f);

		kR *= 2.5f / (float)(1.5 + OptSens / 128.f);

		if(kR > 3.0f)
		{
			continue;
		}

		clook.x = cptr->lookx;
		clook.y = 0;
		clook.z = cptr->lookz;

		MulVectorsScal(wlook, rlook, kwind);
		kwind *= Wind.speed / 10;
		MulVectorsScal(clook, rlook, klook);
		klook *= -1.f;

		if(HeadY > 180)
		{
			kstand = 0.7f;
		}

		else
		{
			kstand = 1.2f;
		}

		//============= reasons ==============//

		float kALook  = kR * ((klook + 3.f) / 3.f) * kstand * kmask;

		if(klook > 0.3)
		{
			kALook *= 2.0;
		}

		if(klook > 0.8)
		{
			kALook *= 2.0;
		}

		kALook /= DinoInfo[cptr->CType].LookK;

		if(kALook < 1.0)
			if(TraceLook(cptr->pos.x, cptr->pos.y + 220, cptr->pos.z, PlayerX, PlayerY + HeadY / 2.f, PlayerZ))
			{
				kALook *= 1.3f;
			}

		if(kALook < 1.0)
			if(TraceLook(cptr->pos.x, cptr->pos.y + 220, cptr->pos.z, PlayerX, PlayerY + HeadY, PlayerZ))
			{
				kALook = 2.0;
			}

		kALook *= (1.f + (float)ObjectsOnLook / 6.f);

		/*
		  if (kR<1.0f) {
		      char t[32];
		   sprintf(t,"%d", ObjectsOnLook);
		   AddMessage(t);
		   kALook = 20.f;
		  }*/


		float kASmell = kR * ((kwind + 2.0f) / 2.0F) * ((klook + 3.f) / 3.f) * kscent;

		if(kwind > 0)
		{
			kASmell *= 2.0;
		}

		kASmell /= DinoInfo[cptr->CType].SmellK;

		float kRes = std::min(kALook, kASmell);

		if(kRes < 1.0)
		{

			////MessageBeep(0xFFFFFFFF);
			char t[128];

			if(kALook < kASmell)
			{
				sprintf(t, "LOOK: KR: %f  Tr: %d  K: %f", kR, ObjectsOnLook, kALook);
			}

			else
			{
				sprintf(t, "SMELL: KR: %f  Tr: %d  K: %f", kR, ObjectsOnLook, kASmell);
			}

			AddMessage(t);


			kRes = std::min(kRes, kR);
			cptr->AfraidTime = (int)(1.0 / (kRes + 0.1) * 10.f * 1000.f);
			cptr->State = 2;
#ifndef _iceage // alacn

			if(cptr->AI == AI_TREX) //===== T-Rex
				if(kALook > kASmell)
				{
					cptr->State = 3;
				}

#endif
			cptr->NoFindCnt = 0;
		}
	}
}





void PlaceTrophy()
{
	ChCount = 0;

	for(int c = 0; c < 24; c++)
	{
		if(!TrophyRoom.Body[c].ctype)
		{
			continue;
		}

		Characters[ChCount].CType = TrophyRoom.Body[c].ctype;

		if(c < 6)
		{
			Characters[ChCount].alpha = pi / 2;
		}

		else if(c < 12)
		{
			Characters[ChCount].alpha = pi;
		}

		else if(c < 18)
		{
			Characters[ChCount].alpha = pi * 3 / 2;
		}

		else
		{
			Characters[ChCount].alpha = 0;
		}

		ResetCharacter(&Characters[ChCount]);

		Characters[ChCount].State = c;
		Characters[ChCount].scale = TrophyRoom.Body[c].scale;
		Characters[ChCount].pos.x = LandingList.list[c].x * 256.f + 128.f;
		Characters[ChCount].pos.z = LandingList.list[c].y * 256.f + 128.f;

		Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
		                                     Characters[ChCount].pos.z);
		ChCount++;
	}
}


/*
    CreateCharacter( type, x,y )
*/
void CreateCharacter(unsigned int i, float x, float z)
{
position:
	Characters[ChCount].CType = i;

	Characters[ChCount].pos.x = x;
	Characters[ChCount].pos.z = z;
	Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
	                                     Characters[ChCount].pos.z);

	if(CheckPlaceCollisionP(Characters[ChCount].pos))
	{
		goto position;
	}

	Characters[ChCount].tgx = Characters[ChCount].pos.x;
	Characters[ChCount].tgz = Characters[ChCount].pos.z;
	Characters[ChCount].tgtime = 0;

	ResetCharacter(&Characters[ChCount]);
	ChCount++;
}

/*
    PlaceCharacters()
    Spawn in the initial set of animals to hunt and the ambients
*/
void PlaceCharacters()
{
	int c, tr;
	ChCount = 0;
	PrintLog("Placing Animals...");

	fprintf(stderr, "TargetDino = %d\n", TargetDino);

	for(c = 10; c < 30; c++)
		if((TargetDino & (1 << c)) > 0)
		{
			TargetCall = c;
			break;
		}


	/******************** Place Ambients ******************/

	int MC = 5 + OptDens / 80;

	if(OptDayNight == 2)
	{
		MC /= 2;
	}

	tr = 0;

	for(c = 0; c < MC; c++)
	{
#ifdef _iceage // alacn
		Characters[ChCount].CType = 1 + c % 2;
#else
		Characters[ChCount].CType = 1 + c % 5;
#endif
replace1:
		Characters[ChCount].pos.x = PlayerX + siRand(10040);
		Characters[ChCount].pos.z = PlayerZ + siRand(10040);
		Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
		                                     Characters[ChCount].pos.z);
		tr++;

		if(tr > 10240)
		{
			break;
		}

		if(CheckPlaceCollisionP(Characters[ChCount].pos))
		{
			goto replace1;
		}

		ResetCharacter(&Characters[ChCount]);

#ifdef _iceage // alacn

		if(Characters[ChCount].AI == AI_ARCHEO)
		{
			Characters[ChCount].pos.y += 2048.f;
		}

#else

		if(Characters[ChCount].AI == AI_DIMET ||
		        Characters[ChCount].AI == AI_PTERA)
		{
			Characters[ChCount].pos.y += 2048.f;
		}

#endif

		Characters[ChCount].tgx = Characters[ChCount].pos.x;
		Characters[ChCount].tgz = Characters[ChCount].pos.z;
		Characters[ChCount].tgtime = 0;


		ChCount++;
	}


#ifndef _iceage // alacn

	//======== Brachiosaurus ============//
	tr = 0;
	int BrachCount = 3 + rRand(3);

	for(c = 0; c < 3 + BrachCount; c++)
	{
		Characters[ChCount].CType = AI_to_CIndex[AI_BRACH];
replace3:
		Characters[ChCount].pos.x = 512 * 256 + siRand(50 * 256) * 10.f;
		Characters[ChCount].pos.z = 512 * 256 + siRand(50 * 256) * 10.f;
		Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
		                                     Characters[ChCount].pos.z);
		float wy = GetLandUpH(Characters[ChCount].pos.x,
		                      Characters[ChCount].pos.z) - Characters[ChCount].pos.y;
		tr++;

		if(tr > 10240)
		{
			break;
		}

		if(wy > 380)
		{
			goto replace3;
		}

		if(wy < 220)
		{
			goto replace3;
		}

		Characters[ChCount].tgx = Characters[ChCount].pos.x;
		Characters[ChCount].tgz = Characters[ChCount].pos.z;
		Characters[ChCount].tgtime = 0;

		ResetCharacter(&Characters[ChCount]);
		ChCount++;
	}

#endif

	int TC = 0; // No idea...

	int TDi[10]; // Target Dino Array
	TDi[0] = 10;

	for(c = 10; c < 20; c++) // hard coded... getting a SIGFPE because TC is 0
	{
		if(TargetDino & (1 << c))
		{
			TDi[TC] = c;
			++TC;
		}
	}

	MC = 8 + OptDens / 30 + rRand(6); // Max Animals?
	tr = 0;                         //...

	//======== main =========//
	for(c = 0; c < MC; c++)
	{

		if((c < 4) || (!TargetDino))  // Place randomized animals on the island
		{
			Characters[ChCount].CType = AI_to_CIndex[10] + rRand(6);
		}

		else // We place the real deals
		{
#ifdef _iceage // alacn

			if(c < 10)
			{
				Characters[ChCount].CType = AI_to_CIndex[ TDi[c % TC] ];
				printf("Placing extra character [%d]\n", Characters[ChCount].CType);
			}

			else
			{
				Characters[ChCount].CType = AI_to_CIndex[ TDi[rRand(TC - 1)] ];
				printf("Placing extra character [%d]\n", Characters[ChCount].CType);
			}

#else //_carn2

			Characters[ChCount].CType = AI_to_CIndex[ TDi[c % TC] ];

#endif
			//Characters[ChCount].CType = AI_to_CIndex[ AI_BRONT+1];
			//Characters[ChCount].CType = AI_to_CIndex[10] + 7;//rRand(3);
replace2:
			Characters[ChCount].pos.x = (float)(512 * 256 + siRand(50 * 256) * 10);
			Characters[ChCount].pos.z = (float)(512 * 256 + siRand(50 * 256) * 10);
			Characters[ChCount].pos.y = GetLandH(Characters[ChCount].pos.x,
			                                     Characters[ChCount].pos.z);
			Characters[ChCount].pinfo = &ChInfo[Characters[ChCount].CType];
			tr++;

			if(tr > 10240)
			{
				break;
			}

			if(fabs(Characters[ChCount].pos.x - PlayerX) +
			        fabs(Characters[ChCount].pos.z - PlayerZ) < 256 * 40)
			{
				goto replace2;
			}

			if(CheckPlaceCollisionP(Characters[ChCount].pos))
			{
				goto replace2;
			}

			Characters[ChCount].tgx = Characters[ChCount].pos.x;
			Characters[ChCount].tgz = Characters[ChCount].pos.z;
			Characters[ChCount].tgtime = 0;

			ResetCharacter(&Characters[ChCount]);
			ChCount++;

			/* Pack Spawn */
			if(DinoInfo[ Characters[ChCount - 1].CType ].Group)
			{
				int ChPrev = ChCount - 1;
				DinoInfo[ Characters[ChPrev].CType ].GroupMinSize = 3;

				for(int i = 0; i < rand() % DinoInfo[ Characters[ChPrev].CType ].GroupMinSize; i++)
				{
					CreateCharacter(Characters[ChPrev].CType, Characters[ChPrev].pos.x, Characters[ChPrev].pos.z);
				}
			}
		}
	}

	PrintLog("\n");
	DemoPoint.DemoTime = 0;
}








void CreateChMorphedModel(TCharacter *cptr)
{
	TAni *aptr  = &cptr->pinfo->Animation[cptr->Phase];
	TAni *paptr = &cptr->pinfo->Animation[cptr->PrevPhase];

	int CurFrame, SplineD, PCurFrame, PSplineD;
	float scale = cptr->scale;

	CurFrame = ((aptr->FramesCount - 1) * cptr->FTime * 256) / aptr->AniTime;
	SplineD = CurFrame & 0xFF;
	CurFrame = (CurFrame >> 8);


	bool PMorph = (cptr->Phase != cptr->PrevPhase) && (cptr->PPMorphTime < PMORPHTIME) && (g->MORPHP);

	PCurFrame = 0; // alacn

	if(PMorph)
	{
		PCurFrame = ((paptr->FramesCount - 1) * cptr->PrevPFTime * 256) / paptr->AniTime;
		PSplineD = PCurFrame & 0xFF;
		PCurFrame = (PCurFrame >> 8);
	}



	if(!g->MORPHA)
	{
		SplineD = 0;
		PSplineD = 0;
	}

	float k1, k2, pk1, pk2, pmk1, pmk2;

	k2 = (float)(SplineD) / 256.f;
	k1 = 1.0f - k2;
	k1 /= 8.f;
	k2 /= 8.f;

	if(PMorph)
	{
		pk2 = (float)(PSplineD) / 256.f;
		pk1 = 1.0f - pk2;
		pk1 /= 8.f;
		pk2 /= 8.f;
		pmk1 = (float)cptr->PPMorphTime / PMORPHTIME;
		pmk2 = 1.f - pmk1;
	}

	int VCount = cptr->pinfo->mptr->VCount;
	int16_t *adptr  =  aptr->aniData + CurFrame * VCount * 3;
	int16_t *padptr = paptr->aniData + PCurFrame * VCount * 3;

	float sb = (float)sin(cptr->beta) * scale;
	float cb = (float)cos(cptr->beta) * scale;
	float sg = (float)sin(cptr->gamma);
	float cg = (float)cos(cptr->gamma);

	for(int v = 0; v < VCount; v++)
	{
		float x = *(adptr + v * 3 + 0) * k1 + *(adptr + (v + VCount) * 3 + 0) * k2;
		float y = *(adptr + v * 3 + 1) * k1 + *(adptr + (v + VCount) * 3 + 1) * k2;
		float z = - (*(adptr + v * 3 + 2) * k1 + * (adptr + (v + VCount) * 3 + 2) * k2);

		if(PMorph)
		{
			float px = *(padptr + v * 3 + 0) * pk1 + *(padptr + (v + VCount) * 3 + 0) * pk2;
			float py = *(padptr + v * 3 + 1) * pk1 + *(padptr + (v + VCount) * 3 + 1) * pk2;
			float pz = - (*(padptr + v * 3 + 2) * pk1 + * (padptr + (v + VCount) * 3 + 2) * pk2);

			x = x * pmk1 + px * pmk2;
			y = y * pmk1 + py * pmk2;
			z = z * pmk1 + pz * pmk2;
		}


		float zz = z;
		float xx = cg * x - sg * y;
		float yy = cg * y + sg * x;


		//float fi = (z / 400) * (cptr->bend / 1.5f);
		float fi;

		if(z > 0)
		{
			fi = z / 240.f;

			if(fi > 1.f)
			{
				fi = 1.f;
			}
		}

		else
		{
			fi = z / 380.f;

			if(fi < -1.f)
			{
				fi = -1.f;
			}
		}

		fi *= cptr->bend;

		float bendc = (float)cos(fi);
		float bends = (float)sin(fi);

		float bx = bendc * xx - bends * zz;
		float bz = bendc * zz + bends * xx;
		zz = bz;
		xx = bx;

		cptr->pinfo->mptr->gVertex[v].x = xx * scale;
		cptr->pinfo->mptr->gVertex[v].y = cb * yy - sb * zz;
		cptr->pinfo->mptr->gVertex[v].z = cb * zz + sb * yy;
	}
}



void CreateMorphedModel(TModel *mptr, TAni *aptr, int FTime, float scale)
{
	int CurFrame = ((aptr->FramesCount - 1) * FTime * 256) / aptr->AniTime;

	int SplineD = CurFrame & 0xFF;
	CurFrame = (CurFrame >> 8);

	float k2 = (float)(SplineD) / 256.f;
	float k1 = 1.0f - k2;
	k1 *= scale / 8.f;
	k2 *= scale / 8.f;

	int VCount = mptr->VCount;
	int16_t *adptr = &(aptr->aniData[CurFrame * VCount * 3]);

	for(int v = 0; v < VCount; v++)
	{
		mptr->gVertex[v].x = *(adptr + v * 3 + 0) * k1 + *(adptr + (v + VCount) * 3 + 0) * k2;
		mptr->gVertex[v].y = *(adptr + v * 3 + 1) * k1 + *(adptr + (v + VCount) * 3 + 1) * k2;
		mptr->gVertex[v].z = - *(adptr + v * 3 + 2) * k1 - *(adptr + (v + VCount) * 3 + 2) * k2;
	}
}




void CreateMorphedObject(TModel *mptr, TVTL &vtl, int FTime)
{
	int CurFrame = ((vtl.FramesCount - 1) * FTime * 256) / vtl.AniTime;

	int SplineD = CurFrame & 0xFF;
	CurFrame = (CurFrame >> 8);

	float k2 = (float)(SplineD) / 256.f;
	float k1 = 1.0f - k2;
	k1 /= 8.f;
	k2 /= 8.f;

	int VCount = mptr->VCount;
	int16_t *adptr = &(vtl.aniData[CurFrame * VCount * 3]);

	for(int v = 0; v < VCount; v++)
	{
		mptr->gVertex[v].x = *(adptr + v * 3 + 0) * k1 + *(adptr + (v + VCount) * 3 + 0) * k2;
		mptr->gVertex[v].y = *(adptr + v * 3 + 1) * k1 + *(adptr + (v + VCount) * 3 + 1) * k2;
		mptr->gVertex[v].z = - *(adptr + v * 3 + 2) * k1 - *(adptr + (v + VCount) * 3 + 2) * k2;
	}
}
