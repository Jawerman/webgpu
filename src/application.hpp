#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <webgpu/webgpu.hpp>

using namespace wgpu;

class Application {
public:
  // Initialize everything and return true if it went all right
  bool Initialize();

  // Uninitialize everything that was initialized
  void Terminate();

  // Draw a frame and handle events
  void MainLoop();

  // Return true as long as the main loop should keep on running
  bool IsRunning();

private:
  GLFWwindow *m_window;
  Device m_device;
  Queue m_queue;
  Surface m_surface;
  std::unique_ptr<ErrorCallback> m_uncapturedErrorCallbackHandle;

  TextureView GetNextSurfaceTextureView();
};
