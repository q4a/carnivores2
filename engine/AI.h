
#ifndef __AI_H__
#define __AI_H__


enum EAnimalAI
{
	AI_HUNTER	= 0,

	AI_MOSH		= 1,
	AI_GALL		= 2,
	AI_DIMOR	= 3,
	AI_PTERA	= 4,
	AI_DIMET	= 5,
	AI_BRACH	= 6,
	AI_COMPY	= 7,
	AI_FISH		= 8,

	AI_PARA		= 10,
	AI_ANKY		= 11,
	AI_STEGO	= 12,
	AI_ALLO		= 13,
	AI_CHASM	= 14,
	AI_VELO		= 15,
	AI_SPINO	= 16,
	AI_CERAT	= 17,
	AI_TREX		= 18,
	AI_PACH		= 19,
	AI_CARNO	= 20, //Not Done
	AI_IGU		= 21, //Not Done
	AI_DILO		= 22, //Not Done
	AI_UTAH		= 23, //Not Done
	AI_AMARGA	= 24, //Not Done
	AI_TROOD	= 25, //Not Done
	AI_OVI		= 26, //Not Done

	AI_PIG		= 100,
	AI_ARCHEO	= 101,

	AI_BRONT	= 102,
	AI_HOG		= 103,
	AI_WOLF		= 104,
	AI_RHINO	= 105,
	AI_DIATR	= 106,
	AI_DEER		= 107,
	AI_SMILO	= 108,
	AI_MAMM		= 109,
	AI_BEAR		= 110,
	AI_BIGFOOT	= 110,
};


enum EAllosaurState
{
	RAP_RUN    = 0,
	RAP_WALK   = 1,
	RAP_SWIM   = 2,
	RAP_SLIDE  = 3,
	RAP_JUMP   = 4,
	RAP_DIE    = 5,
	RAP_EAT    = 6,
	RAP_SLP    = 7,
};

enum ETyrannosaurState
{
	REX_RUN    = 0,
	REX_WALK   = 1,
	REX_SCREAM = 2,
	REX_SWIM   = 3,
	REX_SEE    = 4,
	REX_SEE1   = 5,
	REX_SMEL   = 6,
	REX_SMEL1  = 7,
	REX_DIE    = 8,
	REX_EAT    = 9,
	REX_SLP    = 10,
};

enum EVelociState
{
	VEL_RUN    = 0,
	VEL_WALK   = 1,
	VEL_SWIM   = 3,
	VEL_SLIDE  = 2,
	VEL_JUMP   = 4,
	VEL_DIE    = 5,
	VEL_EAT    = 6,
	VEL_SLP    = 7,
};

enum ESpinosaurState
{
	SPN_RUN    = 0,
	SPN_WALK   = 1,
	SPN_SLIDE  = 2,
	SPN_IDLE1  = 3,
	SPN_IDLE2  = 4,
	SPN_JUMP   = 5,
	SPN_DIE    = 6,
	SPN_EAT    = 7,
	SPN_SLP    = 8,
	SPN_SWIM   = 1,
};

enum ECeratosaurState
{
	CER_WALK   = 0,
	CER_RUN    = 1,
	CER_IDLE1  = 2,
	CER_IDLE2  = 3,
	CER_IDLE3  = 4,
	CER_DIE    = 5,
	CER_SLP    = 6,
	CER_EAT    = 7,
	CER_SWIM   = 0,
};

enum EMoshopsState
{
	MOS_RUN    = 0,
	MOS_WALK   = 1,
	MOS_DIE    = 2,
	MOS_IDLE1  = 3,
	MOS_IDLE2  = 4,
	MOS_SLP    = 5,
};

enum EDimetrodonState
{
	DMT_WALK   = 0,
	DMT_RUN    = 1,
	DMT_IDLE1  = 2,
	DMT_IDLE2  = 3,
	DMT_DIE    = 4,
	DMT_SLP    = 5,
};

enum EAnkylosaurState
{
	ANK_RUN    = 0,
	ANK_WALK   = 1,
	ANK_IDLE1  = 2,
	ANK_IDLE2  = 3,
	ANK_DIE    = 4,
	ANK_SLP    = 5,
};

enum EStegosaurState
{
	STG_RUN    = 0,
	STG_WALK   = 1,
	STG_DIE    = 2,
	STG_IDLE1  = 3,
	STG_IDLE2  = 4,
	STG_SLP    = 5,
};

enum EGallimimusState
{
	GAL_RUN    = 0,
	GAL_WALK   = 1,
	GAL_SLIDE  = 2,
	GAL_DIE    = 3,
	GAL_IDLE1  = 4,
	GAL_IDLE2  = 5,
	GAL_SLP    = 6,
};

enum ETrikeState
{
	TRI_RUN    = 0,
	TRI_WALK   = 1,
	TRI_IDLE1  = 2,
	TRI_IDLE2  = 3,
	TRI_IDLE3  = 4,
	TRI_DIE    = 5,
	TRI_SLP    = 6,
};

enum EPachyState
{
	PAC_WALK   = 0,
	PAC_RUN    = 1,
	PAC_SLIDE  = 2,
	PAC_DIE    = 3,
	PAC_IDLE1  = 4,
	PAC_IDLE2  = 5,
	PAC_SLP    = 6,
};

enum EParasaurState
{
	PAR_WALK   = 0,
	PAR_RUN    = 1,
	PAR_IDLE1  = 2,
	PAR_IDLE2  = 3,
	PAR_DIE    = 4,
	PAR_SLP    = 5,
};

enum EIguanodonState
{
	IGU_WALK	= 0,
	IGU_RUN		= 1,
	IGU_IDLE1	= 2,
	IGU_IDLE2	= 3,
	IGU_DIE		= 4,
	IGU_SLP		= 5,
	IGU_ALRM1	= 6,
	IGU_ALRM2	= 7,
};

enum EDimorphodonState
{
	DIM_FLY = 0,
	DIM_FLYP,
	DIM_FALL,
	DIM_DIE,
	DIM_SLP,
};

enum EBrachiosaurState
{
	BRA_WALK = 0,
	BRA_IDLE1,
	BRA_IDLE2,
	BRA_IDLE3,
	BRA_DIE,
	BRA_SLP,
};

#endif // __AI_H__
