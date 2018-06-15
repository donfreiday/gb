#ifndef GB_JOYPAD
#define GB_JOYPAD

#include "common.h"

class Joypad {
public:
  Joypad();
  void keyPressed(u8 key);
  void keyReleased(u8 key);
  u8 getState();

private:
  u8 state;
};

#endif
