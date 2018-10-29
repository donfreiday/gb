// gb: a Gameboy Emulator by Don Freiday
// File: main.cpp
// Description: Main loop and gui

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <set>
#include "disassembler.h"
#include "gb.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __clang__
#pragma clang diagnostic ignored \
    "-Wint-to-void-pointer-cast"  // warning : cast to 'void *' from smaller
                                  // integer type 'int'
#endif

bool loadROM();

void handleSDLEvents();

// Window rendering functions
void imguiLCD();
void imguiRegisters();
void imguiDisassembly();

// Emulator core
gb g_core;

// Disassembler
Disassembler* g_disassembler;

// Breakpoints
std::set<u16> g_breakpoints;

SDL_Window* g_window;

// Imgui window flags and stats
bool g_showLcdWindow = true;
ImVec2 g_LcdWindowSize;
bool g_showRegWindow = true;
bool g_showDisassemblyWindow = true;

ImVec4 g_clearColor = ImColor(0, 0, 0);

// State variables for GUI
bool g_quit = false;
bool g_running = true;
bool g_scrollDisasmToPC = false;

void main_loop() {
  // Handle keydown, window close, etc
  handleSDLEvents();

  // Setup SDL and start new ImGui frame
  ImGui_ImplSdl_NewFrame(g_window);

  // Run the emulator until vsync
  while (g_running && !g_core.gpu.vsync) {
    g_core.step();

    // Did we hit a breakpoint?
    if (g_breakpoints.find(g_core.cpu.reg.pc) != g_breakpoints.end()) {
      g_running = false;
      g_scrollDisasmToPC = true;
    }

    // Make sure current address has been disassembled
    /*Disassembler::Line line{g_core.cpu.reg.pc};
    if (!g_disassembler->disassembly.count(line)) {
      g_disassembler->disassembleFrom(g_core.cpu.reg.pc);
    }*/
  }
  g_core.gpu.vsync = false;

  // Render GUI windows
  imguiLCD();
  imguiRegisters();
  imguiDisassembly();

  // ImGui::ShowTestWindow();

  // Rendering
  glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x,
             (int)ImGui::GetIO().DisplaySize.y);
  glClearColor(g_clearColor.x, g_clearColor.y, g_clearColor.z, g_clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui::Render();
  SDL_GL_SwapWindow(g_window);
}

// Load ROM, set up SDL/ImGui, main loop till quit, cleanup ImGui/SDL
int main(int argc, char** argv) {
  // Load ROM
#ifdef __EMSCRIPTEN__
  if (!g_core.loadROM("roms/tetris.gb")) {
    printf("Invalid ROM file: roms/tetris.gb\n");
    return -1;
  }
#else
  if (argc < 2) {
    printf("Please specify a ROM file.\n");
    return -1;
  }
  if (!g_core.loadROM(argv[1])) {
    printf("Invalid ROM file: %s\n", argv[1]);
    return -1;
  }
#endif

  // Initialize disassembler
  g_disassembler = new Disassembler(&g_core.cpu);

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("Error: %s\n", SDL_GetError());
    return -1;
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
                              SDL_WINDOWPOS_CENTERED, current.w, current.h,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext glcontext = SDL_GL_CreateContext(g_window);

  // Setup ImGui binding
  ImGui_ImplSdl_Init(g_window);

  // Main loop
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!g_quit) {
    main_loop();
  }
#endif

  // Cleanup
  delete g_disassembler;
  g_disassembler = nullptr;
  ImGui_ImplSdl_Shutdown();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(g_window);
  SDL_Quit();

  return 0;
}

// Handle SDL events. Keydown and keyup events are passed to the core
void handleSDLEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSdl_ProcessEvent(&event);
    switch (event.type) {
      case SDL_KEYDOWN:
        g_core.handleSDLKeydown(event.key.keysym.sym);
        break;

      case SDL_KEYUP:
        g_core.handleSDLKeyup(event.key.keysym.sym);
        break;

      case SDL_QUIT:
        g_quit = true;
        break;

      default:
        break;
    }
  }
}

// Display LCD in a window
void imguiLCD() {
  // Set up window flags
  ImGuiWindowFlags windowFlags = 0;
  windowFlags |= ImGuiWindowFlags_NoTitleBar;
  windowFlags |= ImGuiWindowFlags_NoScrollbar;
  windowFlags |= ImGuiWindowFlags_NoCollapse;

  // Set the window size to Gameboy LCD dimensions
  ImGui::SetNextWindowSize(ImVec2(g_core.gpu.width, g_core.gpu.height),
                           ImGuiSetCond_Once);

  // Window defaults to upper left corner (for now)
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);

  // No padding
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  // Render window
  ImGui::Begin("lcd", &g_showLcdWindow, windowFlags);
  g_LcdWindowSize = ImGui::GetWindowSize();
  ImGui::Image((void*)g_core.gpu.texture, g_LcdWindowSize);

  // Done, clean up
  ImGui::End();
  ImGui::PopStyleVar();
}

// Display registers in a window
void imguiRegisters() {
  // Set the window size to Gameboy LCD dimensions
  ImGui::SetNextWindowSize(ImVec2(g_core.gpu.width, g_core.gpu.height),
                           ImGuiSetCond_Once);

  // Position window below LCD
  ImGui::SetNextWindowPos(ImVec2(0, g_LcdWindowSize.y), ImGuiSetCond_Once);

  ImGui::Begin("reg", &g_showRegWindow);

  ImGui::Text(
      "af = %04X\nbc = %04X\nde = %04X\nhl = %04X\nsp = %04X\npc = %04X",
      g_core.cpu.reg.af, g_core.cpu.reg.bc, g_core.cpu.reg.de,
      g_core.cpu.reg.hl, g_core.cpu.reg.sp, g_core.cpu.reg.pc);
  ImGui::End();
}

// Display disassembly in a window
void imguiDisassembly() {
  ImGui::SetNextWindowSize(ImVec2(400.0f, 500.0f), ImGuiSetCond_Once);
  ImGui::Begin("disassembly", &g_showDisassemblyWindow);

  // Button to run or pause emulator
  if (ImGui::Button(g_running ? "Pause" : " Run ")) {
    g_scrollDisasmToPC = g_running;
    g_running = !g_running;
  }

  // Button for step. Only works when emulation is paused
  ImGui::SameLine();
  if (ImGui::Button("Step")) {
    if (!g_running) {
      g_core.step();
    }
  }

  ImGui::BeginChild("disasm", ImVec2(0.0f, 0.0f));
  ImGuiListClipper clipper(0x7FFF, ImGui::GetTextLineHeight());
  u16 index = clipper.DisplayStart * 2;

  while (index < clipper.DisplayEnd * 2) {
    // Is the current index a breakpoint?
    bool breakpoint = g_breakpoints.find(index) != g_breakpoints.end();
    
    // Clicking a line sets or removes a breakpoint
    ImGui::PushID(index);
    if (ImGui::Selectable("##breakpoint", ImGuiSelectableFlags_SpanAllColumns)) {
      if (breakpoint) {
        g_breakpoints.erase(index);
      } else {
        g_breakpoints.insert(index);
      }
    }
    ImGui::PopID();

    // Color currently executing line
    ImVec4 color;
    if (index == g_core.cpu.reg.pc) {
      color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
    } else {
      color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Mark breakpoints
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::SameLine();
    if(breakpoint) {
      ImGui::Text("*");
    } else {
      ImGui::Text(" ");
    }
    ImGui::PopStyleVar();
    
    // Display current address and opcode
    ImGui::SameLine();
    ImGui::TextColored(color, "%04X:%02X", index, g_core.cpu.mmu.memory[index]);

    // Display disassembly 
    ImGui::SameLine();
    disassembly d = g_disassembler->disassemble(index);
    ImGui::TextColored(color, d.str.c_str(), d.operand);
  }

  // Scroll to current PC if warranted
  if (!g_running && g_scrollDisasmToPC) {
    ImGui::SetScrollY(g_core.cpu.reg.pc * ImGui::GetTextLineHeight() * 0.5f);
    g_scrollDisasmToPC = false;
  }

  clipper.End();
  ImGui::EndChild();
  ImGui::End();
}