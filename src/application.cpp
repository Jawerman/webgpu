#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <webgpu/webgpu.h>

#include "application.hpp"
#include "webgpu-utils.h"

bool Application::Initialize() {
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  this->window =
      glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

  if (!this->window) {
    std::cerr << "Could not open window!" << std::endl;
    glfwTerminate();
    return false;
  }

  // Get instance
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_EMSCRIPTEN
  WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
  WGPUInstance instance = wgpuCreateInstance(&desc);
#endif

  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return false;
  }

  // Get adapter
  std::cout << "Requesting Adapter..." << std::endl;
  this->surface = glfwCreateWindowWGPUSurface(instance, this->window);

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;
  adapterOpts.compatibleSurface = this->surface;

  WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
  std::cout << "Got adapter: " << adapter << std::endl;
  wgpuInstanceRelease(instance);

  // Get device
  std::cout << "Requesting device..." << std::endl;
  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.nextInChain = nullptr;
  deviceDesc.label = "My device";
  deviceDesc.requiredFeatureCount = 0;
  deviceDesc.requiredLimits = nullptr;
  deviceDesc.defaultQueue.nextInChain = nullptr;
  deviceDesc.defaultQueue.label = "The default queue";
  deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason,
                                     char const *message,
                                     void * /*pUserData*/) {
    std::cerr << "Device lost: reason " << reason;
    if (message)
      std::cerr << " (" << message << ")";
    std::cerr << std::endl;
  };

  this->device = requestDeviceSync(adapter, &deviceDesc);
  std::cout << "Got device" << device << std::endl;
  wgpuAdapterRelease(adapter);

  wgpuDeviceSetUncapturedErrorCallback(
      device,
      [](WGPUErrorType type, char const *message, void * /*pUserData*/) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message)
          std::cerr << " ( " << message << ")";
        std::cerr << std::endl;
      },
      nullptr);

  this->queue = wgpuDeviceGetQueue(device);
  return true;
}

void Application::Terminate() {
  wgpuQueueRelease(this->queue);
  wgpuSurfaceRelease(this->surface);
  wgpuDeviceRelease(this->device);
  glfwDestroyWindow(this->window);
  glfwTerminate();
}

void Application::MainLoop() {
  glfwPollEvents();

#if defined(WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(this->device);
#elif defined(WEBGPU_BACKEND_WGPU)
  wgpuDevicePoll(this->device, false, nullptr);
#endif
}

bool Application::IsRunning() { return !glfwWindowShouldClose(this->window); }
