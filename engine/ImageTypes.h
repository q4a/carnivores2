

#ifndef H_IMAGETYPES_H
#define H_IMAGETYPES_H

#include <stdint.h>

// -- LINUX File Types (for structs not available in Linux headers)
#if defined( AF_PLATFORM_LINUX )
#pragma pack(push, 1)

#define BI_RGB 0

typedef struct tagBITMAPFILEHEADER {
	uint16_t		bfType;
	uint32_t		bfSize;
	uint16_t		bfReserved1;
	uint16_t		bfReserved2;
	uint32_t		bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	uint32_t	biSize;
	uint32_t  	biWidth;
	uint32_t  	biHeight;
	uint16_t	biPlanes;
	uint16_t	biBitCount;
	uint32_t	biCompression;
	uint32_t	biSizeImage;
	uint32_t	biXPelsPerMeter;
	uint32_t	biYPelsPerMeter;
	uint32_t	biClrUsed;
	uint32_t	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
#pragma pack(pop)

#endif // End of Linux specific structures

enum targa_e
{
	TARGA_NONE		= 0,
	TARGA_INDEXED	= 1,
	TARGA_RGBA		= 2,
	TARGA_GRAYSCALE	= 3,
	//RLE packs

	TARGA_BIT_ORIGIN_TOPLEFT	= 16,
	TARGA_BIT_DATA_STORAGE		= 96,

	TARGA_END = 32768
};

#pragma pack(push, 1)
typedef struct TARGAFILEHEADER
{
    uint8_t		tgaIdentSize;          // size of ID field that follows 18 byte header (0 usually)
    uint8_t		tgaColorMapType;      // type of colour map 0=none, 1=has palette
    uint8_t		tgaImageType;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    uint16_t	tgaColorMapOffset;     // first colour map entry in palette
    uint16_t	tgaColorMapLength;    // number of colours in palette
    uint8_t		tgaColorMapBits;      // number of bits per palette entry 15,16,24,32

    uint16_t	tgaXStart;             // image x origin
    uint16_t	tgaYStart;             // image y origin
    uint16_t	tgaWidth;              // image width in pixels
    uint16_t	tgaHeight;             // image height in pixels
    uint8_t		tgaBits;               // image bits per pixel 8,16,24,32
    uint8_t		tgaDescriptor;         // image descriptor bits (vh flip bits)
} TARGAFILEHEADER;

enum wavefile_e
{
	// String Identifiers as uint32_t integers
	WAVE_CHUNKID		= 0x52494646,
	WAVE_FORMATID		= 0x57415645,
	WAVE_HEADERID		= 0x666d7420,
	WAVE_DATAID			= 0x64617461,

	// Actual format specifiers
	WAVE_FORMAT_NONE	= 0,
	WAVE_FORMAT_PCM		= 1,

	// Channels
	WAVE_CHANNELS_MONO		= 1,
	WAVE_CHANNELS_STEREO	= 2,
	WAVE_CHANNELS_QUAD		= 4,
	WAVE_CHANNELS_SURROUND51= 5,
	WAVE_CHANNELS_SURROUND71= 7,
	// TODO: 5.1 and 7.1 support?

	// Bits Per Sample
	WAVE_BPS_8BIT		= 8,
	WAVE_BPS_16BIT		= 16,
	WAVE_BPS_24BIT		= 24,
	WAVE_BPS_32BIT		= 32
};

typedef struct WAVEFILEFORMAT
{
	uint32_t	wavChunkID;		// "RIFF"
	uint32_t	wavChunkSize;	// FileSize - 8
	uint32_t	wavFormat;		// "WAVE"

	uint32_t	wavHeaderID;	// "fmt "
	uint32_t	wavHeaderSize;	// HeaderSize;
	uint16_t	wavType;		// 1==PCM
	uint16_t	wavChannels;	// 1==Mono, 2==Stereo
	uint32_t	wavSampleRate;
	uint32_t	wavByteRate;	// SampleRate * NumChannels * BitsPerSample/8
	uint16_t	wavBlockAlign;	// Channels * BitsPerSample / 8;
	uint16_t	wavBitsPerSample;

	uint32_t	wavDataID;		// "data"
	uint32_t	wavDataLength;
} WAVEFILEFORMAT;

#pragma pack(pop)

#endif // H_IMAGETYPES_H
