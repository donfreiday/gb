/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "gb.h"

gb::gb() {
  debugEnabled = true;
  initscr();  // Initialize curses
  gpu.mmu = &cpu.mmu;
  cpu.mmu.joypad = &joypad;
  gpu.reset();
  disasmStartAddr = cpu.reg.pc;
}

gb::~gb() {
  endwin();  // End curses mode
}

bool gb::loadROM() { return cpu.mmu.load(); }

void gb::debug() {
  cbreak();              // Disable TTY buffering, one character at a time
  noecho();              // Suppress echoing of typed characters
  keypad(stdscr, true);  // Capture special keys

  switch (getch()) {
    case KEY_F(7):
      disassemble();
      step();
      display();
      break;

    case KEY_UP:
      
      break;
    
    case KEY_DOWN:
      break;

    default:
      break;
  }
}

void gb::step() {
  cpu.execute();
  gpu.step(cpu.cpu_clock_t);
  cpu.checkInterrupts();
}

// Use curses to print registers, etc
void gb::display() {
  int x = 40;
  int y = 0;
  mvprintw(y++, x, "af= %04X", cpu.reg.af);
  mvprintw(y++, x, "bc= %04X", cpu.reg.bc);
  mvprintw(y++, x, "de= %04X", cpu.reg.de);
  mvprintw(y++, x, "hl= %04X", cpu.reg.hl);
  mvprintw(y++, x, "sp= %04X", cpu.reg.sp);
  mvprintw(y++, x, "pc= %04X", cpu.reg.pc);
  mvprintw(y++, x, "ime=%c", cpu.ime ? 1 : '.');
  mvprintw(y++, x, "ima=todo");
  y = 0;
  x += 11;
  mvprintw(y++, x, "lcdc=%02X", cpu.mmu.memory[LCD_CTL]);
  mvprintw(y++, x, "stat=%02X", cpu.mmu.memory[LCD_STAT]);
  mvprintw(y++, x, "ly=%02X", cpu.mmu.memory[LCD_SCROLLY]);
  mvprintw(y++, x, "if=%02X", cpu.mmu.memory[CPU_INTERRUPT_FLAG]);
  refresh();
}

void gb::disassemble() {
  getmaxyx(stdscr, rows, cols);  // Update screen boundaries
  u16 pc = disasmStartAddr;
  u16 operand;
  u8 op;
  for (int i = 0; i < rows; i++) {
    (pc == cpu.reg.pc) ? attron(A_STANDOUT) : attroff(A_STANDOUT); // Highlight currently executing line
    mvprintw(i, 0, "%04X: ", pc);
    op = cpu.mmu.read_u8(pc);
    if (op == 0xCB) {
      operand = cpu.mmu.read_u8(++pc);
      printw("%s", cpu.instructions_CB[op].disassembly);
    } else if (cpu.instructions[op].operandLength == 1) {
      operand = cpu.mmu.read_u8(++pc);
      printw(cpu.instructions[op].disassembly, operand);
    } else if (cpu.instructions[op].operandLength == 2) {
      operand = cpu.mmu.read_u16(++pc);
      printw(cpu.instructions[op].disassembly, operand);
      pc++;
    } else {
      printw("%s", cpu.instructions[op].disassembly);
    }
    pc++;
  }
  refresh();
}

/*case SDLK_d: {
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
}*/

void gb::run() {
  bool quit = false;
  SDL_Event e;

  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      if (debugEnabled) {
        debug();
      } else {
        step();
      }

      switch (e.type) {
        case SDL_QUIT:
          quit = true;
          break;

        case SDL_KEYDOWN:
          handleSDLKeydown(e.key.keysym.sym);
          break;

        default:
          break;
      }

      /*// Toggle breakpoint
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
    }*/
    }
  }
}

void gb::handleSDLKeydown(SDL_Keycode key) {
  switch (key) {
    case SDLK_d:
      debugEnabled = !debugEnabled;
      break;

    case SDLK_UP:
      printw("UP\n");
      break;

    case SDLK_DOWN:
      printw("DOWN\n");
      break;

    case SDLK_RIGHT:
      printw("RIGHT\n");
      break;

    case SDLK_LEFT:
      printw("LEFT\n");
      break;

    default:
      break;
  }
}

int main(int argc, char* args[]) {
  gb core;
  core.loadROM();
  core.run();
  return 0;
}
