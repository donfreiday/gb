#include "gpu.h"

GPU::GPU() {
  reset();
}

GPU::~GPU() {
  close();
}

void GPU::reset() {
  width = 166;
  height = 144;
  mode = 0;
  modeclock = 0;
  line = 0;
  init();
}

// Starts up SDL and creates window
bool GPU::init() {
  if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL failed to initialize! SDL_Error: %s\n", SDL_GetError());
    return false;
  }
  window = SDL_CreateWindow("gb", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
  screenSurface = SDL_GetWindowSurface(window);
  backSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0, 0, 0, 0);
  SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) ); // Fill the screen with white
  SDL_UpdateWindowSurface( window );
  return true;
}

// Shuts down SDL and frees resources
void GPU::close() {
  SDL_DestroyWindow(window);
  window = NULL;
  SDL_Quit();
}

// Clock step
void GPU::step(int cpu_clock) {
  modeclock += cpu_clock;
  switch(mode) {
    // OAM read mode, scanline active
    case 2:
    if(modeclock>=80) {
      // scanline mode 3
      modeclock = 0;
      mode = 0;
    }
    break;

    // VRAM read mode, scanline active
    case 3:
    if(modeclock >= 172) {
      // hblank
      modeclock = 0;
      mode = 0;

      // write scanline to surface
      renderscan();
    }
    break;

    // hblank, swap surfaces after last hblank
    case 0:
    if (modeclock >= 204) {
      modeclock = 0;
      line++;
      if (line == 143) {
        // vblank
        mode = 1;
        swapsurface();
      }
    }
  }
}

// Write scanline to framebuffer
void GPU::renderscan() {

}

// Swap SDL buffers
void GPU::swapsurface() {

}
