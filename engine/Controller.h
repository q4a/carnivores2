
#ifndef H_XINPUT
#define H_XINPUT

#define MAX_CONTROLLERS 1
#define INPUT_DEADZONE  ( 0.24f )  // Default to 24% of the +/- 32767 range.

// Defines for Xbox 360 Controller Indices
#define XBOX_PAD_A			0
#define XBOX_PAD_B			1
#define XBOX_PAD_X			2
#define XBOX_PAD_Y			3
#define XBOX_PAD_LB			4
#define XBOX_PAD_RB			5
#define XBOX_PAD_BACK		6
#define XBOX_PAD_START		7
#define XBOX_PAD_XBOXMENU	8
#define XBOX_PAD_LSTICK		9
#define XBOX_PAD_RSTICK		10

#define XBOX_PAD_LEFTX		0
#define XBOX_PAD_LEFTY		1
#define XBOX_PAD_LTRIGGER	2
#define XBOX_PAD_RIGHTX		3
#define XBOX_PAD_RIGHTY		4
#define XBOX_PAD_RTRIGGER	5
#define XBOX_PAD_DPADX		6
#define XBOX_PAD_DPADY		7

struct CONTROLLER_STATE
{
    uint8_t			lastState[20];
    uint8_t			state[20];
    float			m_axes[10];
	//XINPUT_VIBRATION vibration;
	float 		leftRumble;
	float 		rightRumble;
	float 		timeRumble;
	bool 		bConnected;
	bool 		NoSNDInput;
	bool 		NoSNDOutput;
	uint32_t	dwResult;
	uint32_t	dwPacketNumber;
};

_EXTORNOT CONTROLLER_STATE gController;

#endif

