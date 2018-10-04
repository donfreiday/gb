// ImGui - standalone example application for SDL2 + OpenGL ES 2 + Emscripten

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>
#include "gb.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

bool g_done = false;
SDL_Window* g_window;
bool g_show_lcd_window = true;
ImVec4 g_clear_color = ImColor(114, 144, 154);

gb core;

void main_loop() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSdl_ProcessEvent(&event);
    if (event.type == SDL_QUIT) g_done = true;
  }
  ImGui_ImplSdl_NewFrame(g_window);

  while (!core.gpu.vsync) {
    core.step();
  }
  core.gpu.vsync = false;

   ImGui::SetNextWindowSize(ImVec2(core.gpu.width, core.gpu.height));
  ImGui::Begin("gb", &g_show_lcd_window);
  ImGui::Image((void*)core.gpu.texture,
               ImVec2(core.gpu.width, core.gpu.height));
  ImGui::End();

  ImGui::ShowTestWindow();

    // Rendering
  glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x,
             (int)ImGui::GetIO().DisplaySize.y);
  glClearColor(g_clear_color.x, g_clear_color.y, g_clear_color.z,
               g_clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui::Render();
  SDL_GL_SwapWindow(g_window);
}

int main(int argc, char** argv) {
  core.debugEnabled = true;
#ifndef __EMSCRIPTEN__

  if (argc < 2) {
    printf("Please specify a ROM file.\n");
    return -1;
  }
  if (!core.loadROM(argv[1])) {
    printf("Invalid ROM file: %s\n", argv[1]);
    return -1;
  }
#else
  // Emscripten ROM load and mainLoop callback setup
  // Debugger is implemented in curses, not yet working with emscripten
  if (!core.loadROM("roms/tetris.gb")) {
    printf("Invalid ROM file: roms/tetris.gb\n");
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
  g_window =
      SDL_CreateWindow("ImGui SDL2+OpenGLES+Emscripten example",
                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280,
                       720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  SDL_GLContext glcontext = SDL_GL_CreateContext(g_window);

  // Setup ImGui binding
  ImGui_ImplSdl_Init(g_window);

  // Load Fonts
  // (see extra_fonts/README.txt for more details)
  // ImGuiIO& io = ImGui::GetIO();
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // NULL, io.Fonts->GetGlyphRangesJapanese());

  // Merge glyphs from multiple fonts into one (e.g. combine default font with
  // another with Chinese glyphs, or add icons)
  // ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 };
  // ImFontConfig icons_config; icons_config.MergeMode = true;
  // icons_config.PixelSnapH = true;
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 18.0f);
  // io.Fonts->AddFontFromFileTTF("../../extra_fonts/fontawesome-webfont.ttf", 18.0f,
  // &icons_config, icons_ranges);

  // Main loop
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!g_done) {
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
