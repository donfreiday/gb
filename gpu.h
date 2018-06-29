/* gb: a Gameboy Emulator
   Author: Don Freiday */

#ifndef GB_GPU
#define GB_GPU

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "common.h"
#include "mmu.h"

class GPU {
 public:
  GPU();
  ~GPU();

  void reset();
  void step(u8 cycles);  // clock step
  void renderScreen();

  MMU* mmu;
  int modeclock;

 private:
  u8 screenData[144][160][3];
  SDL_Window* window;
  SDL_GLContext mainContext;

  int width;
  int height;

  int mode;
  u8 scanline;

  bool initSDL();  // Starts up SDL and creates window
  void initGL();

  void renderScanline();  // write scanline to surface
  void renderBackground();
  void renderSprites();

  enum COLOR { WHITE, LIGHT_GRAY, DARK_GRAY, BLACK };

  COLOR paletteLookup(u8 colorID, u16 address);

  void requestInterrupt(u8 interrupt);
};

#endif
