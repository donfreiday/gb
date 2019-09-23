// gb: a Gameboy Emulator by Don Freiday
// File: main.cpp
// Description: Main loop and gui

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <set>
#include "common.hpp"
#include "cpu.hpp"
#include "gpu.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "joypad.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __clang__
#pragma clang diagnostic ignored \
    "-Wint-to-void-pointer-cast"  // warning : cast to 'void *' from smaller
                                  // integer type 'int'
#endif

void main_loop();
// These are global because emscripten's main_loop() can't have parameters
CPU g_cpu;
GPU g_gpu;
Joypad g_joypad;

// Window rendering functions
void imguiLCD();
void imguiRegisters();
void imguiLcdStatus();
void imguiDisassembly();
void imguiHelp();

// Event handling
void handleSdlEvents();
void handleKeyPress(SDL_Keycode key);

// Disassembler
struct disassembly {
  std::string str;
  u16 address;
  u8 opcode;
  u16 operand;
  u8 operandSize;
  std::set<u16> knownEntryPoints;
} g_disasm;
void disassemble(CPU& cpu, u16& pc);

// Breakpoints
std::set<u16> g_breakpoints;

// State variables
bool g_quit = false;
bool g_running = true;
bool g_stepping = false;
bool g_scrollDisasmToPC = true;
bool g_fullscreenLcd = false;

SDL_Window* g_window;

// LCD will be rendered to this texture
GLuint g_lcdTexture;

// Main event loop
void main_loop() {
  // Setup SDL and start new ImGui frame
  ImGui_ImplSdl_NewFrame(g_window);

  handleSdlEvents();

  // Run the emulator until vsync or breakpoint
  while ((g_running || g_stepping) && !g_gpu.vsync) {
    g_disasm.knownEntryPoints.insert(g_cpu.reg.pc);

    if (g_stepping) {
      g_stepping = false;
      g_scrollDisasmToPC = true;
    }

    g_cpu.checkInterrupts();
    g_cpu.execute();
    g_gpu.step(g_cpu.cpu_clock_t);

    // Check for breakpoint
    if (g_breakpoints.find(g_cpu.reg.pc) != g_breakpoints.end()) {
      g_running = false;
      g_scrollDisasmToPC = true;
      g_fullscreenLcd = false;
    }
  }
  g_gpu.vsync = false;

  // Render GUI windows
  imguiLCD();
  if (!g_fullscreenLcd) {
    imguiRegisters();
    imguiLcdStatus();
    imguiDisassembly();
    imguiHelp();
  }

  // ImGui::ShowTestWindow();

  // Append FPS to window title
  std::stringstream windowTitle;
  windowTitle << "gb: A Gameboy Emulator    FPS: " << std::fixed
              << std::setprecision(0) << ImGui::GetIO().Framerate;

  // Rendering
  glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x,
             (int)ImGui::GetIO().DisplaySize.y);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui::Render();
  SDL_SetWindowTitle(g_window, windowTitle.str().c_str());
  SDL_GL_SwapWindow(g_window);
}

// Load ROM, set up SDL/ImGui, main loop till quit, cleanup ImGui/SDL
int main(int argc, char** argv) {
  // Initialize emulator components
  g_gpu.mmu = &g_cpu.mmu;
  g_cpu.mmu.joypad = &g_joypad;
  g_gpu.reset();

  // Load ROM
#ifdef __EMSCRIPTEN__
  if (!g_cpu.mmu.load("roms/tetris.gb")) {
    printf("Invalid ROM file: roms/tetris.gb\n");
    return -1;
  }
#else
  if (argc < 2) {
    printf("Please specify a ROM file.\n");
    return -1;
  }
  if (!g_cpu.mmu.load(argv[1])) {
    printf("Invalid ROM file: %s\n", argv[1]);
    return -1;
  }
#endif

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL Error: %s\n", SDL_GetError());
    return -1;
  }

  // Setup MAX_CONTROLLERS game controllers
  // MAX_CONTROLLERS is (defined in common.hpp)
  SDL_GameController* controllers[MAX_CONTROLLERS];

  // Initialize array of controllers with nullptrs
  for (int i = 0; i < MAX_CONTROLLERS; i++) {
    controllers[i] = nullptr;
  }

  // Open game controllers
  if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0) {
    printf("SDL Error initializing gamepad support: %s\n", SDL_GetError());
  } else {
    int maxJoysticks = SDL_NumJoysticks();
    int controllerID = 0;
    for (int i = 0; i < maxJoysticks && controllerID < MAX_CONTROLLERS; ++i) {
      if (SDL_IsGameController(i)) {
        controllers[i] = SDL_GameControllerOpen(i);
        controllerID++;
      }
    }
  }

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_DisplayMode current;
  SDL_GetCurrentDisplayMode(0, &current);
  g_window = SDL_CreateWindow("gb: A Gameboy Emulator", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1024, 768,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext glcontext = SDL_GL_CreateContext(g_window);

  // Setup ImGui binding
  ImGui_ImplSdl_Init(g_window);

  // Generate a new texture and store the handle
  glGenTextures(1, &g_lcdTexture);

  // These settings stick with the texture that's bound. Only need to set them
  // once.
  glBindTexture(GL_TEXTURE_2D, g_lcdTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // allocate memory on the graphics card for the texture. It's fine if
  // texture_data doesn't have any data in it, the texture will just appear
  // black until you update it.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_gpu.width, g_gpu.height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, g_gpu.screenData);

  // Main loop
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!g_quit) {
    main_loop();
  }
#endif

  // Cleanup
  // When you're done using the texture, delete it. This will set texname to 0 &
  // delete all of the graphics card memory associated with the texture. If you
  // don't call this method, the texture will stay in graphics card memory until
  // you close the application.
  glDeleteTextures(1, &g_lcdTexture);

  // Close game controllers
  for (int i = 0; i < MAX_CONTROLLERS; ++i) {
    if (controllers[i]) {
      SDL_GameControllerClose(controllers[i]);
    }
  }

  ImGui_ImplSdl_Shutdown();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(g_window);
  SDL_Quit();

  return 0;
}

// Display LCD in a window
void imguiLCD() {
  // Update texture
  // bind the texture again when you want to update it.
  glBindTexture(GL_TEXTURE_2D, g_lcdTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, g_gpu.width, g_gpu.height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, g_gpu.screenData);

  // Set up window flags
  ImGuiWindowFlags windowFlags = 0;
  windowFlags |= ImGuiWindowFlags_NoTitleBar;
  windowFlags |= ImGuiWindowFlags_NoScrollbar;
  windowFlags |= ImGuiWindowFlags_NoCollapse;
  windowFlags |= ImGuiWindowFlags_NoResize;
  windowFlags |= ImGuiWindowFlags_NoMove;

  // Get window dimensions
  float winWidth = ImGui::GetIO().DisplaySize.x;
  float winHeight = ImGui::GetIO().DisplaySize.y;

  // Fullscreen
  if (g_fullscreenLcd) {
    // Scale while maintaining approximate aspect ratio
    // Todo: calculate proper scale factor
    float width = g_gpu.width;
    float height = g_gpu.height;
    while (width * 1.1f < winWidth && height * 1.1f < winHeight) {
      width *= 1.1;
      height *= 1.1;
    }
    ImGui::SetNextWindowSize(ImVec2(width, height));

    // Position centered
    ImGui::SetNextWindowPosCenter();
  }

  // Windowed
  else {
    // Set the window size to Gameboy LCD dimensions
    ImGui::SetNextWindowSize(ImVec2(g_gpu.width * 2, g_gpu.height * 2));
    // Position top right
    ImGui::SetNextWindowPos(ImVec2(winWidth - g_gpu.width * 2, 0.0f));
  }

  // No padding
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  // Render window
  ImGui::Begin("lcd", nullptr, windowFlags);
  ImGui::Image((void*)g_lcdTexture, ImGui::GetWindowSize());

  // Done, clean up
  ImGui::End();
  ImGui::PopStyleVar();
}

// Display registers in a window
void imguiRegisters() {
  // Position to the right of disassembly window
  ImGui::SetNextWindowPos(ImVec2(DISASM_WINDOW_WIDTH, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(REG_WINDOW_WIDTH, REG_WINDOW_HEIGHT));
  ImGui::Begin("reg", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  ImGui::Text(
      "af = %04X\nbc = %04X\nde = %04X\nhl = %04X\nsp = %04X\npc = %04X",
      g_cpu.reg.af, g_cpu.reg.bc, g_cpu.reg.de, g_cpu.reg.hl, g_cpu.reg.sp,
      g_cpu.reg.pc);
  ImGui::End();
}

// Display LCD status in a window
void imguiLcdStatus() {
  // Position to the right of disassembly window, below register window
  ImGui::SetNextWindowPos(ImVec2(DISASM_WINDOW_WIDTH, REG_WINDOW_HEIGHT));
  ImGui::SetNextWindowSize(ImVec2(REG_WINDOW_WIDTH, LCD_STATUS_WINDOW_HEIGHT));
  ImGui::Begin("lcd stat", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  ImGui::Text(
      "%02X FF40 LCDC\n"
      "%02X FF41 STAT\n"
      "%02X FF42 SCY\n"
      "%02X FF43 SCX\n"
      "%02X FF44 LY\n"
      "%02X FF45 LYC\n"
      "%02X FF46 DMA\n"
      "%02X FF47 BGP\n"
      "%02X FF48 OBP0\n"
      "%02X FF49 OBP1\n"
      "%02X FF4A WY\n"
      "%02X FF4B WX\n",
      g_cpu.mmu.memory[0xFF40], g_cpu.mmu.memory[0xFF41],
      g_cpu.mmu.memory[0xFF42], g_cpu.mmu.memory[0xFF43],
      g_cpu.mmu.memory[0xFF44], g_cpu.mmu.memory[0xFF45],
      g_cpu.mmu.memory[0xFF46], g_cpu.mmu.memory[0xFF47],
      g_cpu.mmu.memory[0xFF48], g_cpu.mmu.memory[0xFF49],
      g_cpu.mmu.memory[0xFF4A], g_cpu.mmu.memory[0xFF4B]);
  ImGui::End();
}

// Display disassembly in a window
void imguiDisassembly() {
  // Get window dimensions
  ImGui::SetNextWindowSize(
      ImVec2(DISASM_WINDOW_WIDTH, ImGui::GetIO().DisplaySize.y));

  // Position in top left corner
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

  ImGui::Begin("disassembly", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

  // Button to run or pause emulator
  if (ImGui::Button(g_running ? "Pause(r)" : " Run(r) ")) {
    g_scrollDisasmToPC = g_running;
    g_running = !g_running;
  }

  // Button for step. Only works when emulation is paused
  ImGui::SameLine();
  if (ImGui::Button("Step(s)")) {
    if (!g_running) {
      g_stepping = true;
    }
  }

  // Button for jump to PC
  ImGui::SameLine();
  if (ImGui::Button("Goto PC(p)")) {
    g_scrollDisasmToPC = true;
  }

  ImGui::BeginChild("disasm", ImVec2(0.0f, 0.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

  ImGuiListClipper clipper(0x7FFF, ImGui::GetTextLineHeight());
  u16 index = clipper.DisplayStart * 2;

  while (index < clipper.DisplayEnd * 2) {
    // Is the current index a breakpoint?
    bool breakpoint = g_breakpoints.find(index) != g_breakpoints.end();

    // Clicking a line sets or removes a breakpoint
    ImGui::PushID(index);
    if (ImGui::Selectable("##breakpoint",
                          ImGuiSelectableFlags_SpanAllColumns)) {
      if (breakpoint) {
        g_breakpoints.erase(index);
      } else {
        g_breakpoints.insert(index);
      }
    }
    ImGui::PopID();

    // Color currently executing line
    ImVec4 color;
    if (index == g_cpu.reg.pc) {
      color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
    } else {
      color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Mark breakpoints
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::SameLine();
    if (breakpoint) {
      ImGui::Text("*");
    } else {
      ImGui::Text(" ");
    }
    ImGui::PopStyleVar();

    // This will increment index by size of opcode+operand
    int prevIndex = index;
    disassemble(g_cpu, index);

    // Don't skip the current PC
    if (prevIndex < g_cpu.reg.pc && index > g_cpu.reg.pc) {
      index = g_cpu.reg.pc;
      g_disasm.knownEntryPoints.insert(index);
      color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
      disassemble(g_cpu, index);
    }

    // Display current address, opcode, and operand (if any)
    ImGui::SameLine();
    if (g_disasm.operandSize > 0) {
      // One byte operand
      if (g_disasm.operandSize == 1) {
        ImGui::TextColored(color, "%04X:%02X %02X    ", g_disasm.address,
                           g_disasm.opcode, (u8)g_disasm.operand);
      }
      // Two byte operand
      else {
        ImGui::TextColored(color, "%04X:%02X %04X  ", g_disasm.address,
                           g_disasm.opcode, g_disasm.operand);
      }
    }
    // No operand
    else {
      ImGui::TextColored(color, "%04X:%02X       ", g_disasm.address,
                         g_disasm.opcode);
    }

    // Display disassembly
    ImGui::SameLine();
    ImGui::TextColored(color, g_disasm.str.c_str(), g_disasm.operand);
  }

  // Scroll to current PC if warranted
  if (!g_running && g_scrollDisasmToPC) {
    float target = (g_cpu.reg.pc - 20) * ImGui::GetTextLineHeight() * 0.5f;
    if (fabsf(target - ImGui::GetScrollY()) > 20) {
      ImGui::SetScrollY(target);
    }
    g_scrollDisasmToPC = false;
  }

  clipper.End();
  ImGui::PopStyleVar();
  ImGui::EndChild();
  ImGui::End();
}

void imguiHelp() {
  ImGui::SetNextWindowPos(ImVec2(DISASM_WINDOW_WIDTH+REG_WINDOW_WIDTH, 0.0f));
  ImGui::Begin("controls", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  ImGui::Text(
      "A: z\n"
      "B: x\n"
      "Select: space\n"
      "Start: enter\n"
      "Directions: arrows\n"
      "Fullscreen: f\n");
  ImGui::End();
}

void handleSdlEvents() {
  // Handle keydown, window close, etc
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSdl_ProcessEvent(&event);
    switch (event.type) {
      case SDL_KEYDOWN:
        handleKeyPress(event.key.keysym.sym);
      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_KEYUP:
      case SDL_CONTROLLERBUTTONUP:
        g_joypad.handleEvent(event);
        break;

      case SDL_CONTROLLERDEVICEADDED:
        // todo
        break;

      case SDL_CONTROLLERDEVICEREMOVED:
        // todo
        break;

      case SDL_QUIT:
        g_quit = true;
        break;

      default:
        break;
    }
  }
}

// Handle keyboard events (fullscreen, step, etc)
void handleKeyPress(SDL_Keycode key) {
  switch (key) {
    // Fullscreen
    case SDLK_f:
      g_fullscreenLcd = !g_fullscreenLcd;
      break;

    // Step
    case SDLK_s:
      if (!g_running) {
        g_stepping = true;
      }
      break;

    // Toggle running state
    case SDLK_r:
      g_scrollDisasmToPC = g_running;
      g_running = !g_running;
      break;

    // Scroll to PC
    case SDLK_p:
      g_scrollDisasmToPC = true;
      break;

    default:
      break;
  }
}

// This is a ugly, ugly hack
void disassemble(CPU& cpu, u16& pc) {
  g_disasm.address = pc;
  g_disasm.opcode = cpu.mmu.memory[pc];
  g_disasm.operandSize = 0;
  g_disasm.operand = 0;

  if (g_disasm.opcode == 0xCB) {
    g_disasm.operandSize = 1;
    g_disasm.operand = cpu.mmu.memory[++pc];
    g_disasm.str = cpu.instructions_CB[g_disasm.operand].disassembly;
  } else if (cpu.instructions[g_disasm.opcode].operandLength == 1) {
    g_disasm.operandSize = 1;
    g_disasm.operand = cpu.mmu.memory[++pc];
    g_disasm.str = cpu.instructions[g_disasm.opcode].disassembly;
  } else if (cpu.instructions[g_disasm.opcode].operandLength == 2) {
    g_disasm.operandSize = 2;
    g_disasm.operand = cpu.mmu.read16(++pc);
    g_disasm.str = cpu.instructions[g_disasm.opcode].disassembly;
    pc++;
  } else {
    ++pc;
    g_disasm.operandSize = 0;
    g_disasm.str = cpu.instructions[g_disasm.opcode].disassembly;
  }

  // Make sure we don't skip over any code entry points
  bool skippedEntry = false;
  for (u16 address : g_disasm.knownEntryPoints) {
    if (pc < address && pc + g_disasm.operandSize > address) {
      skippedEntry = true;
      pc = address;
    }
  }

  // Data sections are mixed in with code. So we need to make sure
  // that we don't skip right over the PC by blindly disassembling
  if (pc < g_cpu.reg.pc && pc + g_disasm.operandSize > g_cpu.reg.pc) {
    pc = g_cpu.reg.pc;
    g_disasm.knownEntryPoints.insert(pc);
    skippedEntry = true;
  }

  if (!skippedEntry) {
    pc += g_disasm.operandSize;
  }
}