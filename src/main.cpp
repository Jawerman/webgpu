// #include "webgpu-utils.h"
// #include <GLFW/glfw3.h>
// #include <webgpu/webgpu.h>
// #ifdef WEBGPU_BACKEND_WGPU
// #include <webgpu/wgpu.h>
// #endif // WEBGPU_BACKEND_WGPU

// #ifdef __EMSCRIPTEN__
// #include <emscripten.h>
// #endif // __EMSCRIPTEN__

// #include <cassert>
#include <iostream>
#include "application.hpp"

int main() {
  Application app;

  if (!app.Initialize()) {
    return 1;
  }

#ifdef __EMSCRIPTEN__
  auto callback = [](void *arg) {
    Application *pApp = reinterpret_cast<Application *>(arg);
    pApp->MainLoop();
  };
  emscripten_set_main_loop_arg(callback, &app, 0, true);
#else
  std::cout << "Before Loop" << std::endl;
  while (app.IsRunning()) {
    app.MainLoop();
  }
  std::cout << "After Loop again" << std::endl;
#endif // __EMSCRIPTEN__

  app.Terminate();
  return 0;
}
