// gb: a Gameboy Emulator by Don Freiday
// File: gpu.h
// Description: Emulates the PPU and LCD
//
// Graphics are rendered using the Simple DirectMedia Layer library (SDL 2.0)

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
  int mode;
  u8 scanline;

 private:
  u8 screenData[144][160][3];
  SDL_Window* window;
  SDL_GLContext mainContext;

  int width;
  int height;

  bool initSDL();  // Starts up SDL and creates window
  void initGL();

  int frameStartTime, frameCurrentTime;

  void renderScanline();  // write scanline to surface
  void renderBackground();
  void renderSprites();

  enum COLOR { WHITE, LIGHT_GRAY, DARK_GRAY, BLACK };

  COLOR paletteLookup(u8 colorID, u16 address);

  void requestInterrupt(u8 interrupt);
};

#endif
