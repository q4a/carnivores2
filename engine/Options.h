
#ifndef H_OPTIONS_H
#define H_OPTIONS_H

#include <stdint.h>


enum texturefilters_e {
	FILTER_POINT,
	FILTER_LINEAR,
	FILTER_BILINEAR,
	FILTER_TRILINEAR,
	FILTER_ANISOTROPIC
};


class Options
{
public:

	uint32_t		mWidth,
					mHeight;
	uint8_t			mBackBuffer,
					mZBuffer;

	unsigned int	mAntialiasingLevels;
	float			mAnisotropyLevels;
	bool			mVSync,
					mLighting,				// Lighting in general
					mSpecularLighting,		// Light highlights, like the sun shining off metal.
					mEnvironmentMapping;	// Cubemapping/Spheremapping  (reflective mapping)
	uint32_t		mTextureFiltering;

	uint8_t			mAudioChannels;			// 1,2,4,8,16,32,64,128

	Options()
	{
		this->mAntialiasingLevels = 0;
		this->mAnisotropyLevels = 1.0f;
	}
};


#endif // H_OPTIONS_H
