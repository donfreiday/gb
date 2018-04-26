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
  return true;
}

// Shuts down SDL and frees resources
void GPU::close() {
  SDL_DestroyWindow(window);
  window = NULL;
  SDL_Quit();
}
