
#include "Hunt.h"


void InitAudioSystem( int32_t p_driver )
{
}

void Audio_Shutdown()
{
}

void SetAmbient3d(int length, int16_t *lpdata, float cx, float cy, float cz)
{
}

void SetAmbient(int length, int16_t* lpdata, int vol)
{
}

void AudioSetCameraPos(float cx, float cy, float cz, float ca, float cb)
{
	/*xCamera = (int) cx;
	yCamera = (int) cy;
	zCamera = (int) cz;
	alphaCamera = ca;
	cosa = (float)cosf(ca);
	sina = (float)sinf(ca);*/
}

void Audio_Restore(){}					// Restore audio system after AudioStop()
void AudioStop(){}						// Stop the audio (dont really need this anymore since game is still active in background)
void Audio_SetEnvironment(int, float){} // This is Hardware dependant (EAX)

void AddVoice3dv(int length, int16_t *lpdata, float cx, float cy, float cz, int vol)
{
}

void  AddVoicev( int length, int16_t* lpdata, int volume )
{
}

void  AddVoice( int length, int16_t* lpdata )
{
}

void  AddVoice3d(int length, int16_t* lpdata, float cx, float cy, float cz)
{
}

