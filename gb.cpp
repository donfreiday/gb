/* gb: a Gameboy Emulator
   Author: Don Freiday
   todo: everything */

#include <SDL2/SDL.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "gpu.h"

int main(int argc, char* args[]) {
  printf("\n+------------------------+\n| gb: A Gameboy Emulator |\n+------------------------+\n");
  if (argc < 2) {
    printf("Usage: gb <rom.gb>\n");
    return 0;
  }
  CPU cpu;
  GPU gpu;
  if (!cpu.mmu.load(args[1])) {
    printf("Couldn't load %s\n", args[1]);
    return -1;
  }

  // Run until X is pressed on window
  bool quit = false;
  SDL_Event e;
  while(!quit) {
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT) {
        quit = true;
      }
      else if (e.type == SDL_KEYDOWN) {
        switch(e.key.keysym.sym) {
          case SDLK_UP:
          printf("UP\n");
          break;

          case SDLK_DOWN:
          printf("DOWN\n");
          break;

          case SDLK_RIGHT:
          printf("RIGHT\n");
          break;

          case SDLK_LEFT:
          printf("LEFT\n");
          break;

          case SDLK_z:
          while (cpu.execute()) {
            printf("af=%04X\nbc=%04X\nde=%04X\nhl=%04X\nsp=%04X\npc=%04X\nime=%04x\n\n", cpu.reg.af, cpu.reg.bc, cpu.reg.de, cpu.reg.hl, cpu.reg.sp, cpu.reg.pc, cpu.interrupt);
            gpu.step(cpu.cpu_clock_t);
            cpu.checkInterrupts();

          }
          break;

          case SDLK_x:
          // Fetch the next opcode and increment pc
          cpu.execute();
          printf("af=%04X\nbc=%04X\nde=%04X\nhl=%04X\nsp=%04X\npc=%04X\nime=%04x\n\n", cpu.reg.af, cpu.reg.bc, cpu.reg.de, cpu.reg.hl, cpu.reg.sp, cpu.reg.pc, cpu.interrupt);
          gpu.step(cpu.cpu_clock_t);
          cpu.checkInterrupts();
          break;

          default:
          break;
        }
      }
    }
  }
  return 0;
}
