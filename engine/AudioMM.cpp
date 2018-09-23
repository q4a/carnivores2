
#include "AudioDriver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>		// -- Needed for some defines >.>
#include <mmsystem.h>
#include <ObjBase.h>


HWAVEOUT		g_WaveOut;
WAVEHDR			g_WaveHeader;
WAVEFORMATEX	g_WaveFormat;
uint32_t		g_NumberOfDevices;

void fprintGUID( FILE* f, GUID *g )
{
	fputc( '{', f );
	fprintf( f, "%08X-", g->Data1 );
	fprintf( f, "%04X-", g->Data2 );
	fprintf( f, "%04X-", g->Data3 );
	for ( int c=0; c<8; c++ )
	{
		fprintf( f, "%X", g->Data4[c] );
	}
	fputc( '}', f );
	fputc( '\n', f );
}

void waveDeviceList( FILE* p_log )
{
	g_NumberOfDevices = waveOutGetNumDevs();
	fprintf( p_log, "\tNumber of Devices: %u\n", g_NumberOfDevices );

	for ( uint32_t i=0; i<g_NumberOfDevices; i++ )
	{
		WAVEOUTCAPS2 woCaps;

		waveOutGetDevCaps( i, (WAVEOUTCAPS*)&woCaps, sizeof(WAVEOUTCAPS2) );

		fprintf( p_log, "\tdevice( id:%02u, \"%s\" )\n\t{\n", i, woCaps.szPname );

		fprintf( p_log, "\t\tVersion: %u.%u\n", woCaps.vDriverVersion & 0xFF, (woCaps.vDriverVersion>>8) & 0xFF );

		fprintf( p_log, "\t\tName GUID: " );
		fprintGUID( p_log, &woCaps.NameGuid );
		fprintf( p_log, "\t\tManafacturer GUID: " );
		fprintGUID( p_log, &woCaps.ManufacturerGuid );
		fprintf( p_log, "\t\tProduct GUID: " );
		fprintGUID( p_log, &woCaps.ProductGuid );

		fprintf( p_log, "\t\tChannels: %u\n", woCaps.wChannels );

		fprintf( p_log, "\t\tMono 8-Bit:\n");
		if ( woCaps.dwFormats & WAVE_FORMAT_1M08 ) fprintf( p_log, "\t\t  11.025khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_2M08 ) fprintf( p_log, "\t\t  22.05khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_4M08 ) fprintf( p_log, "\t\t  44.1khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_96M08 ) fprintf( p_log, "\t\t  96khz\n" );
		fprintf( p_log, "\t\tStereo 8-Bit:\n");
		if ( woCaps.dwFormats & WAVE_FORMAT_1S08 ) fprintf( p_log, "\t\t  11.025khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_2S08 ) fprintf( p_log, "\t\t  22.05khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_4S08 ) fprintf( p_log, "\t\t  44.1khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_96S08 ) fprintf( p_log, "\t\t  96khz\n" );

		fprintf( p_log, "\t\tMono 16-Bit:\n");
		if ( woCaps.dwFormats & WAVE_FORMAT_1M16 ) fprintf( p_log, "\t\t  11.025khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_2M16 ) fprintf( p_log, "\t\t  22.05khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_4M16 ) fprintf( p_log, "\t\t  44.1khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_96M16 ) fprintf( p_log, "\t\t  96khz\n" );
		fprintf( p_log, "\t\tStereo 16-Bit:\n");
		if ( woCaps.dwFormats & WAVE_FORMAT_1S16 ) fprintf( p_log, "\t\t  11.025khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_2S16 ) fprintf( p_log, "\t\t  22.05khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_4S16 ) fprintf( p_log, "\t\t  44.1khz\n" );
		if ( woCaps.dwFormats & WAVE_FORMAT_96S16 ) fprintf( p_log, "\t\t  96khz\n" );

		fprintf( p_log, "\t}\n" );
	}
}


bool AudioDriver::init( int16_t* p_buffer, size_t p_length, uint8_t p_voices )
{
	MMRESULT mmRes;
	WAVEOUTCAPS woCaps;

	this->m_voices = p_voices;

	g_WaveFormat.wFormatTag			= WAVE_FORMAT_PCM;
	g_WaveFormat.wBitsPerSample		= 16;
	g_WaveFormat.nChannels			= 2;
	g_WaveFormat.nSamplesPerSec		= 22050;
	g_WaveFormat.nBlockAlign		= (g_WaveFormat.nChannels * g_WaveFormat.wBitsPerSample) / 8;
	g_WaveFormat.nAvgBytesPerSec	= g_WaveFormat.nSamplesPerSec * g_WaveFormat.nBlockAlign;
	g_WaveFormat.cbSize				= 0;

	this->m_channels = g_WaveFormat.nChannels;
	this->m_samplerate = g_WaveFormat.nSamplesPerSec;

	// -- Discover the number of available output WaveForm devices
	waveDeviceList( m_stream );

	if ( g_NumberOfDevices == 0 )
	{
		fprintf( m_stream,  "ERROR -> No WaveForm devices to play through!\n" );
		return false;
	}

	// -- Query for our desired output WaveForm device
	// ...
	mmRes = waveOutOpen( &g_WaveOut, WAVE_MAPPER, &g_WaveFormat, NULL, NULL, CALLBACK_NULL );
	if ( FAILED(mmRes) )
	{
		fprintf( m_stream, "ERROR -> Failed to open the output WaveForm device!\n" );
		return false;
	}

	// -- Get the Device Capabilities
	mmRes = waveOutGetDevCaps( (UINT_PTR)g_WaveOut, &woCaps, sizeof( WAVEOUTCAPS ) );
	if ( FAILED(mmRes) )
	{
		fprintf( m_stream,  "ERROR -> Failed to retrieve the output WaveForm device's capabilities!\n" );
		return false;
	}

	fprintf( m_stream, "\tDevice Name: %s\n", woCaps.szPname );
	fprintf( m_stream, "\tSupport Wave 16-bit Stereo 22050hz: %s\n", (woCaps.dwFormats & WAVE_FORMAT_2M16)?"Yes":"No" );
	fprintf( m_stream, "\tSupport Volume: %s\n", (woCaps.dwSupport & WAVECAPS_VOLUME)?"Yes":"No" );
	fprintf( m_stream, "\tSupport LRVolume: %s\n", (woCaps.dwSupport & WAVECAPS_LRVOLUME)?"Yes":"No" );

	// -- Prepare the device for output
	memset( &g_WaveHeader, 0, sizeof( WAVEHDR ) );

	g_WaveHeader.lpData = (LPSTR)p_buffer;
	g_WaveHeader.dwBufferLength = p_length;

	mmRes = waveOutPrepareHeader( g_WaveOut, &g_WaveHeader, sizeof( WAVEHDR ) );
	if ( FAILED(mmRes) )
	{
		fprintf( m_stream,  "ERROR -> Failed to prepare the specified WAVE header!\n" );
		return false;
	}

	g_AudioThread = CreateThread( 0, 0, &ProcessAudioThread, 0, 0, (LPDWORD)&g_AudioThreadID );
	if ( !g_AudioThread )
	{
		fprintf( haudiolog, "\tERROR -> Failed to create the audio processing thread!\n" );
	}

	return true;
}

bool AudioDriver::shutdown()
{
	MMRESULT mmRes;

	WaitForSingleObject( g_AudioThread, INFINITE );

	mmRes = waveOutUnprepareHeader( g_WaveOut, &g_WaveHeader, sizeof(WAVEHDR) );
	if ( FAILED(mmRes) )
	{
		fprintf( m_stream,  "Failed to unprepare the wave header!\n" );
		return false;
	}

	mmRes = waveOutClose( g_WaveOut );
	if ( FAILED(mmRes) )
	{
		fprintf( m_stream,  "Failed to close the WaveForm device!\n" );
		return false;
	}

	return true;
}

bool AudioDriver::process( )
{
	MMRESULT mmRes = S_OK;

	if ( FAILED( mmRes = waveOutWrite( g_WaveOut, &g_WaveHeader, sizeof(WAVEHDR) ) ) )
	{
		fprintf( m_stream, "Failed to write to the output buffer!\n" );
		return false;
	}

	// -- Wait for the buffer to finish
	/*while( !(g_WaveHeader.dwFlags & WHDR_DONE) )
	{
		// -- wait
	}*/

	return true;
}

const char* AudioDriver::getName()
{
	return "Software Mixer by Alexey Menshikov";
}
