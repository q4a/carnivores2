
#include "AudioAL.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <vector>

using namespace std;

#define HEADER_ID_PCM			( ('P'<<8) | ('C'<<16) | ('M'<<24) )
#define HEADER_ID_WAVE			( ('R') | ('I'<<8) | ('F'<<16) | ('F'<<24) )
#define V_AUDIO_MAJOR			1
#define V_AUDIO_MINOR			7


typedef struct audiodevice_t {	// block aligned structure
	int16_t			m_card, m_device, m_subdevice, m_dummy;
	char			m_cname[ 64 ];
	char			m_dname[ 64 ];
	char			m_sdname[ 64 ];
} audiodevice_t;


ALCdevice				*g_ALDevice			= NULL;
ALCcontext				*g_ALContext		= NULL;
audiodevice_t*			g_AudioDevices		= 0;
uint32_t				g_AudioDeviceCount	= 0;


const char* alGetErrorString( ALenum e )
{
	switch ( e )
	{
		case AL_NO_ERROR:			return "No error";
		case AL_INVALID_NAME:		return "Invalid Name paramater passed to AL call";
		case AL_INVALID_ENUM:		return "Invalid parameter passed to AL call";
		case AL_INVALID_VALUE:		return "Invalid enum parameter value";
		case AL_INVALID_OPERATION:	return "Invalid operation";
		case AL_OUT_OF_MEMORY:		return "Out of memory";
	}

	return "Unknown error";
}

bool alCheckError( FILE* fp, const char* func = " " )
{
	ALenum err = alGetError();

	if ( err == AL_NO_ERROR )
	return false;

	const char* str = alGetErrorString( err );
	fprintf( fp, "AL Error in %s: %s\n", func, str );

	return true;
}




/*
	Add a Hardware Device to the list of available devices
*/
void addDeviceToList( audiodevice_t* device, FILE* out = stdout )
{
	if ( g_AudioDevices == 0 ) return;

	//memcpy( ( (audiodevice_t*)g_AudioDevices + g_AudioDeviceIndex), device, sizeof( audiodevice_t ) );
	fprintf( out, "\thw:%u,%u,%u \"%s\" \"%s\" \"%s\"\n", device->m_card, device->m_device, device->m_subdevice, device->m_cname, device->m_dname, device->m_sdname );
	//g_AudioDeviceIndex++;
}


AudioAL::AudioAL()
{
	m_stream = fopen( "AUDIO.LOG", "w" );
	fprintf( m_stream, "================================================\n" );
	fprintf( m_stream, "\tAtmosFEAR 2 - Audio Log\n" );
	fprintf( m_stream, "\tVersion: %u.%u\n", V_AUDIO_MAJOR, V_AUDIO_MINOR );
	fprintf( m_stream, "\tDate: %s %s\n", __TIME__, __DATE__ );
	fprintf( m_stream, "================================================\n\n" );
}

AudioAL::~AudioAL()
{
	fclose( m_stream );
}


bool AudioAL::init( uint8_t p_voices )
{
	m_ready = false;

	fprintf( m_stream, "InitAudioSystem( \"OpenAL\" )\n{\n" );

	const char* devices = alcGetString( NULL, ALC_DEVICE_SPECIFIER );

	fprintf( m_stream, "\tDevices: %s\n", devices );

	// -- Open the playback device
	fprintf( m_stream, "\tOpening default playback device... " );
	if ( !(g_ALDevice = alcOpenDevice( NULL )) )
		fprintf( m_stream, "Fail!\n" );
	else
		fprintf( m_stream, "Ok!\n" );

	//alcCaptureOpenDevice( NULL, 22050, AL_FORMAT_MONO8, 1024 );
	//alcCaptureCloseDevice()

	// -- Create Audio Context
	g_ALContext = alcCreateContext( g_ALDevice, NULL );
	if ( g_ALContext )
	{
		alcMakeContextCurrent( g_ALContext );
		alcProcessContext( g_ALContext );
	}
	else
	{
		fprintf( m_stream, "\tFailed to create the playback context\n" );
	}

	// -- AL Version
	fprintf( m_stream, "\tOpenAL Version:\t\t%s\n", alGetString( AL_VERSION ) );
	// -- AL Vender
	fprintf( m_stream, "\tOpenAL Vendor:\t\t%s\n", alGetString( AL_VENDOR ) );
	// -- AL Renderer
	fprintf( m_stream, "\tOpenAL Processor:\t%s\n", alGetString( AL_RENDERER ) );

	// -- Generate Voice Sources
	fprintf( m_stream, "\tCreating %u Voices...", p_voices );

	m_numvoices = p_voices;
	m_voices.clear();
	for ( int i=0; i<m_numvoices; i++ )
	{
		m_voices.push_back( voice_t() );
		alGenSources( 1, &m_voices[i] );
		alCheckError( m_stream );
	}

	alGenSources( 1, &m_looped_voice );

	fprintf( m_stream, " Ok!\n" );

	//alDistanceModel( AL_LINEAR_DISTANCE );

	//while ( alGetError != AL_NO_ERROR );

	fprintf( m_stream, "}\n\n" );
	fprintf( m_stream, "GameLoop()\n{\n" );

	m_ready = true;

	return true;
}

bool AudioAL::shutdown()
{
	if ( !m_ready ) return false;

	fprintf( m_stream, "}\n\n" );

	fflush( m_stream );
	fprintf( m_stream, "ShutdownAudioSystem()\n{\n" );

	fprintf( m_stream, "\tReleasing AL voices: " );

	alSourceStop( m_looped_voice );
	alDeleteSources( 1, &m_looped_voice );

	for ( int i=0; i<m_voices.size(); i++ )
	{
		alSourceStop( m_voices[i] );
		alDeleteSources( 1, &m_voices[i] );
	}
	m_voices.clear();

	fprintf( m_stream, "Ok\n" );

	this->freeSounds();

	fprintf( m_stream, "\tShutdown audio device: " );

	alcMakeContextCurrent( NULL );
	alcDestroyContext( g_ALContext );

	alcCloseDevice( g_ALDevice );

	fprintf( m_stream, "Ok\n" );
	fprintf( m_stream, "}\n" );

	m_ready = false;

	return true;
}

bool AudioAL::process( )
{
	if ( !m_ready ) return false;

	return true;
}

const char* AudioAL::getName()
{
	return "OpenAL 1.1";
}

bool AudioAL::addVoice( sound_t p_snd, float x, float y, float z, float p_volume )
{
	if ( !m_ready ) return false;
	if ( p_snd == 0xFFFFFFFF ) return false;
	// -- If Volume is inaudible, quit
	if ( p_volume < 0.001f ) return false;
	if ( p_volume > 1.0f ) p_volume = 1.0f;

	#if defined( AF_DEBUG )
	if ( !alIsBuffer( m_sounds[p_snd] ) )
	{
		fprintf( m_stream, "\taddVoice( %u ) -> ALBuffer is invalid!\n", p_snd );
		return false;
	}
	#endif

	ALuint src = 0;

	for ( int i=0; i<m_voices.size(); i++ )
	{
		ALint	state = AL_STOPPED;
		alGetSourcei( m_voices[i], AL_SOURCE_STATE, &state );
		if ( state == AL_PLAYING )
		{
			//fprintf( m_stream, "\taddVoice( ) -> Voice[%d] is in use!\n", i );
			continue;
		}
		src = m_voices[i];

		alSourceStop( src );
		alSource3f( src, AL_POSITION, x, y, z );
		alSourcei( src, AL_BUFFER, m_sounds[p_snd] );
		alSourcef( src, AL_GAIN, p_volume );
		alSourcef( src, AL_MAX_DISTANCE, 2.0f );
		alSourcePlay( src );
		return true;
	}

#if defined( AF_DEBUG )
	fprintf( m_stream, "\tDEBUG -> Ran out of available voices.\n" );
#endif

	return false;
}

sound_t AudioAL::loadSound( const char* p_file )
{
	if ( !m_ready ) return 0xFFFFFFFF;
	if ( !p_file ) return 0xFFFFFFFF;

	sound_t p_snd = 0;
	FILE* fp = fopen( p_file, "rb" );

	if ( !fp )
	{
		fprintf( m_stream, "\tFailed to open the audio file \"%s\" for reading!\n", p_file );
		return 0;
	}

	uint32_t tag = 0;
	fread( &tag, 4, 1, fp );

	if ( tag == HEADER_ID_WAVE )
	{
		fseek( fp, 4, SEEK_CUR ); // -- Skip the container size
		fseek( fp, 12, SEEK_CUR ); // -- Skip the WAVE Format Header

		uint16_t type = 0;
		uint16_t channels = 0;
		uint32_t samplerate = 0;
		uint32_t avgbytespersec = 0;
		uint16_t blockalign = 0;
		uint16_t bitdepth = 0;
		uint32_t pcm_length = 0;

		fread( &type, 2, 1, fp );
		fread( &channels, 2, 1, fp );
		fread( &samplerate, 4, 1, fp );
		fread( &avgbytespersec, 4, 1, fp );
		fread( &blockalign, 2, 1, fp );
		fread( &bitdepth, 2, 1, fp );

		fseek( fp, 4, SEEK_CUR ); // -- Skip data tag

		fread( &pcm_length, 4, 1, fp );

		char* pcm_data = new char [ pcm_length ];

		fread( pcm_data, pcm_length, 1, fp );

		if ( type == 1 )
		{
			ALenum format = 0;
			if ( channels == 1 && bitdepth == 8 ) format = AL_FORMAT_MONO8;
			if ( channels == 1 && bitdepth == 16 ) format = AL_FORMAT_MONO16;
			if ( channels == 2 && bitdepth == 8 ) format = AL_FORMAT_STEREO8;
			if ( channels == 2 && bitdepth == 16 ) format = AL_FORMAT_STEREO16;

			ALuint buffer = 0;

			// -- Get the index
			p_snd = m_sounds.size();

			// -- Generate the resource
			alGenBuffers( 1, &buffer );
			alCheckError( m_stream, "loadSound()::alGenBuffers" );
			this->m_sounds.push_back( buffer );

			alBufferData( buffer, format, pcm_data, pcm_length, samplerate );
			alCheckError( m_stream, "loadSound()::alBufferData" );
		}
		else
		{
			fprintf( m_stream, "\tThe specified WAVE is not a raw PCM!\n" );
		}

		delete [] pcm_data;
	}
	else if ( tag == HEADER_ID_PCM )
	{
		p_snd = 0;
		fprintf( m_stream, "\tPCM Files not supported.\n" );
	}

	fclose( fp );

	return p_snd;
}

sound_t AudioAL::loadSound( void* p_data, size_t p_length, uint32_t p_rate, uint32_t p_channels, uint32_t p_bits )
{
	if ( !m_ready || !p_data || !p_length ) return 0xFFFFFFFF;

	sound_t p_snd = 0;

	ALenum format = 0;

	if ( p_channels == 1 && p_bits == 8 ) format = AL_FORMAT_MONO8;
	if ( p_channels == 1 && p_bits == 16 ) format = AL_FORMAT_MONO16;
	if ( p_channels == 2 && p_bits == 8 ) format = AL_FORMAT_STEREO8;
	if ( p_channels == 2 && p_bits == 16 ) format = AL_FORMAT_STEREO16;

	ALuint buffer = 0;

	// -- Get the index
	p_snd = m_sounds.size();

	// -- Generate the resource
	alGenBuffers( 1, &buffer );
	alCheckError( m_stream, "loadSound()::alGenBuffers" );
	this->m_sounds.push_back( buffer );

	alBufferData( buffer, format, p_data, p_length, p_rate );
	alCheckError( m_stream, "loadSound()::alBufferData" );

	return p_snd;
}

void AudioAL::freeSounds( )
{
	if ( !m_ready ) return;

	fprintf( m_stream, "\tReleasing AL sounds: " );

	for ( int i=0; i<m_sounds.size(); i++ )
	{
		alDeleteBuffers( 1, &m_sounds[i] );
		ALenum e = alGetError();
		if ( e != AL_NO_ERROR )
		{
			fprintf( m_stream, "\tFailed to delete buffer %d\n\t%s\n", i, alGetErrorString( e ) );
		}
	}
	m_sounds.clear();

	fprintf( m_stream, "Ok\n" );
}

void AudioAL::freeSound( sound_t p_snd )
{
	if ( !m_ready ) return;

	vector<sound_t>::iterator i=m_sounds.begin() + p_snd;

	ALuint buffer = *i;
	alDeleteBuffers( 1, &buffer );

	ALenum e = alGetError();
	if ( e != AL_NO_ERROR )
	{
		fprintf( m_stream, "\tFailed to delete buffer %d\n\t%s\n", i, alGetErrorString( e ) );
	}

	m_sounds.erase( i );
}

bool AudioAL::startLoopedVoice( sound_t p_snd )
{
	if ( !m_ready ) return false;
	alSource3f( m_looped_voice, AL_POSITION, 0.0f, 0.0f, 0.0f );
	alSourcei( m_looped_voice, AL_BUFFER, p_snd );
	alSourcef( m_looped_voice, AL_GAIN, 1.0f );
	alSourcei( m_looped_voice, AL_LOOPING, AL_TRUE );
	alSourcePlay( m_looped_voice );
	return true;
}
