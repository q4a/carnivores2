
#ifndef __CIGUANADON_H__
#define __CIGUANADON_H__

#include "Hunt.h"
#include "AIBase.hpp"

class AIIguanodon : public AIBase
{
public:

	void Animate( TCharacter *cptr );
	void AnimateDead( TCharacter *cptr );

};

void AIIguanodon::Animate( TCharacter *cptr )
{
	bool NewPhase = false;
	int _Phase = cptr->Phase;
	int _FTime = cptr->FTime;
	float _tgalpha = cptr->tgalpha;
	if (cptr->AfraidTime) cptr->AfraidTime = max(0, cptr->AfraidTime - TimeDt);
	if (cptr->State==2) { NewPhase=true; cptr->State=1; }

TBEGIN:
	float targetx = cptr->tgx;
	float targetz = cptr->tgz;
	float targetdx = targetx - cptr->pos.x;
	float targetdz = targetz - cptr->pos.z;

	/* Distance to the target */
	float tdist = (float)sqrtf( targetdx * targetdx + targetdz * targetdz );

	float playerdx = PlayerX - cptr->pos.x;
	float playerdz = PlayerZ - cptr->pos.z;

	/* Distance to the player */
	float pdist = (float)sqrtf( playerdx * playerdx + playerdz * playerdz );


	//=========== run away =================//
	if (cptr->State)
	{
		/* If we are not afraid anymore */
		if (!cptr->AfraidTime)
		{
			cptr->State = 0;
			this->SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN;
		}

		nv.x = playerdx; nv.z = playerdz; nv.y = 0;
		NormVector(nv, 2048.f);
		cptr->tgx = cptr->pos.x - nv.x;
		cptr->tgz = cptr->pos.z - nv.z;
		cptr->tgtime = 0;
	}


	//======== exploring area ===============//
	if ( cptr->State == 0 )
	{
		/* We do not want to be afraid */
		cptr->AfraidTime = 0;

		/* If the player is less than 1024 units away */
		if (pdist<1024.f)
		{
			/* We get afraid here */
			cptr->State = 1;
			cptr->AfraidTime = (6 + rRand(8)) * 1024;
			cptr->Phase = IGU_RUN;
			goto TBEGIN;
		}

		/* If we are less than 456 units away from our target */
		if (tdist<456)
		{
			/* Set a new target location within 8048 units of us */
			this->SetNewTargetPlace(cptr, 8048.f);
			goto TBEGIN; // <- Return to the beginning of the code
		}
	}


//============================================//

	/* No Find Count */
	if ( cptr->NoFindCnt )
	{
		cptr->NoFindCnt--;
	}
	else
	{
		cptr->tgalpha = CorrectedAlpha(FindVectorAlpha(targetdx, targetdz), cptr->alpha);
		if (cptr->AfraidTime)
		{
			cptr->tgalpha += (float)sin(RealTime/1024.f) / 3.f;
			if (cptr->tgalpha < 0) cptr->tgalpha+=2*pi;
			if (cptr->tgalpha > 2*pi) cptr->tgalpha-=2*pi;
		}
	}


   LookForAWay(cptr, true, true);

   if (cptr->NoWayCnt>8)
   {
	   cptr->NoWayCnt=0;
	   cptr->NoFindCnt = 44 + rRand(80);
   }

   if (cptr->tgalpha < 0) cptr->tgalpha+=2*pi;
   if (cptr->tgalpha > 2*pi) cptr->tgalpha-=2*pi;

//===============================================//

	ProcessPrevPhase(cptr);

//======== select new phase =======================//
	cptr->FTime+=TimeDt;

	if (cptr->FTime >= cptr->pinfo->Animation[cptr->Phase].AniTime)
	{
		cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;
		NewPhase = true;
	}

	if (NewPhase && !cptr->State)
	{
		if (cptr->Phase == IGU_IDLE1 || cptr->Phase == IGU_IDLE2)
		{
			if (rRand(128) > 64 && cptr->Phase == IGU_IDLE2)
			{
				cptr->Phase = IGU_WALK;
			}
			else
			{
				cptr->Phase = IGU_IDLE1 + rRand(1);
				goto ENDPSELECT;
			}

			if (rRand(128) > 120)
			{
				cptr->Phase=IGU_IDLE1;
				//MakeNoise( cptr->pos, ctViewR*200 * 1.0f ); // <- Any Animals in the view range will hear it
			}
			else
			{
				cptr->Phase=IGU_WALK;
			}
		}
		else if (cptr->AfraidTime)
		{
			cptr->Phase = IGU_RUN;
		}
		else
		{
			cptr->Phase = IGU_WALK;
		}

ENDPSELECT:

//====== process phase changing ===========//
   if ( (_Phase != cptr->Phase) || NewPhase)
	 ActivateCharacterFx(cptr);

   if (_Phase != cptr->Phase) {
    if (_Phase<=2 && cptr->Phase<=2)
        cptr->FTime = _FTime * cptr->pinfo->Animation[cptr->Phase].AniTime / cptr->pinfo->Animation[_Phase].AniTime + 64;
    else if (!NewPhase) cptr->FTime = 0;

    if (cptr->PPMorphTime>128) {
     cptr->PrevPhase = _Phase;
     cptr->PrevPFTime  = _FTime;
     cptr->PPMorphTime = 0; }
   }

   cptr->FTime %= cptr->pinfo->Animation[cptr->Phase].AniTime;



   //========== rotation to tgalpha ===================//

   float rspd, currspeed, tgbend;
   float dalpha = (float)fabs(cptr->tgalpha - cptr->alpha);
   float drspd = dalpha; if (drspd>pi) drspd = 2*pi - drspd;


   if (cptr->Phase == IGU_IDLE1 || cptr->Phase == IGU_IDLE2) goto SKIPROT;
   if (drspd > 0.02)
    if (cptr->tgalpha > cptr->alpha) currspeed = 0.2f + drspd*1.0f;
                                else currspeed =-0.2f - drspd*1.0f;
     else currspeed = 0;

   if (cptr->AfraidTime) currspeed*=1.5;
   if (dalpha > pi) currspeed*=-1;
   if ((cptr->State & csONWATER) || cptr->Phase==IGU_WALK) currspeed/=1.4f;

   DeltaFunc(cptr->rspeed, currspeed, (float)TimeDt / 400.f);

   tgbend = drspd/3.0f;
   if (tgbend>pi/2.f) tgbend = pi/2.f;

   tgbend*= SGN(currspeed);
   if (fabs(tgbend) > fabs(cptr->bend)) DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1600.f);
                                   else DeltaFunc(cptr->bend, tgbend, (float)TimeDt / 1200.f);


   rspd=cptr->rspeed * TimeDt / 612.f;
   if (drspd < fabs(rspd)) cptr->alpha = cptr->tgalpha;
                      else cptr->alpha+=rspd;


   if (cptr->alpha > pi * 2) cptr->alpha-= pi * 2;
   if (cptr->alpha < 0     ) cptr->alpha+= pi * 2;

SKIPROT:

//========== movement ==============================//
   cptr->lookx = (float)cos(cptr->alpha);
   cptr->lookz = (float)sin(cptr->alpha);

   float curspeed = 0;
   if (cptr->Phase == IGU_RUN ) curspeed = 1.6f;
   if (cptr->Phase == IGU_WALK) curspeed = 0.40f;

   if (drspd > pi / 2.f) curspeed*=2.f - 2.f*drspd / pi;

//========== process speed =============//
   curspeed*=cptr->scale;
   if (curspeed>cptr->vspeed) DeltaFunc(cptr->vspeed, curspeed, TimeDt / 1024.f);
                         else DeltaFunc(cptr->vspeed, curspeed, TimeDt / 256.f);

   MoveCharacter(cptr, cptr->lookx * cptr->vspeed * TimeDt,
	                   cptr->lookz * cptr->vspeed * TimeDt, true, true);

   ThinkY_Beta_Gamma(cptr, 128, 64, 0.6f, 0.4f);
   if (cptr->Phase==IGU_WALK) cptr->tggamma+= cptr->rspeed / 12.0f;
                         else cptr->tggamma+= cptr->rspeed / 8.0f;
   DeltaFunc(cptr->gamma, cptr->tggamma, TimeDt / 2048.f);
}

#endif //__CIGUANADON_H__
