
#include "Hunt.h"


enum
{
    // Enum Starts at 1000, for menu scripting
    MENU_SPLASH = 1000,
    MENU_PROFILE,
    MENU_PROFILECREATE,
    MENU_DISCLAIMER,
    MENU_MAIN,
    MENU_HUNT,
    MENU_SURVIVAL,
    MENU_MULTIPLAYER,
    MENU_TROPHY,
    MENU_OPTIONS,
    MENU_CREDITS
};


class CMenuButton
{
public:
    uint32_t	x,
				y,
				w,
				h,
				id;
    TPicture    pic,
				pic_on;
};


uint32_t		ButtonCount = 0;
uint32_t		MenuState = MENU_SPLASH;
TPicture    	MenuBackground;
CMenuButton 	*MenuButton;


void MenuBegin()
{
    LoadPictureTGA(MenuBackground, "MENUM.TGA");
}

void MenuLoop()
{

}
