/************************************************************************
* Audio Driver Base Class
* Version:	1.1
* By:		James Ogden
*/

#ifndef __pcmlib_h__
#define __pcmlib_h__

#include <stdio.h>
#include <stdint.h>

typedef struct sound_s *sound_p;


class AudioDriver
{
public:

	AudioDriver( FILE* fp = stderr ) :
	  m_stream( fp )
	{}

	/* Initialise the Audio Driver */
	virtual bool init( int16_t* p_buffer = 0, size_t p_length = 0, uint8_t p_voices = 16 );

	/* Shutdown the Audio Driver */
	virtual bool shutdown();

	/* Process an audio stream, wait for the buffer to complete playing */
	virtual bool process( );

	/* Get the driver name */
	virtual const char* getName();

	/* Load a sound raw PCM or MS-WAVE */
	virtual bool	loadSound( sound_t* p_snd, const char* p_file );
	virtual void	freeSound( sound_t* p_snd );

	////////////////////////////////////////////////////////////////////////////////////////////
	// Audio Driver Properties
	uint16_t	m_samplerate;
	uint8_t		m_channels;
	uint8_t		m_voices;
	FILE*		m_stream;
};

#endif //__pcmlib_h__
