/* gb: a Gameboy Emulator
   Author: Don Freiday */

#ifndef GB_GPU
#define GB_GPU

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "common.h"
#include "mmu.h"

class GPU {
  public:
    GPU(MMU& mem);
    ~GPU();
    void reset();
    void step(int cycles); // clock step
    void renderScreen();

    void dumpTile();

  private:
    MMU* mmu;

    int width;
    int height;

    int mode;
    int modeclock;
    int scanline;

    bool initSDL(); // Starts up SDL and creates window
    void initGL();

    void renderScanline(); // write scanline to surface
    void renderBackground();
    void renderSprites();
    void swapsurface(); // swap SDL surfaces



    enum COLOR
		{
			WHITE,
			LIGHT_GRAY,
			DARK_GRAY,
			BLACK
		};

    COLOR paletteLookup(u8 colorID, u16 address);

    u8 screenData[144][160][3];

    // LCD
    void updateLCD();
    bool lcdEnabled();

};

#endif
