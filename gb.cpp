/* gb: a Gameboy Emulator
   Author: Don Freiday
   todo: everything */

#include <SDL/SDL.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "gpu.h"
#include <set>

#define HELP "z: run\nx: step\nm: read memory\nr: registers\ns: stack\nb: toggle breakpoint on PC\nl: list breakpoints\nv: toggle verbose debugging\nh: help\n\n"
#define TITLE "\n+------------------------+\n| gb: A Gameboy Emulator |\n+------------------------+\n"

int main(int argc, char* args[]) {
  printf(TITLE);
  if (argc < 2) {
    printf("Usage: gb <rom.gb>\n");
    return 0;
  }
  CPU cpu;
  GPU gpu(cpu.mmu);
  if (!cpu.mmu.load(args[1])) {
    printf("Couldn't load %s\n", args[1]);
    return -1;
  }
  printf(HELP);

  std::set<u16> breakpoints;
  bool verbose = false;
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

          // Toggle breakpoint
          case SDLK_b: {
            unsigned int breakpoint = 0xFFFF;
            printf("Toggle breakpoint on PC: ");
            scanf("%X", &breakpoint);
            if(breakpoints.count(breakpoint)) {
              breakpoints.erase(breakpoint);
            }
            else {
              breakpoints.insert(breakpoint);
            }
            printf("Breakpoints: ");
            for(u16 b : breakpoints) {
              printf("%04X ", b);
            }
            printf("\n");
          }
          break;

          // Help
          case SDLK_h:
              printf(HELP);
          break;

          // List breakpoints
          case SDLK_l:
            printf("Breakpoints: ");
            for(u16 b : breakpoints) {
              printf("%04X ", b);
            }
            printf("\n");
          break;

          // Dump memory address
          case SDLK_m: {
            unsigned int address = 0;
            printf("Memory address: ");
            scanf("%X", &address);
            printf("%04X: %04X\n", address, cpu.mmu.read_u16(address));
          }
          break;

          // Display registers
          case SDLK_r:
            printf("af=%04X bc=%04X de=%04X hl=%04X sp=%04X pc=%04X ime=%04x\n\n", cpu.reg.af, cpu.reg.bc, cpu.reg.de, cpu.reg.hl, cpu.reg.sp, cpu.reg.pc, cpu.interrupt);
          break;

          // Dump nearest 10 values on the stack
          case SDLK_s:
            for(int i=10; i>-10; i-=2) {
              if(cpu.reg.sp+i <= 0xFFFE && cpu.reg.sp+i >= 0) {
                printf("%04X:%04X\n", cpu.reg.sp+i, cpu.mmu.read_u16(cpu.reg.sp+i));
              }
            }
          break;

          // Toggle verbose debugging
          case SDLK_v:
            verbose = !verbose;
            verbose ? printf("Verbose debugging enabled\n") : printf("Verbose debugging disabled\n");
          break;

          // Run till unimplemented instruction
          case SDLK_z:
            // Printing debug info makes this really slow
            cpu.debug = verbose;
            cpu.debugVerbose = verbose;
            while (cpu.execute()) {
              gpu.step(cpu.cpu_clock_t);
              cpu.checkInterrupts();
              if(breakpoints.count(cpu.reg.pc)) {
                printf("%04X: breakpoint\n", cpu.reg.pc);
                break;
              }
            }
          break;

          // Execute
          case SDLK_x:
            cpu.debug = true;
            cpu.debugVerbose = true;
            cpu.execute();
            gpu.step(cpu.cpu_clock_t);
            cpu.checkInterrupts();
            gpu.renderScreen();
          break;

          default:
          break;
        }
      }
    }
  }
  return 0;
}
