
#ifndef __audiosoft_h__
#define __audiosoft_h__

#include <stdint.h>

typedef struct snd_buffer_s
{
	void			*data;
	size_t			length;

} snd_buffer_t, *snd_buffer_p;

typedef struct snd_source_s
{
	enum
	{
		SOURCE_READY		= 0,
		SOURCE_USED			= 1
	}			status;
	size_t 		position;  // position of next portion to read
	int32_t		x, y, z;    // coords of the voice
	int32_t		volume;   // for aligning
} snd_source_t, *snd_source_p;


////////////////////////////////////////////////////////////////////////////////////////////////

#include "Hunt.h"
#include "AudioDriver.h"
#include "AudioAL.h"

#if defined( AF_PLATFORM_LINUX )
#	include <pthread.h>
#	define THREAD_RET void*
#	define thread_t	pthread_t
#elif defined( AF_PLATFORM_WINDOWS )
#	include <Windows.h>
#	define THREAD_RET DWORD WINAPI
#	define thread_t HANDLE
#endif

#define V_AUDIO_MAJOR			1
#define V_AUDIO_MINOR			6
#define MAX_VOICES				32


AudioDriver*		g_AudioDevice = 0;
bool				g_AudioActive = false;


void AudioLog( const char* str )
{
	if ( str == 0 ) return;

	fprintf( haudiolog, "%s", str );
}

void InitAudioSystem( int32_t p_driver )
{
	haudiolog = fopen( "AUDIO.LOG", "w" );

	fprintf( haudiolog, "================================================\n" );
	fprintf( haudiolog, "\tAtmosFEAR 2 - Audio Log\n" );
	fprintf( haudiolog, "\tVersion: %u.%u\n", V_AUDIO_MAJOR, V_AUDIO_MINOR );
	fprintf( haudiolog, "\tDate: %s %s\n", __TIME__, __DATE__ );
	fprintf( haudiolog, "================================================\n\n" );

	if ( p_driver == AUDIO_SOFTWARE )
		g_AudioDevice = new AudioDriver( haudiolog );
	if ( p_driver == AUDIO_OPENAL )
		g_AudioDevice = new AudioAL( haudiolog );
//	if ( p_driver == AUDIO_DIRECTSOUND )
//		g_AudioDevice = new AudioDS( haudiolog );
//	if ( p_driver == AUDIO_XAUDIO )
//		g_AudioDevice = new AudioXA( haudiolog );

	fprintf( haudiolog, "InitAudioSystem( \"%s\" )\n{\n", g_AudioDevice->getName() );
	g_AudioActive = g_AudioDevice->init( MAX_VOICES );
	fprintf( haudiolog, "}\n\n" );
}

void Audio_Shutdown()
{
	if ( haudiolog )
	{
		fflush( haudiolog );
		fprintf( haudiolog, "Audio_Shutdown()\n{\n" );
	}

	if ( g_AudioActive )
	{
		g_AudioDevice->shutdown();
		g_AudioActive = false;
		if ( g_AudioDevice )
		delete g_AudioDevice;
	}

	if ( haudiolog ) fprintf( haudiolog, "\tShutdown audio device: Ok\n" );

	if ( haudiolog ) fprintf( haudiolog, "}\n" );
	if ( haudiolog ) fclose( haudiolog );
}

void SetAmbient3d(int length, int16_t *lpdata, float cx, float cy, float cz)
{
	return;

	if(!g_AudioActive)
	{
		return;
	}

	if(mambient.iPosition >= length)
	{
		mambient.iPosition = 0;
	}

	mambient.iLength = length;
	mambient.lpData = lpdata;
	mambient.x = cx;
	mambient.y = cy;
	mambient.z = cz;
}

void SetAmbient(int length, int16_t* lpdata, int vol)
{
	return;

	if ( !g_AudioActive || !length || !lpdata || !vol ) return;

	if(ambient.lpData == lpdata)
	{
		return;
	}

	ambient2 = ambient;
	ambient.iLength = length;
	ambient.lpData = lpdata;
	ambient.iPosition = 0;
	ambient.volume = 0;
	ambient.avolume = vol;
}

void AudioSetCameraPos(float cx, float cy, float cz, float ca, float cb)
{
	xCamera = (int) cx;
	yCamera = (int) cy;
	zCamera = (int) cz;
	alphaCamera = ca;
	cosa = (float)cosf(ca);
	sina = (float)sinf(ca);
}

void Audio_Restore(){}					// Restore audio system after AudioStop()
void AudioStop(){}						// Stop the audio (dont really need this anymore since game is still active in background)
void Audio_SetEnvironment(int, float){} // This is Hardware dependant (EAX)

void AddVoice3dv(int length, int16_t *lpdata, float cx, float cy, float cz, int vol)
{
	// If audio isnt active or length/data/volume are zero we ignore this function call
	if ( !g_AudioActive || !length || !lpdata || !vol ) return;

	for(int i = 0; i < MAX_VOICES; i++)
	{
		if(!g_Voices[i].status)
		{
			g_Voices[i].iLength = length;
			g_Voices[i].lpData  = lpdata;
			g_Voices[i].iPosition = 0;
			g_Voices[i].x = (int)cx;
			g_Voices[i].y = (int)cy;
			g_Voices[i].z = (int)cz;
			g_Voices[i].status = 1;
			g_Voices[i].volume = vol;
			return;
		}
	}

#ifdef AF_DEBUG
	fprintf( stderr, "All Voices are in use!\n" );
#endif
}

void  AddVoicev( int length, int16_t* lpdata, int volume )
{
	AddVoice3dv( length, lpdata, 0,0,0, volume );
}

void  AddVoice( int length, int16_t* lpdata )
{
	AddVoice3dv( length, lpdata, 0,0,0, 256 );
}

void  AddVoice3d(int length, int16_t* lpdata, float cx, float cy, float cz)
{
	AddVoice3dv( length, lpdata, cx,cy,cz, 256 );
}

///////////////////////////////////////////////////////////////////////////////////////////////

#endif // __audiosoft_h__
