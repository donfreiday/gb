// gb: a Gameboy Emulator by Don Freiday
// File: main.cpp
// Description: Main loop and gui

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <set>
#include "common.h"
#include "cpu.h"
#include "gpu.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "joypad.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __clang__
#pragma clang diagnostic ignored \
    "-Wint-to-void-pointer-cast"  // warning : cast to 'void *' from smaller
                                  // integer type 'int'
#endif

void main_loop(CPU& cpu, GPU& gpu, Joypad& joypad);

// Window rendering functions
void imguiLCD(GPU& gpu);
void imguiRegisters(CPU& cpu);
void imguiDisassembly(CPU& cpu);

// Disassembler
struct disassembly {
  std::string str;
  u16 operand;
} g_disassembly;
void disassemble(CPU& cpu, u16& pc);

// Breakpoints
std::set<u16> g_breakpoints;

// State variables
bool g_quit = false;
bool g_running = false;
bool g_stepping = false;
bool g_scrollDisasmToPC = false;

SDL_Window* g_window;

// LCD will be rendered to this texture
GLuint g_lcdTexture;

// Imgui window flags and stats
bool g_showLcdWindow = true;
ImVec2 g_LcdWindowSize;
bool g_showRegWindow = true;
bool g_showDisassemblyWindow = true;
ImVec4 g_clearColor = ImColor(0, 0, 0);

// Main event loop
void main_loop(CPU& cpu, GPU& gpu, Joypad& joypad) {
  // Setup SDL and start new ImGui frame
  ImGui_ImplSdl_NewFrame(g_window);

  // Run the emulator until vsync or breakpoint
  while ((g_running || g_stepping) && !gpu.vsync) {
    g_stepping = false;

    cpu.checkInterrupts();
    cpu.execute();
    gpu.step(cpu.cpu_clock_t);

    // Check for breakpoint
    if (g_breakpoints.find(cpu.reg.pc) != g_breakpoints.end()) {
      g_running = false;
      g_scrollDisasmToPC = true;
    }
  }
  gpu.vsync = false;

  // Handle keydown, window close, etc
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSdl_ProcessEvent(&event);
    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_UP:
          case SDLK_DOWN:
          case SDLK_RIGHT:
          case SDLK_LEFT:
          case SDLK_z:
          case SDLK_x:
          case SDLK_RETURN:
          case SDLK_SPACE:
            joypad.keyPressed(event.key.keysym.sym);
            break;

          default:
            break;
        }
        break;

      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
          case SDLK_UP:
          case SDLK_DOWN:
          case SDLK_RIGHT:
          case SDLK_LEFT:
          case SDLK_z:
          case SDLK_x:
          case SDLK_RETURN:
          case SDLK_SPACE:
            joypad.keyReleased(event.key.keysym.sym);
            break;
          default:
            break;
        }
        break;

      case SDL_QUIT:
        g_quit = true;
        break;

      case SDL_JOYBUTTONDOWN:
        break;

      default:
        break;
    }
  }

  // Render GUI windows
  imguiLCD(gpu);
  imguiRegisters(cpu);
  imguiDisassembly(cpu);

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
  // Initialize emulator components
  CPU cpu;
  GPU gpu;
  Joypad joypad;
  gpu.mmu = &cpu.mmu;
  cpu.mmu.joypad = &joypad;
  gpu.reset();

  // Load ROM
#ifdef __EMSCRIPTEN__
  if (!cpu.mmu.load("roms/tetris.gb")) {
    printf("Invalid ROM file: roms/tetris.gb\n");
    return -1;
  }
#else
  if (argc < 2) {
    printf("Please specify a ROM file.\n");
    return -1;
  }
  if (!cpu.mmu.load(argv[1])) {
    printf("Invalid ROM file: %s\n", argv[1]);
    return -1;
  }
#endif

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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gpu.width, gpu.height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, gpu.screenData);

  // Main loop
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!g_quit) {
    main_loop(cpu, gpu, joypad);
  }
#endif

  // Cleanup
  // When you're done using the texture, delete it. This will set texname to 0
  // and
  // delete all of the graphics card memory associated with the texture. If you
  // don't call this method, the texture will stay in graphics card memory until
  // you close the application.
  glDeleteTextures(1, &g_lcdTexture);

  ImGui_ImplSdl_Shutdown();
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(g_window);
  SDL_Quit();

  return 0;
}

// Display LCD in a window
void imguiLCD(GPU& gpu) {
  // Update texture
  // bind the texture again when you want to update it.
  glBindTexture(GL_TEXTURE_2D, g_lcdTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gpu.width, gpu.height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, gpu.screenData);

  // Set up window flags
  ImGuiWindowFlags windowFlags = 0;
  windowFlags |= ImGuiWindowFlags_NoTitleBar;
  windowFlags |= ImGuiWindowFlags_NoScrollbar;
  windowFlags |= ImGuiWindowFlags_NoCollapse;

  // Set the window size to Gameboy LCD dimensions
  ImGui::SetNextWindowSize(ImVec2(gpu.width*2, gpu.height*2), ImGuiSetCond_Once);

  // No padding
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  // Render window
  ImGui::Begin("lcd", &g_showLcdWindow, windowFlags);
  g_LcdWindowSize = ImGui::GetWindowSize();
  ImGui::Image((void*)g_lcdTexture, g_LcdWindowSize);

  // Done, clean up
  ImGui::End();
  ImGui::PopStyleVar();
}

// Display registers in a window
void imguiRegisters(CPU& cpu) {
  ImGui::Begin("reg", &g_showRegWindow);

  ImGui::Text(
      "af = %04X\nbc = %04X\nde = %04X\nhl = %04X\nsp = %04X\npc = %04X",
      cpu.reg.af, cpu.reg.bc, cpu.reg.de, cpu.reg.hl, cpu.reg.sp, cpu.reg.pc);
  ImGui::End();
}

// Display disassembly in a window
void imguiDisassembly(CPU& cpu) {
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
      g_stepping = true;
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
    if (index == cpu.reg.pc) {
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

    // Display current address and opcode
    ImGui::SameLine();
    ImGui::TextColored(color, "%04X:%02X", index, cpu.mmu.memory[index]);

    // Display disassembly
    ImGui::SameLine();
    disassemble(cpu, index);
    ImGui::TextColored(color, g_disassembly.str.c_str(), g_disassembly.operand);
  }

  // Scroll to current PC if warranted
  if (!g_running && g_scrollDisasmToPC) {
    ImGui::SetScrollY(cpu.reg.pc * ImGui::GetTextLineHeight() * 0.5f);
    g_scrollDisasmToPC = false;
  }

  clipper.End();
  ImGui::EndChild();
  ImGui::End();
}

// This is a ugly, ugly hack
void disassemble(CPU& cpu, u16& pc) {
  u8 opcode = cpu.mmu.memory[pc];
  u8 operandSize = 0;
  g_disassembly.operand = 0;

  if (opcode == 0xCB) {
    operandSize = 1;
    g_disassembly.operand = cpu.mmu.memory[++pc];
    g_disassembly.str = cpu.instructions_CB[g_disassembly.operand].disassembly;
  } else if (cpu.instructions[opcode].operandLength == 1) {
    operandSize = 1;
    g_disassembly.operand = cpu.mmu.memory[++pc];
    g_disassembly.str = cpu.instructions[opcode].disassembly;
  } else if (cpu.instructions[opcode].operandLength == 2) {
    operandSize = 2;
    g_disassembly.operand = cpu.mmu.read16(++pc);
    g_disassembly.str = cpu.instructions[opcode].disassembly;
    pc++;
  } else {
    ++pc;
    operandSize = 0;
    g_disassembly.str = cpu.instructions[opcode].disassembly;
  }
  pc += operandSize;
}