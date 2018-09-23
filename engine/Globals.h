
#ifndef H_GLOBALS_H
#define H_GLOBALS_H

class Globals
{
public:

	/* Global Scope Members */

	int			argc;
	char**		argv;
	bool		SHADOWS3D,		// Render Shadows
				REVERSEMS,		// Inverted Mouse Y
				SLOW,			// Slow Motion
				DEBUG,			// Developer Mode (Cheats)
				GREMEDY,		// Are we being debugged by Gremedy? (GL Debugger)
				MORPHP,			// ?
				MORPHA;			// ?
	bool		PARTICLES,
				TRANSPARENCY,
				ALPHAKEY,
				WATERANI,
				Clouds,
				SKY,
				GOURAUD,
				MODELS,
				TIMER,
				BITMAPP,
				MIPMAP,
				NOCLIP,
				CLIP3D,
				NODARKBACK,
				CORRECTION,
				LOWRESTX,
				FOGENABLE,
				FOGON,
				CAMERAINFOG,
				WATERREVERSE,
				waterclip,
				UNDERWATER,
				ONWATER,
				NeedWater,
				SNOW,
				RAIN,
				SWIM,
				FLY,
				PAUSE,
				OPTICMODE,
				BINMODE,
				EXITMODE,
				CHEATED,
				MapMode,
				RunMode,
				CrouchMode;

	/* End Global Scope Members */

	static Globals* getGlobals()
	{
		static Globals g;
		return &g;
	}

private:

	Globals()
	{
		argc = 0;
		argv = 0;
	}

};

#endif // H_GLOBALS_H
