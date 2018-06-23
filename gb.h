#ifndef GB_CORE
#define GB_CORE

#include <SDL2/SDL.h>
#include <stdio.h>
#include <set>
#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "joypad.h"

class gb {
 public:
  gb();
  ~gb();

  bool loadROM();
  void run();

 private:
  CPU cpu;
  GPU gpu;
  Joypad joypad;
};

#endif
