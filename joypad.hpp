// gb: a Gameboy Emulator by Don Freiday
// File: joypad.cpp
// Description: Maintains joypad state

#ifndef GB_JOYPAD
#define GB_JOYPAD

#include "common.hpp"
#include <SDL2/SDL.h>

class Joypad {
 public:
  Joypad();
  void handleEvent(SDL_Event e);
  u8 read(u8 request);
  u8 write();

 private:
  u8 buttons, directions;
};

#endif
