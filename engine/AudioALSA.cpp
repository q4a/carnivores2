
#include "AudioDriver.h"

#include <alsa/asoundlib.h>

#define LEFT_LOW        		32
#define LEFT_MIDDLE     		88
#define LEFT_HIGH       		144

#define RIGHT_LOW       		144
#define RIGHT_MIDDLE    		200
#define RIGHT_HIGH      		256

#define MAX_RADIUS         		6000
#define MIN_RADIUS         		512

enum audiostate_e
{
	AUDIOSTATE_SHUTTINGDOWN = 100
};


typedef struct audiodevice_t {	// block aligned structure
	int16_t			m_card, m_device, m_subdevice, m_dummy;
	char			m_cname[ 64 ];
	char			m_dname[ 64 ];
	char			m_sdname[ 64 ];
} audiodevice_t;


int 					g_SelectedCard = -1;
char					g_CardName[64];
snd_ctl_t*				g_Card = 0;
snd_pcm_t*				g_PCMPlayback = 0;
snd_pcm_hw_params_t*	g_PCMHWParams = 0;
snd_pcm_uframes_t		g_PCMNumFrames = 32;
bool					g_AudioReady	= false;
audiodevice_t*			g_AudioDevices = 0;
uint32_t				g_AudioDeviceIndex = 0;
uint32_t				g_AudioDeviceCount = 0;
size_t					g_SampleLength = 0;
int16_t*				g_SampleBuffer = 0;

VOICE		g_Voices[MAX_VOICES];
uint32_t	g_SampleRate = 22050;
int			xCamera, yCamera, zCamera;
float		alphaCamera, cosa, sina;
thread_t	g_AudioThread = 0;
uint32_t	g_AudioThreadID = 0;
uint32_t	g_AudioState = 0;
int16_t		_SoundBufferData[ MAX_BUFFER_LENGTH/2 ];
int16_t		*lpSoundBuffer;
size_t		iSoundBufferLen = 0;
AMBIENT		ambient,
			ambient2;
MAMBIENT	mambient;


THREAD_RET ProcessAudioThread(void* ptr);
void Audio_MixSound(int16_t *DestAddr, int16_t *SrcAddr, int MixLen, int LVolume, int RVolume);
void Audio_MixChannels();
void Audio_MixAmbient();
void Audio_MixAmbient3d();


/*
	Add a Hardware Device to the list of available devices
*/
void addDeviceToList( audiodevice_t* device, FILE* out = stdout )
{
	if ( g_AudioDevices == 0 ) return;

	memcpy( ( (audiodevice_t*)g_AudioDevices + g_AudioDeviceIndex), device, sizeof( audiodevice_t ) );
	fprintf( out, "\thw:%u,%u,%u \"%s\" \"%s\" \"%s\"\n", device->m_card, device->m_device, device->m_subdevice, device->m_cname, device->m_dname, device->m_sdname );
	g_AudioDeviceIndex++;
}


/*
	Iterate through the cards, devices and sub-devices available for audio output,
	take a two-pass method, get the total number available, allocate the device list
	then resift through the list again to store the actual devices.
*/
void alsaDeviceList( FILE* out )
{
	snd_pcm_stream_t		stream = SND_PCM_STREAM_PLAYBACK;
	snd_ctl_t*				handle;
	int						card,
							err,
							dev,
							idx;
	snd_ctl_card_info_t*	info;
	snd_pcm_info_t*			pcminfo;


	snd_ctl_card_info_alloca( &info );		//alloca will free itself when the function exits
	snd_pcm_info_alloca( &pcminfo );		//alloca will free itself when the function exits

	// -- Iterate and find all the available cards and their devices
devicelist:
	card = -1;
	if (snd_card_next(&card) < 0 || card < 0) {
		fprintf( out, "no soundcards found...");
		return;
	}

	//fprintf( stdout, "**** List of %s Hardware Devices ****\n", snd_pcm_stream_name(stream) );

	while (card >= 0)
	{
		char name[32];
		sprintf(name, "hw:%d", card);
		if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
			fprintf( out, "control open (%i): %s", card, snd_strerror(err) );
			goto next_card;
		}
		if ((err = snd_ctl_card_info(handle, info)) < 0) {
			fprintf( out, "control hardware info (%i): %s", card, snd_strerror(err));
			snd_ctl_close(handle);
			goto next_card;
		}
		dev = -1;
		while (1) {
			unsigned int count;
			if (snd_ctl_pcm_next_device(handle, &dev)<0)
			{
				fprintf( out, "snd_ctl_pcm_next_device");
			}
			if (dev < 0)
				break;

			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);

			if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
			{
				if (err != -ENOENT)
				{
					fprintf( out, "control digital audio info (%i): %s", card, snd_strerror(err));
				}
				continue;
			}

			/*fprintf( out, "card %i: %s [%s], device %i: %s [%s]\n",
				card, snd_ctl_card_info_get_id(info), snd_ctl_card_info_get_name(info),
				dev,
				snd_pcm_info_get_id(pcminfo),
				snd_pcm_info_get_name(pcminfo));*/

			count = snd_pcm_info_get_subdevices_count(pcminfo);
			//fprintf( out, "  Subdevices: %i/%i\n", snd_pcm_info_get_subdevices_avail(pcminfo), count);

			for (idx = 0; idx < (int)count; idx++)
			{
				snd_pcm_info_set_subdevice(pcminfo, idx);
				if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
				{
					fprintf( out, "control digital audio playback info (%i): %s", card, snd_strerror(err));
				}
				else
				{
					//fprintf( out, "  Subdevice #%i: %s\n", idx, snd_pcm_info_get_subdevice_name(pcminfo));
					g_AudioDeviceCount++;
					audiodevice_t device = {
						card,
						dev,
						idx,
						0,
						"N/C",
						"N/D",
						"N/SD"
					};
					int l = strlen( snd_ctl_card_info_get_name(info) );
					strncpy( device.m_cname, snd_ctl_card_info_get_name(info), (l>63) ? 63 : l );
					l = strlen( snd_pcm_info_get_name(pcminfo) );
					strncpy( device.m_dname, snd_pcm_info_get_name(pcminfo), (l>63) ? 63 : l );
					l = strlen( snd_pcm_info_get_subdevice_name(pcminfo) );
					strncpy( device.m_sdname, snd_pcm_info_get_subdevice_name(pcminfo), (l>63) ? 63 : l );
					addDeviceToList( &device, out );
				}
			}
		}

		snd_ctl_close(handle);

	next_card:
		//if ( name ) free( name );
		if (snd_card_next(&card) < 0)
		{
			fprintf( out, "snd_card_next\n");
			break;
		}
	}

	// -- Allocate a list of audio devices
	if ( g_AudioDevices == 0 )
	{
		g_AudioDevices = (audiodevice_t*)malloc( g_AudioDeviceCount * sizeof(audiodevice_t) );
		fprintf( out, "\tAvailable Audio Devices: %u\n", g_AudioDeviceCount );
		goto devicelist;
	}

	//snd_pcm_info_free(pcminfo);
}


bool AudioDriver::init( int16_t* p_buffer, size_t p_length, uint8_t p_voices )
{
	int err = 0;

	this->m_voices = p_voices;

	fprintf( m_stream, "\tInitialising ALSA Audio...\n" );

	fprintf( m_stream, "\tGathering available audio devices...\n" );
	alsaDeviceList( m_stream );

	fprintf( m_stream, "\tOpening PCM WaveForm device...\n" );

	int i;
	int16_t *sample = 0;
	unsigned int samplerate = 22050;
	int dir 				= 0;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;

	g_SampleBuffer = p_buffer;
	g_SampleLength = p_length;

	//char device[32] = "hw:1,0,0"; // Change this to desired card,device,sdevice at later date from global options
	char device[32] = "pulse";//"hw:0,0,0"; // Change this to desired card,device,sdevice at later date from global options

	if ((err = snd_pcm_open (&g_PCMPlayback, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		fprintf (m_stream, "ERROR -> cannot open audio device %s (%s)\n",
			 device,
			 snd_strerror (err));
		return false;
	}

	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (m_stream, "ERROR -> cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return false;
	}

	if ((err = snd_pcm_hw_params_any (g_PCMPlayback, hw_params)) < 0) {
		fprintf (m_stream, "ERROR -> cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		goto failed;
	}

	err = snd_pcm_hw_params_set_rate_resample(g_PCMPlayback, hw_params, 0 );
	if (err < 0) {
		fprintf( m_stream, "ERROR -> cannot set rate resample for playback: (%s)\n", snd_strerror(err));
		goto failed;
	}

	if ((err = snd_pcm_hw_params_set_access (g_PCMPlayback, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (m_stream, "ERROR -> cannot set access type (%s)\n",
			 snd_strerror (err));
		goto failed;
	}

	if ((err = snd_pcm_hw_params_set_format (g_PCMPlayback, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (m_stream, "ERROR -> cannot set sample format (%s)\n",
			 snd_strerror (err));
		goto failed;
	}

	// -- Set the SampleRate of the device
	if ((err = snd_pcm_hw_params_set_rate_near (g_PCMPlayback, hw_params, &samplerate, &dir)) < 0)
	{
		fprintf (m_stream, "ERROR -> cannot set sample rate (%s)\n",
			 snd_strerror (err));
		goto failed;
	}
	if ( samplerate != 22050 )
	{
		fprintf( m_stream, "\tWARNING -> SampleRate is %uhz but we desire 22050hz\n", samplerate );
	}

	g_PCMNumFrames = 32;
	snd_pcm_hw_params_set_period_size_near( g_PCMPlayback, hw_params, &g_PCMNumFrames, &dir);

	if ((err = snd_pcm_hw_params_set_channels (g_PCMPlayback, hw_params, 2)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot set channel count (%s)\n",
			 snd_strerror (err));
		goto failed;
	}

	if ((err = snd_pcm_hw_params (g_PCMPlayback, hw_params)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot set parameters (%s)\n",
			 snd_strerror (err));
		goto failed;
	}

	/*if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot allocate software parameters structure (%s)\n",
			 snd_strerror (err));
		goto failed_sw;
	}
	if ((err = snd_pcm_sw_params_current (g_PCMPlayback, sw_params)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot initialize software parameters structure (%s)\n",
			 snd_strerror (err));
		goto failed_sw;
	}
	if ((err = snd_pcm_sw_params_set_avail_min (g_PCMPlayback, sw_params, 2048U)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot set minimum available count (%s)\n",
			 snd_strerror (err));
		goto failed_sw;
	}
	if ((err = snd_pcm_sw_params_set_start_threshold (g_PCMPlayback, sw_params, 0U)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot set start mode (%s)\n",
			 snd_strerror (err));
		goto failed_sw;
	}
	if ((err = snd_pcm_sw_params (g_PCMPlayback, sw_params)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot set software parameters (%s)\n",
			 snd_strerror (err));
		goto failed_sw;
	}*/

	if ((err = snd_pcm_prepare (g_PCMPlayback)) < 0) {
		fprintf (m_stream, "\tERROR -> cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		goto failed;
	}

	fprintf( m_stream, "\tSampleRate: %dhz\n", samplerate );
	fprintf( m_stream, "\tChannels: 2 [Stereo]\n" );
	fprintf( m_stream, "\tBitDepth: 16 Little-Endian\n" );

	this->m_samplerate	= samplerate;
	this->m_channels	= 2;

	int ierr = pthread_create( &g_AudioThread, 0, (void*(*)(void*))&ProcessAudioThread, 0 );
	if ( ierr )
	{
		fprintf( haudiolog, "\tERROR -> Failed to create the audio processing thread!\n" );
	}

	g_AudioReady = true;

	//nd_pcm_sw_params_free( sw_params );
	snd_pcm_hw_params_free (hw_params);
	return true;

failed_sw:
	//snd_pcm_sw_params_free( sw_params );
failed:
	snd_pcm_hw_params_free( hw_params );
	return false;
}

bool AudioDriver::shutdown()
{

	g_AudioState = AUDIOSTATE_SHUTTINGDOWN;

	pthread_join( g_AudioThread, 0 );

	if ( m_stream ) fprintf( m_stream, "\tTerminate processing thread: Ok\n" );

	if ( g_PCMPlayback )
	{
		snd_pcm_drain( g_PCMPlayback );
		snd_pcm_drop( g_PCMPlayback );
		snd_pcm_close( g_PCMPlayback );
	}

	if ( g_AudioDevices )
	{
		free( g_AudioDevices );
		g_AudioDevices = 0;
	}

	g_AudioReady = false;

	return true;
}

bool AudioDriver::process( )
{
	if ( !g_AudioReady ) return false;
	int err = -1;

	err = snd_pcm_writei( g_PCMPlayback, g_SampleBuffer, g_PCMNumFrames );//g_SampleLength );

	if (err == -EPIPE)
	{
		/* EPIPE means underrun */
		fprintf(stderr, "underrun occurred\n");
		snd_pcm_prepare( g_PCMPlayback );
	}
	else if (err < 0)
	{
		fprintf(stderr, "error from writei: %s\n", snd_strerror( err ) );
	}
	else if (err != (int)g_PCMNumFrames)
	{
		fprintf(stderr, "short write, write %d frames\n", err );
	}

	return true;
}

const char* AudioDriver::getName()
{
	return "Software Mixer by Alexey Menshikov";
}

int ProcessAudio()
{
	//uint32_t  len1, len2;
	//size_t hres = 0;
	//int CurBlock, NextBlock;
	//static int PrevBlock = 1;

	/*if(AudioNeedRestore)
	{
		Audio_Restore();
	}

	if(AudioNeedRestore)
	{
		return 1;
	}*/

	memset( lpSoundBuffer, 0, MAX_BUFFER_LENGTH );
	Audio_MixChannels();
	Audio_MixAmbient();
	Audio_MixAmbient3d();

	g_AudioDevice->process( );

	return 1;
}


void CalcLRVolumes(int v0, int x, int y, int z, int& lv, int& rv)
{
	float xx, yy, zz, xa, d, k, fi;
	float l = 0.8f;
	float r = 0.8f;

	if(x == 0)
	{
		lv = v0 * 180;
		rv = v0 * 180;
		return;
	}

	v0 *= 200;
	x -= xCamera;
	y -= yCamera;
	z -= zCamera;

	xx = (float)x * cosa + (float)z * sina;
	yy = (float)y;
	zz = (float)fabsf((float)z * cosa - (float)x * sina);
	xa = (float)fabsf(xx);

	d = (float)sqrtf(xx * xx + yy * yy + zz * zz) - MIN_RADIUS;

	if(d <= 0)
	{
		k = 1.f;
	}
	else
	{
		k = 1224.f / (1224 + d);
	}

	if(d > 6000)
	{
		d -= 6000;
		k = k * (4000 - d) / (4000);
	}

	if(k < 0)
	{
		k = 0.f;
	}

	fi = (float)atan2(xa, zz);
	r = 0.7f + 0.3f * fi / (3.141593f / 2.f);
	l = 0.7f - 0.6f * fi / (3.141593f / 2.f);

	if(xx > 0)
	{
		lv = (int)(v0 * l * k);
		rv = (int)(v0 * r * k);
	}
	else
	{
		lv = (int)(v0 * r * k);
		rv = (int)(v0 * l * k);
	}
}


void Audio_MixChannels()
{
	int iMixLen;
	int LV, RV;

	for(int i = 0; i < MAX_VOICES; i++)
	{
		if(g_Voices[ i ].status)
		{
			if(g_Voices[ i ].iPosition + MAX_SB_LENGTH >= (g_Voices[ i ].iLength/2))
			{
				iMixLen = (g_Voices[ i ].iLength - g_Voices[ i ].iPosition) >> 1;
			}
			else
			{
				iMixLen = MAX_SB_LENGTH;
			}

			CalcLRVolumes(g_Voices[i].volume, g_Voices[i].x, g_Voices[i].y, g_Voices[i].z, LV, RV);

			if(LV || RV)
			{
				Audio_MixSound((int16_t *)lpSoundBuffer, ((int16_t *)((intptr_t)g_Voices[i].lpData + g_Voices[i].iPosition)), iMixLen, LV, RV);
				//Audio_MixSound((int16_t *)lpSoundBuffer, ((int16_t *)g_Voices[i].lpData + g_Voices[i].iPosition), iMixLen, LV, RV);
			}

			if(g_Voices[ i ].iPosition + MAX_SB_LENGTH >= (g_Voices[ i ].iLength/2))
			{
				g_Voices[ i ].status = 0;
			}
			else
			{
				g_Voices[ i ].iPosition += MAX_SB_LENGTH;
			}
		}
	}
}


void Audio_DoMixAmbient(AMBIENT& ambient)
{
	int iMixLen;
	int v;

	if(!ambient.lpData)
	{
		return;
	}

	v = (32000 * ambient.volume * ambient.avolume) / 256 / 256;

	if(ambient.iPosition + MAX_SB_LENGTH * 2 >= ambient.iLength)
	{
		iMixLen = (ambient.iLength - ambient.iPosition) >> 1;
	}
	else
	{
		iMixLen = MAX_SB_LENGTH;
	}

	Audio_MixSound((int16_t *)lpSoundBuffer, (int16_t *)(((intptr_t)ambient.lpData + ambient.iPosition)), iMixLen, v, v);

	if(ambient.iPosition + MAX_SB_LENGTH * 2 >= ambient.iLength)
	{
		ambient.iPosition = 0;
	}
	else
	{
		ambient.iPosition += MAX_SB_LENGTH * 2;
	}

	if ( iMixLen < MAX_SB_LENGTH )
	{
		Audio_MixSound((int16_t *)((intptr_t)lpSoundBuffer + iMixLen * 4),
		               (int16_t *)ambient.lpData, MAX_SB_LENGTH - iMixLen, v, v);
		ambient.iPosition += (MAX_SB_LENGTH - iMixLen);
	}
}


void Audio_MixAmbient()
{
	Audio_DoMixAmbient(ambient);

	if(ambient.volume < 256)
	{
		ambient.volume = af_min(ambient.volume + 16, 256);
	}

	if(ambient2.volume)
	{
		Audio_DoMixAmbient(ambient2);
	}

	if(ambient2.volume > 0)
	{
		ambient2.volume = af_max(ambient2.volume - 16, 0);
	}
}

void Audio_MixAmbient3d()
{
	int iMixLen;
	int LV, RV;

	if(!mambient.lpData)
	{
		return;
	}

	CalcLRVolumes(256, (int)mambient.x, (int)mambient.y, (int)mambient.z, LV, RV);

	if(!LV || !RV)
	{
		return;
	}

	if(mambient.iPosition + MAX_SB_LENGTH * 2 >= mambient.iLength)
	{
		iMixLen = (mambient.iLength - mambient.iPosition) >> 1;
	}
	else
	{
		iMixLen = MAX_SB_LENGTH;
	}

	Audio_MixSound((int16_t *)lpSoundBuffer, ((int16_t *)((intptr_t)mambient.lpData + mambient.iPosition)), iMixLen, LV, RV);

	if(mambient.iPosition + MAX_SB_LENGTH * 2 >= mambient.iLength)
	{
		mambient.iPosition = 0;
	}
	else
	{
		mambient.iPosition += MAX_SB_LENGTH;
	}

	if(iMixLen < MAX_SB_LENGTH)
	{
		Audio_MixSound((int16_t *)((intptr_t)lpSoundBuffer + iMixLen * 4),
		               (int16_t *)mambient.lpData, MAX_SB_LENGTH - iMixLen, LV, RV);
		mambient.iPosition += (MAX_SB_LENGTH - iMixLen);
	}
}

#define CLAMP16(x)	(((x) < 0) ? ((x) = -32767) : ((x) = 32767))

/* Mixes 16-bit signed mono, 22050Hz */
void Audio_MixSound(int16_t *DestAddr, int16_t *SrcAddr, int MixLen, int LVolume, int RVolume)
{
	int32_t tmp;

	for ( ; MixLen > 0; MixLen-- )//while(MixLen--)
	{
		/* Do the left channel */
		tmp = LVolume * *((int16_t*)SrcAddr) >> 16;

		if(tmp >= 32768)
		{
			CLAMP16(*DestAddr);
			//++DestAddr; CLAMP16(*DestAddr);
		}
		else
		{
			*DestAddr += tmp;
			//++DestAddr; *DestAddr += tmp;

		}

		++DestAddr;

		/* Do the right channel */
		tmp = RVolume * (*SrcAddr) >> 16;

		if(tmp >= 32768)
		{
			CLAMP16(*DestAddr);
			//++DestAddr; CLAMP16(*DestAddr);
		}
		else
		{
			*DestAddr += tmp;
			//++DestAddr; *DestAddr += tmp;
		}

		++DestAddr;
		++SrcAddr;
	}
}

void ProcessAudioThread( void* ptr )
{
	while( g_AudioState != AUDIOSTATE_SHUTTINGDOWN )
	{
		if ( g_AudioActive )
		{
			ProcessAudio();
		}
	}

	fprintf( haudiolog, "\tAUDIO-THREAD: Exited safely\n" );

	return 0;
}
