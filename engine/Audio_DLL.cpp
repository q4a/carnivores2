/* Audio in nix has been nixxed for the time being */

#include "Hunt.h"
#include "pcmLib.h"

#define V_AUDIO_MAJOR	1
#define V_AUDIO_MINOR	4

bool	g_AudioActive = false;


#ifdef AF_PLATFORM_NIX

void AudioLog( const char* str )
{
	if ( str == 0 ) return;

	FILE* fp = haudiolog;

	fprintf( fp, "%s", str );
}

void InitAudioSystem( int32_t p_driver )
{
	haudiolog = fopen( "AUDIO.LOG", "w" );

	fprintf( haudiolog, "================================================\n" );
	fprintf( haudiolog, "\tAtmosFEAR 2 - Audio Log\n" );
	fprintf( haudiolog, "\tVersion: %u.%u\n", V_AUDIO_MAJOR, V_AUDIO_MINOR );
	fprintf( haudiolog, "\tDate: %s %s\n", __TIME__, __DATE__ );
	fprintf( haudiolog, "================================================\n\n" );

	g_AudioActive = pcmInit( haudiolog );
}

void Audio_Shutdown()
{
	if ( g_AudioActive )
	{
		pcmShutdown();
		g_AudioActive = false;
	}

	if ( haudiolog ) fclose( haudiolog );
}

void SetAmbient3d(int, int16_t*, float, float, float){}
void SetAmbient(int, int16_t*, int){}
void AudioSetCameraPos(float, float, float, float, float){}
void Audio_Restore(){}
void AudioStop(){}
void Audio_SetEnvironment(int, float){}

void  AddVoicev  (int, int16_t*, int){}
void  AddVoice3dv(int, int16_t*, float, float, float, int){}
void  AddVoice3d (int, int16_t*, float, float, float){}

#elif defined( AF_PLATFORM_WINDOWS )

#include <windows.h>

#define req_versionH 0x0001
#define req_versionL 0x0002

HINSTANCE hAudioDLL = NULL;
void DoHalt(char*);

typedef void (WINAPI * LPFUNC1)(void);
typedef void (WINAPI * LPFUNC2)(HWND, HANDLE);
typedef void (WINAPI * LPFUNC3)(float, float, float, float, float);
typedef void (WINAPI * LPFUNC4)(int, uint16_t*, int);
typedef void (WINAPI * LPFUNC5)(int, uint16_t*, float, float, float);
typedef void (WINAPI * LPFUNC6)(int, uint16_t*, float, float, float, int);

typedef int  (WINAPI * LPFUNC7)(void);
typedef void (WINAPI * LPFUNC8)(int, float);

LPFUNC1 audio_restore;
LPFUNC1 audiostop;
LPFUNC1 audio_shutdown;

LPFUNC2 initaudiosystem;
LPFUNC3 audiosetcamerapos;
LPFUNC4 setambient;
LPFUNC5 setambient3d;
LPFUNC6 addvoice3dv;
LPFUNC7 audio_getversion;
LPFUNC8 audio_setenvironment;


void Audio_Shutdown()
{
	if (audio_shutdown) audio_shutdown();
	if (hAudioDLL)  	FreeLibrary(hAudioDLL);
	hAudioDLL = NULL;
	audio_shutdown = NULL;
}


void InitAudioSystem(HWND hw, HANDLE hlog, int  driver)
{
	Audio_Shutdown();

	switch (driver)
	{
        case 0:
            hAudioDLL = LoadLibrary("a_soft.dll");
            if (!hAudioDLL) DoHalt("Can't load A_SOFT.DLL");
        break;
        case 1:
            hAudioDLL = LoadLibrary("a_ds3d.dll");
            if (!hAudioDLL) DoHalt("Can't load A_DS3D.DLL");
        break;
        case 2:
            hAudioDLL = LoadLibrary("a_a3d.dll");
            if (!hAudioDLL) DoHalt("Can't load A_A3D.DLL");
        break;
        case 3:
            hAudioDLL = LoadLibrary("a_eax.dll");
            if (!hAudioDLL) DoHalt("Can't load A_EAX.DLL");
        break;
	}


	initaudiosystem   = (LPFUNC2) GetProcAddress(hAudioDLL, "InitAudioSystem");
	if (!initaudiosystem) DoHalt("Can't find procedure address.");

	audio_restore     = (LPFUNC1) GetProcAddress(hAudioDLL, "Audio_Restore");
	if (!audio_restore) DoHalt("Can't find procedure address.");

	audiostop         = (LPFUNC1) GetProcAddress(hAudioDLL, "AudioStop");
	if (!audiostop)   DoHalt("Can't find procedure address.");

	audio_shutdown    = (LPFUNC1) GetProcAddress(hAudioDLL, "Audio_Shutdown");
	if (!audio_shutdown) DoHalt("Can't find procedure address.");

	audiosetcamerapos = (LPFUNC3) GetProcAddress(hAudioDLL, "AudioSetCameraPos");
	if (!audiosetcamerapos) DoHalt("Can't find procedure address.");

	setambient        = (LPFUNC4) GetProcAddress(hAudioDLL, "SetAmbient");
	if (!setambient) DoHalt("Can't find procedure address.");

	setambient3d      = (LPFUNC5) GetProcAddress(hAudioDLL, "SetAmbient3d");
	if (!setambient3d) DoHalt("Can't find procedure address.");

	addvoice3dv       = (LPFUNC6) GetProcAddress(hAudioDLL, "AddVoice3dv");
	if (!addvoice3dv) DoHalt("Can't find procedure address.");

	audio_getversion  = (LPFUNC7) GetProcAddress(hAudioDLL, "Audio_GetVersion");
	if (!audio_getversion) DoHalt("Can't find procedure address.");

	audio_setenvironment = (LPFUNC8) GetProcAddress(hAudioDLL, "Audio_SetEnvironment");
	if (!audio_setenvironment) DoHalt("Can't find procedure address.");

	int v1 = audio_getversion()>>16;
	int v2 = audio_getversion() & 0xFFFF;
	if ( (v1!=req_versionH) || (v2<req_versionL) )
		DoHalt("Incorrect audio driver version.");

	initaudiosystem(hw, hlog);
}


void AudioStop()
{
    if(audiostop) // alacn
		audiostop();
}

void Audio_Restore()
{
	if (audio_restore)
  	  audio_restore();
}



void AudioSetCameraPos(float cx, float cy, float cz, float ca, float cb)
{
	audiosetcamerapos(cx, cy, cz, ca, cb);
}


void Audio_SetEnvironment(int e, float f)
{
   audio_setenvironment(e, f);
}

void SetAmbient(int length, uint16_t* lpdata, int av)
{
	if (!setambient) return;
	if (!length) return;
	if (!lpdata) return;
	setambient(length, lpdata, av);
}


void SetAmbient3d(int length, uint16_t* lpdata, float cx, float cy, float cz)
{
	setambient3d(length, lpdata, cx, cy, cz);
}


void AddVoice3dv(int length, uint16_t* lpdata, float cx, float cy, float cz, int vol)
{
	addvoice3dv(length, lpdata, cx, cy, cz, vol);
	//->XBox
}




void AddVoice3d(int length, uint16_t* lpdata, float cx, float cy, float cz)
{
	AddVoice3dv(length, lpdata, cx, cy, cz, 256);
}


void AddVoicev(int length, uint16_t* lpdata, int v)
{
	AddVoice3dv(length, lpdata, 0,0,0, v);
}


void AddVoice(int length, uint16_t* lpdata)
{
	AddVoice3dv(length, lpdata, 0,0,0, 256);
}

#endif //AF_PLATFORM_NIX
