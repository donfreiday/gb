/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "gb.h"

#define DEBUG true

gb::gb() {
  gpu.mmu = &cpu.mmu;
  cpu.mmu.joypad = &joypad;
  gpu.reset();
}

gb::~gb() {}

bool gb::loadROM() { return cpu.mmu.load(); }

void gb::run() {
  std::set<u16> breakpoints;
  bool verbose = false;
  bool quit = false;
  SDL_Event e;

  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      } else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
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
            if (breakpoints.count(breakpoint)) {
              breakpoints.erase(breakpoint);
            } else {
              breakpoints.insert(breakpoint);
            }
            printf("Breakpoints: ");
            for (u16 b : breakpoints) {
              printf("%04X ", b);
            }
            printf("\n");
          } break;

          // Disassemble
          case SDLK_d: {
            int length = 0;
            printf("Disassemble how many bytes: ");
            scanf("%X", &length);

            for (int pc = cpu.reg.pc; pc < cpu.reg.pc + length; pc++) {
              printf("%04X: ", pc);
              u8 op = cpu.mmu.read_u8(pc);
              u16 operand;
              if (op == 0xCB) {
                operand = cpu.mmu.read_u8(++pc);
                printf("%s", cpu.instructions_CB[op].disassembly);
              } else if (cpu.instructions[op].operandLength == 1) {
                operand = cpu.mmu.read_u8(pc++);
                printf(cpu.instructions[op].disassembly, operand);
              } else if (cpu.instructions[op].operandLength == 2) {
                operand = cpu.mmu.read_u16(pc++);
                printf(cpu.instructions[op].disassembly, operand);
                pc++;
              } else {
                printf("%s", cpu.instructions[op].disassembly);
              }
              printf("   m:%d\n", (op == 0xCB)
                                      ? cpu.instructions_CB[operand].cycles / 4
                                      : cpu.instructions[op].cycles / 4);
            }
          }

          break;
          // List breakpoints
          case SDLK_l:
            printf("Breakpoints: ");
            for (u16 b : breakpoints) {
              printf("%04X ", b);
            }
            printf("\n");
            break;

          // Read memory word
          case SDLK_m: {
            unsigned int address = 0;
            printf("Memory address: ");
            scanf("%X", &address);
            printf("%04X: %04X\n", address, cpu.mmu.read_u16(address));
          } break;

          // Read memory byte
          case SDLK_n: {
            unsigned int address = 0;
            printf("Memory address: ");
            scanf("%X", &address);
            printf("%04X: %02X\n", address, cpu.mmu.read_u8(address));
          } break;

          // Display registers
          case SDLK_r:
            printf(
                "af=%04X bc=%04X de=%04X hl=%04X sp=%04X pc=%04X ime=%04x\n\n",
                cpu.reg.af, cpu.reg.bc, cpu.reg.de, cpu.reg.hl, cpu.reg.sp,
                cpu.reg.pc, cpu.ime);
            break;

          // Dump nearest 10 values on the stack
          case SDLK_s:
            for (int i = 10; i > -10; i -= 2) {
              if (cpu.reg.sp + i <= 0xFFFE && cpu.reg.sp + i >= 0) {
                printf("%04X:%04X\n", cpu.reg.sp + i,
                       cpu.mmu.read_u16(cpu.reg.sp + i));
              }
            }
            break;

          // Toggle verbose debugging
          case SDLK_v:
            verbose = !verbose;
            verbose ? printf("Verbose debugging enabled\n")
                    : printf("Verbose debugging disabled\n");
            break;

          // Run till unimplemented instruction
          case SDLK_z:
            // Printing debug info makes this really slow
            cpu.debug = true;
            cpu.debugVerbose = true;
            while (cpu.execute()) {
              gpu.step(cpu.cpu_clock_t);
              cpu.checkInterrupts();
              if (breakpoints.count(cpu.reg.pc)) {
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
            break;

          // Reset
          case SDLK_q:
            cpu.reset();
            gpu.reset();
            break;

          default:
            break;
        }
      }
    }
  }
}

int main(int argc, char* args[]) {
  gb core;
  core.loadROM();
  core.run();
  return 0;
}
