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
    void step(u8 cycles); // clock step
    void renderScreen();

  private:
    MMU* mmu;

    u8 screenData[144][160][3];

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

    enum COLOR
		{
			WHITE,
			LIGHT_GRAY,
			DARK_GRAY,
			BLACK
		};

    COLOR paletteLookup(u8 colorID, u16 address);
};

#endif
