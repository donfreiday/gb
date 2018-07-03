#ifndef GB_JOYPAD
#define GB_JOYPAD

#include "common.h"
#include <SDL2/SDL.h>

class Joypad {
 public:
  Joypad();
  void keyPressed(SDL_Keycode key);
  void keyReleased(SDL_Keycode key);
  u8 read(u8 request);
  u8 write();

 private:
  u8 buttons, directions;
};

#endif
