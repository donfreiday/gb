#ifndef GB_GPU
#define GB_GPU

#include <SDL2/SDL.h>
#include "common.h"
#include "mmu.h"

class GPU {
  public:
    GPU(MMU &mem);
    ~GPU();
    void reset();
    void step(int cycles); // clock step

  private:
    MMU mmu;

    int width;
    int height;
    int lines;

    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Surface* backSurface;
    bool init(); // Starts up SDL and creates window
    void close(); // Shuts down SDL and frees resources
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
