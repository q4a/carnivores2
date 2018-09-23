/************************************************************************
* OpenAL Audio Driver
* Version:	1.0
* By:		James Ogden
*/

#ifndef __audioal_h__
#define __audioal_h__

#include <stdint.h>
#include <cstdio>
#include <vector>

typedef uint32_t	sound_t;
typedef uint32_t	voice_t;


class AudioAL
{
public:

	AudioAL( );
	~AudioAL( );

	/* Initialise the Audio Driver */
	bool init( uint8_t p_voices = 16 );

	/* Shutdown the Audio Driver */
	bool shutdown( );

	/* Pause the audio */
	bool pause();

	/* Resume the audio */
	bool resume();

	/* Process an audio stream, wait for the buffer to complete playing */
	bool process( );

	/* Get the name of this Audio */
	const char* getName();

	/* Load a sound raw PCM or MS-WAVE */
	sound_t loadSound( const char* p_file );
	sound_t loadSound( void* p_data, size_t p_length, uint32_t p_rate = 22050, uint32_t p_channels = 2, uint32_t p_bits = 16 );

	/* Free the resources for the desired sound */
	void freeSound( sound_t p_snd );

	/* Free the resources associated with the loaded sounds */
	void freeSounds( );

	/* Add a voice to the world */
	bool addVoice( sound_t p_snd, float x = 0.0f, float y = 0.0f, float z = 0.0f, float p_volume = 1.0f );

	/* Set the looped sound and play it (For Music) */
	bool startLoopedVoice( sound_t p_snd );

	/* Stop the looped sound */
	bool stopLoopedVoice( );

private:

	bool					m_ready;
	uint32_t				m_looped_voice;
	uint16_t				m_samplerate;
	uint8_t					m_channels;
	uint8_t					m_numvoices;
	FILE					*m_stream;
	std::vector<sound_t>	m_sounds;
	std::vector<voice_t>	m_voices;
};

#endif //__pcmlib_h__
