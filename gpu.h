#ifndef GB_GPU
#define GB_GPU

#include <SDL2/SDL.h>
#include "common.h"

class GPU {
  public:
    GPU();
    ~GPU();
    void reset();
    void step(int cpu_clock); // clock step

  private:
    int width;
    int height;
    int mode;
    int modeclock;
    int line;

    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Surface* backSurface;
    bool init(); // Starts up SDL and creates window
    void close(); // Shuts down SDL and frees resources
    void renderscan(); // write scanline to surface
    void swapsurface(); // swap SDL surfaces
};

#endif
