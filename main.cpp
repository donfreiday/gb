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

#ifdef __clang__
#pragma clang diagnostic ignored \
    "-Wint-to-void-pointer-cast"  // warning : cast to 'void *' from smaller
                                  // integer type 'int'
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
    switch (event.type) {
      case SDL_KEYDOWN:
        core.handleSDLKeydown(event.key.keysym.sym);
        break;

      case SDL_KEYUP:
        core.handleSDLKeyup(event.key.keysym.sym);
        break;

      case SDL_QUIT:
        g_done = true;
        break;

      default:
        break;
    }
  }
  ImGui_ImplSdl_NewFrame(g_window);

  while (!core.gpu.vsync) {
    core.step();
  }
  core.gpu.vsync = false;

  // Display LCD in a window
  ImGui::SetNextWindowContentSize(ImVec2(core.gpu.width, core.gpu.height));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("gb", &g_show_lcd_window);
  ImGui::Image((void*)core.gpu.texture,
               ImVec2(core.gpu.width, core.gpu.height));
  ImGui::End();
  ImGui::PopStyleVar();

  // Main debug window
  ImGui::Begin("debugger");
  ImGui::BeginGroup();
  ImGui::BeginChild("disassembly", ImVec2(100.0f, 200.0f), true);
  ImGui::EndChild();
  ImGui::EndGroup();
  ImGui::End();

  /*ImGui::TextWrapped(
      "(Use SetScrollHere() or SetScrollFromPosY() to scroll to a given "
      "position.)");
  static bool track = true;
  static int track_line = 50, scroll_to_px = 200;
  ImGui::Checkbox("Track", &track);
  ImGui::SameLine(130);
  track |= ImGui::DragInt("##line", &track_line, 0.25f, 0, 99, "Line %.0f");
  bool scroll_to = ImGui::Button("Scroll To");
  ImGui::SameLine(130);
  scroll_to |=
      ImGui::DragInt("##pos_y", &scroll_to_px, 1.00f, 0, 9999, "y = %.0f px");
  if (scroll_to) track = false;

  for (int i = 0; i < 5; i++) {
    if (i > 0) ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text(
        "%s", i == 0 ? "Top"
                     : i == 1 ? "25%"
                              : i == 2 ? "Center" : i == 3 ? "75%" : "Bottom");
    ImGui::BeginChild(ImGui::GetID((void*)i),
                      ImVec2(ImGui::GetWindowWidth() * 0.17f, 200.0f), true);
    if (scroll_to)
      ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + scroll_to_px,
                               i * 0.25f);
    for (int line = 0; line < 100; line++) {
      if (track && line == track_line) {
        ImGui::TextColored(ImColor(255, 255, 0), "Line %d", line);
        ImGui::SetScrollHere(i * 0.25f);  // 0.0f:top, 0.5f:center, 1.0f:bottom
      } else {
        ImGui::Text("Line %d", line);
      }
    }
    ImGui::EndChild();
    ImGui::EndGroup();
  }*/

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
