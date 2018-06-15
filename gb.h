#ifndef GB_CORE
#define GB_CORE

#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "joypad.h"
#include <SDL/SDL.h>
#include <stdio.h>
#include <set>

class gb {
public:
  gb();
  ~gb();

  bool loadROM(char* file);
  void run();

  CPU cpu;
  GPU gpu;
  Joypad joypad;

};


#endif
