/* gb: a Gameboy Emulator
   Author: Don Freiday */

#include "gb.h"

// curses color pairs
#define CYAN 1
#define GREEN 2
#define WHITE 3

gb::gb() {
  // Init debugger
  debugEnabled = true;
  runToBreak = false;
  cursorPos = 0;

  // Init curses
  initscr();
  cbreak();              // Disable TTY buffering, one character at a time
  noecho();              // Suppress echoing of typed characters
  keypad(stdscr, true);  // Capture special keys
  start_color();
  init_pair(CYAN, COLOR_CYAN , COLOR_BLACK);
  init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
  getmaxyx(stdscr, yMax, xMax);

  // Init core
  gpu.mmu = &cpu.mmu;
  cpu.mmu.joypad = &joypad;
  gpu.reset();
}

gb::~gb() {
  endwin();  // End curses mode
}

bool gb::loadROM() { return cpu.mmu.load(); }

void gb::debug() {
  getmaxyx(stdscr, yMax, xMax);
  if (runToBreak) {
    if (std::find(breakpoints.begin(), breakpoints.end(), cpu.reg.pc) != breakpoints.end()) {
      runToBreak = false;
      cursorPos = getDisasmIndex(
          cpu.reg.pc);  // Snap cursor to last executed instruction
      display();
      return;
    }
    step();
    return;
  }

  switch (getch()) {
    // Breakpoint
    case KEY_F(2): {
      if (std::find(breakpoints.begin(), breakpoints.end(), disasm[cursorPos].pc) != breakpoints.end()) {
        breakpoints.erase(disasm[cursorPos].pc);
      } else {
        breakpoints.insert(disasm[cursorPos].pc);
      }
      break;
    }

    // Step
    case KEY_F(7):
      step();
      cursorPos = getDisasmIndex(
          cpu.reg.pc);  // Snap cursor to last executed instruction
      break;

    // Run to break
    case KEY_F(9):
      runToBreak = true;
      break;

    case KEY_UP:
      cursorPos--;
      break;

    case KEY_PPAGE:  // pageup
      cursorPos -= yMax + 2;
      break;

    case KEY_DOWN:
      cursorPos++;
      break;

    case KEY_NPAGE:  // pagedown
      cursorPos += yMax - 2;
      break;

    default:
      break;
  }
  display();
}

void gb::step() {
  cpu.execute();
  gpu.step(cpu.cpu_clock_t);
  cpu.checkInterrupts();
}

// Find current PC in disassembly
// todo: rethink this, use a better search algorithm / data structure
int gb::getDisasmIndex(u16 pc) {
  std::vector<disassembly>::iterator it; 
  it = find(disasm.begin(), disasm.end(), pc);
  return std::distance(disasm.begin(), it);
}

// Use curses to print disassembly, registers, etc
// todo: overhaul, partial screen draws, etc
void gb::display() {
  clear();  // Clear screen

  // Calculate bounds of disassembly display
  int index = getDisasmIndex(cpu.reg.pc);
  int start = index - (yMax / 2);
  int end = index + (yMax / 2);

  start += cursorPos;
  end += cursorPos;

  if (start <= 0) {
    end -= start;
    start = 0;
  }

  if (end > (int)disasm.size()) {
    end = disasm.size();
  }

  // Print disassembly
  for (int i = start; i < end; i++) {
    attroff(A_STANDOUT | A_BOLD);
    attron(COLOR_PAIR(WHITE));

    // Cursor position is highlighted
    if (cursorPos == i) {
      attron(A_STANDOUT);
    }

    // Current PC is bold
    if (disasm[i].pc == cpu.reg.pc) {
      attron(A_BOLD);
    }

    // Breakpoints are red
    if (std::find(breakpoints.begin(), breakpoints.end(), disasm[i].pc)!=breakpoints.end()) {
      attron(COLOR_PAIR(CYAN));
    }

    printw("%04X: ", disasm[i].pc);
    if (disasm[i].operandSize > 0) {
      printw(disasm[i].str.c_str(), disasm[i].operand);
    } else {
      printw(disasm[i].str.c_str());
    }
    printw("\n");
  }

  // Registers
  attron(COLOR_PAIR(WHITE));
  int x = xMax / 2;
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
  u16 operand, pc = 0;
  u8 op;
  while (pc < cpu.mmu.getRomSize()) {
    disassembly d;
    d.pc = pc;
    op = cpu.mmu.read_u8(pc);
    if (op == 0xCB) {
      d.operandSize = 0;
      operand = cpu.mmu.read_u8(++pc);
      d.str = cpu.instructions_CB[operand].disassembly;
    } else if (cpu.instructions[op].operandLength == 1) {
      d.operandSize = 1;
      d.operand = cpu.mmu.read_u8(++pc);
      d.str = cpu.instructions[op].disassembly;
    } else if (cpu.instructions[op].operandLength == 2) {
      d.operandSize = 2;
      d.operand = cpu.mmu.read_u16(++pc);
      d.str = cpu.instructions[op].disassembly;
      pc++;
    } else {
      d.operandSize = 0;
      d.str = cpu.instructions[op].disassembly;
    }
    pc++;
    disasm.push_back(d);
  }
}

void gb::run() {
  bool quit = false;
  SDL_Event e;

  if (debugEnabled) {
    disassemble();
    display();
  }

  while (!quit) {
    if (debugEnabled) {
      debug();
    } else {
      step();
    }
    while (SDL_PollEvent(&e) != 0) {
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
