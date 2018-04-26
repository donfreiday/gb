#ifndef GB_GPU
#define GB_GPU

#include <SDL2/SDL.h>
#include "common.h"

class GPU {
  public:
    GPU();
    ~GPU();
    void reset();

  private:
    int width;
    int height;
    SDL_Window* window;
    SDL_Surface* screenSurface;
    bool init(); // Starts up SDL and creates window
    void close(); // Shuts down SDL and frees resources
};

#endif
