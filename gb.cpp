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
  curs_set(0);           // Hide the cursor
  timeout(1);            // Nonblocking getch()
  start_color();
  init_pair(CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);

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
  // todo: this is a hack
  if (cpu.mmu.unmapBootrom) {
    disasm.clear();
    disassemble();
    cursorPos = getDisasmIndex(cpu.reg.pc);
    cpu.mmu.unmapBootrom = false;
  }

  if (runToBreak) {
    if (std::find(breakpoints.begin(), breakpoints.end(), cpu.reg.pc) !=
        breakpoints.end()) {
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
      if (std::find(breakpoints.begin(), breakpoints.end(),
                    disasm[cursorPos].pc) != breakpoints.end()) {
        breakpoints.erase(disasm[cursorPos].pc);
      } else {
        breakpoints.insert(disasm[cursorPos].pc);
      }
      break;
    }

    // Watch memory
    case KEY_F(3):

      break;

    // Reset
    case KEY_F(5):
      cpu.reset();
      gpu.reset();
      disasm.clear();
      disassemble();
      cursorPos = getDisasmIndex(cpu.reg.pc);
      display();
      break;

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
      cursorMove(-1);
      break;

    case KEY_PPAGE:  // pageup
      cursorMove(-(yMax - 2));
      break;

    case KEY_DOWN:
      cursorMove(1);
      break;

    case KEY_NPAGE:  // pagedown
      cursorMove(yMax - 2);
      break;

    case ERR:  // no input
      return;  // don't redraw display
      break;

    default:
      break;
  }
  display();
}

void gb::cursorMove(int distance) {
  cursorPos += distance;
  if (cursorPos < 0) {
    cursorPos = 0;
  }
  if (cursorPos > (int)disasm.size()) {
    cursorPos = disasm.size();
  }
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
  getmaxyx(stdscr, yMax, xMax);

  // Calculate bounds of disassembly display
  int start = cursorPos - (yMax / 2);
  int end = cursorPos + (yMax / 2);

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
    if (std::find(breakpoints.begin(), breakpoints.end(), disasm[i].pc) !=
        breakpoints.end()) {
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
  int x = xMax / 3;
  int y = 0;
  mvprintw(y++, x, "af= %04X", cpu.reg.af);
  mvprintw(y++, x, "bc= %04X", cpu.reg.bc);
  mvprintw(y++, x, "de= %04X", cpu.reg.de);
  mvprintw(y++, x, "hl= %04X", cpu.reg.hl);
  mvprintw(y++, x, "sp= %04X", cpu.reg.sp);
  mvprintw(y++, x, "pc= %04X", cpu.reg.pc);
  mvprintw(y++, x, "ime=%c", cpu.ime ? 1 : '.');
  y = 0;
  x += 11;
  mvprintw(y++, x, "lcdc=%02X", cpu.mmu.memory[LCD_CTL]);
  mvprintw(y++, x, "stat=%02X", cpu.mmu.memory[LCD_STAT]);
  mvprintw(y++, x, "ly=%02X", cpu.mmu.memory[LCD_SCANLINE]);
  mvprintw(y++, x, "ie=%02X", cpu.mmu.memory[CPU_INTERRUPT_ENABLE]);
  mvprintw(y++, x, "if=%02X", cpu.mmu.memory[CPU_INTERRUPT_FLAG]);

  // Print stack
  y += 5;
  x -= 11;
  u8 stackDispSize = 28;  // bytes
  start = cpu.reg.sp;
  if (start < stackDispSize) {
    start = stackDispSize;
  }
  end = start - stackDispSize;
  for (int i = start; i >= end; i -= 2) {
    if (i == cpu.reg.sp) {
      attron(A_STANDOUT);
    } else {
      attroff(A_STANDOUT);
    }
    mvprintw(y++, x, "%04X:%02X%02X", i, cpu.mmu.memory[i + 1],
             cpu.mmu.memory[i]);
  }

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

        case SDL_KEYUP:
          handleSDLKeyup(e.key.keysym.sym);
          break;

        default:
          break;
      }
    }
  }
}

void gb::handleSDLKeydown(SDL_Keycode key) {
  switch (key) {
    case SDLK_d:
      debugEnabled = !debugEnabled;
      break;

    case SDLK_UP:
    case SDLK_DOWN:
    case SDLK_RIGHT:
    case SDLK_LEFT:
    case SDLK_z:
    case SDLK_x:
    case SDLK_RETURN:
    case SDLK_SPACE:
      joypad.keyPressed(key);
      break;

    default:
      break;
  }
}

void gb::handleSDLKeyup(SDL_Keycode key) {
  switch (key) {
    case SDLK_UP:
    case SDLK_DOWN:
    case SDLK_RIGHT:
    case SDLK_LEFT:
    case SDLK_z:
    case SDLK_x:
    case SDLK_RETURN:
    case SDLK_SPACE:
      joypad.keyReleased(key);
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
