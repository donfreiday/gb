// gb: a Gameboy Emulator by Don Freiday
// File: debugger.cpp
// Description: Interactive debugger implemented using ncurses

#include "debugger.h"

#define DUMP_TO_FILE false

Debugger::Debugger() {}

Debugger::~Debugger() {
  if (DUMP_TO_FILE) {
    fout.close();
  }
}

void Debugger::init(CPU* cpuPtr, GPU* gpuPtr) {
  cpu = cpuPtr;
  gpu = gpuPtr;
  runToBreak = false;

    // disassemble(cpu->reg.pc);

  if (DUMP_TO_FILE) {
    fout.open("debug.txt");
  }
}

void Debugger::run() {

}

/*
void Debugger::initCurses() {
  // Init curses
  initscr();
  cbreak();              // Disable TTY buffering, one character at a time
  noecho();              // Suppress echoing of typed characters
  keypad(stdscr, true);  // Capture special keys
  curs_set(0);           // Hide the cursor
  timeout(1);            // Nonblocking getch()
  start_color();
  init_pair(RED, COLOR_RED, COLOR_BLACK);
  init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(WHITE, COLOR_WHITE, COLOR_BLACK);
}

void Debugger::run() {
  if (cpu->mmu.memMapChanged) {
    disasm.clear();
    disassemble(0);
    cursorPos = getDisasmIndex(cpu->reg.pc);
    cpu->mmu.memMapChanged = false;
    display();
  }

  if (runToBreak) {
    if (std::find(breakpoints.begin(), breakpoints.end(), cpu->reg.pc) !=
        breakpoints.end()) {
      runToBreak = false;

      // Hack: disassemble from breakpoint
      while (disasm.back().pc > cpu->reg.pc) {
        disasm.pop_back();
      }
      disassemble(cpu->reg.pc);

      // Snap cursor to last executed instruction
      cursorPos = getDisasmIndex(cpu->reg.pc);

      display();
      return;
    }
    step();
    return;
  }

  switch (getch()) {
    // Set breakpoint at cursor
    case KEY_F(2): {
      if (std::find(breakpoints.begin(), breakpoints.end(),
                    disasm[cursorPos].pc) != breakpoints.end()) {
        breakpoints.erase(disasm[cursorPos].pc);
      } else {
        breakpoints.insert(disasm[cursorPos].pc);
      }

    } break;

    // Reset
    case KEY_F(5):
      cpu->reset();
      gpu->reset();
      cpu->mmu.load(cpu->mmu.romFile);
      disasm.clear();
      disassemble(0);
      cursorPos = getDisasmIndex(cpu->reg.pc);
      display();
      break;

    // Step
    case KEY_F(7):
      step();
      cursorPos = getDisasmIndex(
          cpu->reg.pc);  // Snap cursor to last executed instruction
      break;

    // Run to break
    case KEY_F(9):
      step();  // get past breakpoint at current PC
      runToBreak = true;
      return;  // skip display()
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

void Debugger::cursorMove(int distance) {
  cursorPos += distance;
  if (cursorPos < 0) {
    cursorPos = 0;
  }
  if (cursorPos > (int)disasm.size()) {
    cursorPos = disasm.size();
  }
}

// Find current PC in disassembly
// todo: rethink this, use a better search algorithm / data structure
int Debugger::getDisasmIndex(u16 pc) {
  std::vector<disassembly>::iterator it;
  it = find(disasm.begin(), disasm.end(), pc);

  // Hack hack hack: if we don't have disassembly of the instruction at current
  // PC, try disassembling again from current PC
  if (it == disasm.end()) {
    while (disasm.back().pc > pc) {
      disasm.pop_back();
    }
    disassemble(pc);
    it = find(disasm.begin(), disasm.end(), pc);
  }

  return std::distance(disasm.begin(), it);
}

// Use curses to print disassembly, registers, etc
// todo: overhaul, partial screen draws, etc
void Debugger::display() {
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

    // Breakpoints bullets
    if (std::find(breakpoints.begin(), breakpoints.end(), disasm[i].pc) !=
        breakpoints.end()) {
      attron(COLOR_PAIR(RED) | A_BOLD);
      printw("*");
      attroff(COLOR_PAIR(RED) | A_BOLD);
    } else {
      printw(" ");
    }

    // Cursor position is highlighted
    if (cursorPos == i) {
      attron(A_STANDOUT);
    }

    // Current PC is bold
    if (disasm[i].pc == cpu->reg.pc) {
      attron(A_BOLD);
    }

    printw("%04X: %02X ", disasm[i].pc, cpu->mmu.memory[disasm[i].pc]);
    if (disasm[i].operandSize == 1) {
      printw("%02X     ", cpu->mmu.memory[disasm[i].pc + 1]);
      printw(disasm[i].str.c_str(), disasm[i].operand);
    } else if (disasm[i].operandSize == 2) {
      printw("%02X %02X  ", cpu->mmu.memory[disasm[i].pc + 1],
             cpu->mmu.memory[disasm[i].pc + 2]);
      printw(disasm[i].str.c_str(), disasm[i].operand);
    } else {
      printw("       ");
      printw(disasm[i].str.c_str());
    }
    printw("\n");
  }

  // Registers
  attron(COLOR_PAIR(WHITE));
  int x = xMax / 3;
  int y = 0;
  attron(A_BOLD);
  mvprintw(y++, x, "Registers");
  attroff(A_BOLD);
  mvprintw(y++, x, "af= %04X", cpu->reg.af);
  mvprintw(y++, x, "bc= %04X", cpu->reg.bc);
  mvprintw(y++, x, "de= %04X", cpu->reg.de);
  mvprintw(y++, x, "hl= %04X", cpu->reg.hl);
  mvprintw(y++, x, "sp= %04X", cpu->reg.sp);
  mvprintw(y++, x, "pc= %04X", cpu->reg.pc);
  mvprintw(y++, x, "ime=%d", (cpu->ime ? 1 : 0));
  y = 1;
  x += 11;
  mvprintw(y++, x, "lcdc=%02X", cpu->mmu.memory[LCDC]);
  mvprintw(y++, x, "stat=%02X", cpu->mmu.memory[STAT]);
  mvprintw(y++, x, "ly=%02X", cpu->mmu.memory[LY]);
  mvprintw(y++, x, "ie=%02X", cpu->mmu.memory[IE]);
  mvprintw(y++, x, "if=%02X", cpu->mmu.memory[IF]);

  // Print stack
  y += 3;
  x -= 11;
  attron(A_BOLD);
  mvprintw(y++, x, "Stack");
  attroff(A_BOLD);
  u8 stackDispSize = 28;  // bytes
  start = cpu->reg.sp + (stackDispSize / 2);
  if (start > 0xFFFF) {
    start = 0xFFFF;
  }
  end = start - stackDispSize;
  for (int i = start; i >= end; i -= 2) {
    if (i == cpu->reg.sp) {
      attron(A_STANDOUT);
    } else {
      attroff(A_STANDOUT);
    }
    mvprintw(y++, x, "%04X:%02X%02X", i, cpu->mmu.memory[i + 1],
             cpu->mmu.memory[i]);
  }

  // Print IO map
  attroff(A_STANDOUT);
  y = 0;
  x += 24;
  attron(A_BOLD);
  mvprintw(y++, x, "LCD");
  attroff(A_BOLD);
  mvprintw(y++, x, "%02X FF40 LCDC", cpu->mmu.memory[LCDC]);
  mvprintw(y++, x, "%02X FF41 STAT", cpu->mmu.memory[STAT]);
  mvprintw(y++, x, "%02X FF42 SCY", cpu->mmu.memory[SCY]);
  mvprintw(y++, x, "%02X FF43 SCX", cpu->mmu.memory[SCX]);
  mvprintw(y++, x, "%02X FF44 LY", cpu->mmu.memory[LY]);
  mvprintw(y++, x, "%02X FF45 LYC", cpu->mmu.memory[LYC]);
  mvprintw(y++, x, "%02X FF46 DMA", cpu->mmu.memory[DMA]);
  mvprintw(y++, x, "%02X FF47 BGP", cpu->mmu.memory[BGP]);
  mvprintw(y++, x, "%02X FF48 OBP0", cpu->mmu.memory[OBP0]);
  mvprintw(y++, x, "%02X FF49 OBP1", cpu->mmu.memory[OBP1]);
  mvprintw(y++, x, "%02X FF4A WY", cpu->mmu.memory[WY]);
  mvprintw(y++, x, "%02X FF4B WX", cpu->mmu.memory[WX]);

  // LCDC
  y += 2;
  x -= 2;
  attron(A_BOLD);
  mvprintw(y++, x, "  LCDC (FF40)");
  attroff(A_BOLD);
  bool set;
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_DISPLAY_ENABLE);
  mvprintw(y++, x, "%c LCD %s", set ? '*' : ' ', set ? "on" : "off");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_WINDOW_TILE_MAP_SELECT);
  mvprintw(y++, x, "%c WIN %s", set ? '*' : ' ',
           set ? "9c00-9fff" : "9800-9bff");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_WINDOW_ENABLE);
  mvprintw(y++, x, "%c WIN %s", set ? '*' : ' ', set ? "on" : "off");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_TILE_DATA_SELECT);
  mvprintw(y++, x, "%c CHR %s", set ? '*' : ' ',
           set ? "8000-8FFF" : "8800-97FF");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_BG_TILE_MAP_SELECT);
  mvprintw(y++, x, "%c BG  %s", set ? '*' : ' ',
           set ? "9c00-9fff" : "9800-9bff");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_OBJ_SIZE);
  mvprintw(y++, x, "%c OBJ %s", set ? '*' : ' ', set ? "8x16" : "8x8");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_OBJ_ENABLE);
  mvprintw(y++, x, "%c OBJ %s", set ? '*' : ' ', set ? "on" : "off");
  set = bitTest(cpu->mmu.memory[LCDC], LCDC_BG_ENABLE);
  mvprintw(y++, x, "%c BG  %s", set ? '*' : ' ', set ? "on" : "off");

  // More IO map
  y = 0;
  x += 22;
  attron(A_BOLD);
  mvprintw(y++, x, "various");
  attroff(A_BOLD);
  mvprintw(y++, x, "%02X FF70 SVBK", cpu->mmu.memory[SVBK]);  // CGB only
  mvprintw(y++, x, "%02X FF4F VBK", cpu->mmu.memory[VBK]);    // CGB only
  mvprintw(y++, x, "%02X FF4D KEY1", cpu->mmu.memory[KEY1]);  // CGB only
  mvprintw(y++, x, "%02X FF00 JOYP", cpu->mmu.memory[JOYP]);
  mvprintw(y++, x, "%02X FF01 SB", cpu->mmu.memory[SB]);
  mvprintw(y++, x, "%02X FF02 SC", cpu->mmu.memory[SC]);
  mvprintw(y++, x, "%02X FF04 DIV", cpu->mmu.memory[DIV]);
  mvprintw(y++, x, "%02X FF05 TIMA", cpu->mmu.memory[TIMA]);
  mvprintw(y++, x, "%02X FF06 TMA", cpu->mmu.memory[TMA]);
  mvprintw(y++, x, "%02X FF07 TAC", cpu->mmu.memory[TAC]);
  mvprintw(y++, x, "%02X FF0F IF", cpu->mmu.memory[IF]);
  mvprintw(y++, x, "%02X FFFF IE", cpu->mmu.memory[IE]);

  // STAT
  y += 2;
  x -= 2;
  attron(A_BOLD);
  mvprintw(y++, x, "  STAT (FF41)");
  attroff(A_BOLD);
  set = bitTest(cpu->mmu.memory[STAT], STAT_LYC_INT_ENABLE);
  mvprintw(y++, x, "%c b6 LY=LYC int", set ? '*' : ' ');
  set = bitTest(cpu->mmu.memory[STAT], STAT_MODE2_INT_ENABLE);
  mvprintw(y++, x, "%c b5 OAM (mode2) int", set ? '*' : ' ');
  set = bitTest(cpu->mmu.memory[STAT], STAT_MODE1_INT_ENABLE);
  mvprintw(y++, x, "%c b4 VBlank (mode1) int", set ? '*' : ' ');
  set = bitTest(cpu->mmu.memory[STAT], STAT_MODE0_INT_ENABLE);
  mvprintw(y++, x, "%c b3 HBlank (mode0) int", set ? '*' : ' ');
  set = bitTest(cpu->mmu.memory[STAT], STAT_LYC_FLAG);
  mvprintw(y++, x, "%c b2 LY=LYC", set ? '*' : ' ');
  mvprintw(y++, x, "  Mode %d", (cpu->mmu.memory[STAT] & 3));  // low two bits

  y += 2;
  x += 2;
  attron(A_BOLD);
  mvprintw(y++, x, "GPU");
  attroff(A_BOLD);
  mvprintw(y++, x, "Mode %d", gpu->mode);
  mvprintw(y++, x, "Modeclock %d", gpu->modeclock);
  mvprintw(y++, x, "Scanline %02X", gpu->scanline);

  refresh();
}

void Debugger::disassemble(u16 initPC) {
  u16 operand, pc = initPC;
  u8 op;
  while (pc < 0xFFFF) {  // hack: disassemble entire memory map
    disassembly d;
    d.pc = pc;
    op = cpu->mmu.memory[pc];
    if (op == 0xCB) {
      d.operandSize = 0;
      operand = cpu->mmu.memory[++pc];
      d.str = cpu->instructions_CB[operand].disassembly;
    } else if (cpu->instructions[op].operandLength == 1) {
      d.operandSize = 1;
      d.operand = cpu->mmu.memory[++pc];
      d.str = cpu->instructions[op].disassembly;
    } else if (cpu->instructions[op].operandLength == 2) {
      d.operandSize = 2;
      d.operand = cpu->mmu.read_u16(++pc);
      d.str = cpu->instructions[op].disassembly;
      pc++;
    } else {
      d.operandSize = 0;
      d.str = cpu->instructions[op].disassembly;
    }
    pc++;
    disasm.push_back(d);
  }
}

void Debugger::step() {
  cpu->checkInterrupts();
  cpu->execute();
  gpu->step(cpu->cpu_clock_t);
  if (DUMP_TO_FILE) {
    fout << setfill('0') << setw(4) << hex << cpu->reg.pc+1 << ": "
         << setfill('0') << setw(4) << " af=" << cpu->reg.af << setfill('0')
         << setw(4) << " bc=" << cpu->reg.bc << setfill('0') << setw(4)
         << " de=" << cpu->reg.de << setfill('0') << setw(4)
         << " hl=" << cpu->reg.hl << setfill('0') << setw(4)
         << " sp=" << cpu->reg.sp << endl;
  }
}*/