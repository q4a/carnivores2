
#ifndef VIDEOOPTIONS_H
#define VIDEOOPTIONS_H


class VideoOptions
{
public:

	int				mWidth, mHeight;
	int				mBackBuffer, mZBuffer;

	unsigned int	mAntialiasingLevels;
	float			mAnisotropyLevels;
	bool			mVSync,
					mTrilinearFiltering,
					mBilinearFiltering,
					mNearestFiltering,
					mSpecularLighting,		// Light highlights, like the sun shining off metal.
					mLighting,				// Lighting in general
					mEnvironmentMapping;	// Cubemapping/Spheremapping  (reflective mapping)

	VideoOptions()
	{
		this->mAntialiasingLevels = 0;
		this->mAnisotropyLevels = 0.0f;
	}
};


class GlobalVideoOptions
{
private:
public:

	static VideoOptions* SharedVariable()
	{
		static VideoOptions g;
		return &g;
	}
};

#endif // VIDEOOPTIONS_H