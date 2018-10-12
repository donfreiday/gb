// gb: a Gameboy Emulator by Don Freiday
// File: main.cpp
// Description: Main loop and gui

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
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

void displayLCD();
void displayRegisters();

SDL_Window* g_window;
bool g_quit = false;
bool g_showLcdWindow = true;
ImVec4 g_clearColor = ImColor(0, 0, 0);
gb g_core;

void main_loop() {
  // Handle keydown, window close, etc
  handleSDLEvents();

  // Setup SDL and start new ImGui frame
  ImGui_ImplSdl_NewFrame(g_window);

  // Run the emulator until vsync
  while (!g_core.gpu.vsync) {
    g_core.step();
  }
  g_core.gpu.vsync = false;

  // Draw LCD Window
  displayLCD();

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
  g_core.debugEnabled = true;

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
void displayLCD() {
  // Set up window flags
  ImGuiWindowFlags windowFlags = 0;
  windowFlags |= ImGuiWindowFlags_NoTitleBar;
  windowFlags |= ImGuiWindowFlags_NoScrollbar;
  windowFlags |= ImGuiWindowFlags_NoCollapse;

  // Set the window size to Gameboy LCD dimensions
  ImGui::SetNextWindowSize(ImVec2(g_core.gpu.width, g_core.gpu.height), ImGuiSetCond_Once);

  // Window defaults to upper left corner (for now)
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);

  // No padding
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  // Render window
  ImGui::Begin("gb", &g_showLcdWindow, windowFlags);
  ImGui::Image((void*)g_core.gpu.texture, ImGui::GetWindowSize());

  // Done, clean up
  ImGui::End();
  ImGui::PopStyleVar();
}

void displayRegisters() {}