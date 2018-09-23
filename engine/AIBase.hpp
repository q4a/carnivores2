
#ifndef H_AIBASE_H
#define H_AIBASE_H

#include "Hunt.h"


class AIBase
{
public:

	void CheckAfraid( void );
	void LookForAWay( TCharacter *cptr, bool bWaterCheck, bool bObjectCheck );
	void SetNewTargetPlace( TCharacter* cptr, float R );

	virtual void Animate( TCharacter* cptr ) = 0;
	virtual void AnimateDead( TCharacter* cptr ) = 0;

	float   m_anger;
	float   m_hunger;
	float   m_fear;

};


void AIBase::SetNewTargetPlace( TCharacter *cptr, float R )
{
	 vec3 p;
	 int tr = 0;
replace:
     p.x = cptr->pos.x + siRand((int)R); if (p.x<512) p.x = 512; if (p.x>1018*256) p.x = 1018*256;
     p.z = cptr->pos.z + siRand((int)R); if (p.z<512) p.z = 512; if (p.z>1018*256) p.z = 1018*256;
	 p.y = GetLandH(p.x, p.z);
	 tr++;
	 if (tr<128)
	  if ( fabs(p.x - cptr->pos.x) + fabs(p.z - cptr->pos.z) < R / 2.f) goto replace;

	 R+=512;

	 if (tr<256)
	 if (CheckPlaceCollisionP(p)) goto replace;

	 cptr->tgtime = 0;
	 cptr->tgx = p.x;
	 cptr->tgz = p.z;
}

void AIBase::LookForAWay(TCharacter *cptr, bool wc, bool mc)
{
	float alpha = cptr->tgalpha;
	float dalpha = 15.f;
	float afound = alpha;
	int maxp = 16;
	int curp;

	if (!CheckPossiblePath(cptr, wc, mc)) { cptr->NoWayCnt=0; return; }

	cptr->NoWayCnt++;

	for (int i=0; i<12; i++)
	{
		cptr->tgalpha = alpha+dalpha*pi/180.f;
		curp=CheckPossiblePath(cptr, wc, mc) + (i>>1);

		if (!curp) return;
		if (curp<maxp)
		{
			maxp = curp;
			afound = cptr->tgalpha;
		}

		cptr->tgalpha = alpha-dalpha*pi/180.f;
		curp=CheckPossiblePath(cptr, wc, mc) + (i>>1);

		if (!curp) return;
		if (curp<maxp)
		{
			maxp = curp;
			afound = cptr->tgalpha;
		}

		dalpha+=15.f;
	}

	cptr->tgalpha = afound;
}

void AIBase::CheckAfraid()
{
	/* If the player is dead, then ignore */
	if (!MyHealth) return;
	/* If we are in trophy mode, ignore */
	if (TrophyMode) return;

	vec3 ppos, plook, clook, wlook, rlook;
	ppos = PlayerPos;

	/* IF we are in debug mode, player is underwater or in observer mode, ignore */
	if (DEBUG || g->UNDERWATER || ObservMode) return;

	plook.y = 0;
	plook.x = (float) sinf( CameraAlpha );
	plook.z = (float)-cosf( CameraAlpha );

	wlook = Wind.nv;

	float kR, kwind, klook, kstand;

	float kmask  = 1.0f;
	float kskill = 1.0f;
	float kscent = 1.0f;

	if (CamoMode)  kmask *=1.5;
	if (ScentMode) kscent*=1.5;

	for (int c=0; c<ChCount; c++)
	{
		TCharacter *cptr = &Characters[c];
		if (!cptr->Health) continue;
		if (cptr->AI<10) continue;
		if (cptr->AfraidTime || cptr->State==1) continue;

		rlook = SubVectors(ppos, cptr->pos);
		kR = VectorLength( rlook ) / 256.f / (32.f + ctViewR/2);
		NormVector(rlook, 1.0f);

		kR *= 2.5f / (float)(1.5+OptSens/128.f );
		if (kR > 3.0f) continue;

		clook.x = cptr->lookx;
		clook.y = 0;
		clook.z = cptr->lookz;

		MulVectorsScal(wlook, rlook, kwind);
		kwind *= Wind.speed / 10;
		MulVectorsScal(clook, rlook, klook);
		klook *= -1.f;

		if (HeadY > 180) kstand = 0.7f; else kstand = 1.2f;

		//============= reasons ==============//

		float kALook  = kR * ((klook+3.f) / 3.f) * kstand * kmask;
		if (klook>0.3) kALook*= 2.0;
		if (klook>0.8) kALook*= 2.0;

		kALook/=DinoInfo[cptr->CType].LookK;

		if (kALook < 1.0)
		if (TraceLook(cptr->pos.x, cptr->pos.y+220, cptr->pos.z, PlayerX, PlayerY+HeadY/2.f, PlayerZ) )
		{
			kALook*=1.3f;
		}

		if (kALook < 1.0)
		if (TraceLook(cptr->pos.x, cptr->pos.y+220, cptr->pos.z, PlayerX, PlayerY+HeadY, PlayerZ) )
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


	 float kASmell = kR * ((kwind+2.0f) / 2.0F) * ((klook+3.f) / 3.f) * kscent;
     if (kwind>0) kASmell*= 2.0;
	 kASmell/=DinoInfo[cptr->CType].SmellK;

	 float kRes = min (kALook, kASmell);

     if (kRes < 1.0) {

        //MessageBeep(0xFFFFFFFF);
		char t[128];
		if (kALook<kASmell)
		 sprintf(t, "LOOK: KR: %f  Tr: %d  K: %f", kR, ObjectsOnLook, kALook);
		else
	     sprintf(t, "SMELL: KR: %f  Tr: %d  K: %f", kR, ObjectsOnLook, kASmell);
		AddMessage(t);


		kRes = min (kRes, kR);
		cptr->AfraidTime = (int)(1.0 / (kRes+0.1) * 10.f * 1000.f);
		cptr->State = 2;
#ifndef _iceage // alacn
		if (cptr->AI==AI_TREX) //===== T-Rex
			if (kALook>kASmell) cptr->State=3;
#endif
		cptr->NoFindCnt = 0;
	 }
   }
}

#endif
