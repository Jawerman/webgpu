#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <iostream>
#include <webgpu/webgpu.h>

#include "application.hpp"
#include "dawn/dawn_proc_table.h"
#include "webgpu-utils.h"

bool Application::Initialize() {
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  this->window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

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
  this->surface = glfwGetWGPUSurface(instance, this->window);

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;
  adapterOpts.compatibleSurface = this->surface;

  WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
  std::cout << "Got adapter: " << adapter << std::endl;
  inspectAdapter(adapter);

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
  inspectDevice(this->device);

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

  // Configura surface
  WGPUSurfaceConfiguration surfaceConfig = {};

  WGPUTextureFormat surfaceFormat =
      wgpuSurfaceGetPreferredFormat(this->surface, adapter);
  surfaceConfig.format = surfaceFormat;
  surfaceConfig.viewFormatCount = 0;
  surfaceConfig.viewFormats = nullptr;
  surfaceConfig.width = 640;
  surfaceConfig.height = 480;
  surfaceConfig.nextInChain = nullptr;
  surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
  surfaceConfig.device = device;
  surfaceConfig.presentMode = WGPUPresentMode_Fifo;
  surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;

  wgpuSurfaceConfigure(this->surface, &surfaceConfig);

  wgpuInstanceRelease(instance);
  wgpuAdapterRelease(adapter);
  return true;
}

void Application::Terminate() {
  wgpuSurfaceUnconfigure(this->surface);
  wgpuQueueRelease(this->queue);
  wgpuSurfaceRelease(this->surface);
  wgpuDeviceRelease(this->device);
  glfwDestroyWindow(this->window);
  glfwTerminate();
}

void Application::MainLoop() {
  glfwPollEvents();

  WGPUTextureView targetView = GetNextSurfaceTextureView();
  if (!targetView)
    return;

  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  encoderDesc.label = "My command encoder";
  WGPUCommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(this->device, &encoderDesc);

  WGPURenderPassDescriptor renderPassDesc = {};
  renderPassDesc.nextInChain = nullptr;

  WGPURenderPassColorAttachment renderPassColorAttachment = {};
  renderPassColorAttachment.view = targetView;
  renderPassColorAttachment.resolveTarget = nullptr;
  renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
  renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
  renderPassColorAttachment.clearValue = WGPUColor{1.0, 0.0, 0.0, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
  renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // !WEBGPU_BACKEND_WGPU

  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &renderPassColorAttachment;
  renderPassDesc.depthStencilAttachment = nullptr;
  renderPassDesc.timestampWrites = nullptr;

  WGPURenderPassEncoder renderPass =
      wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
  wgpuRenderPassEncoderEnd(renderPass);
  wgpuRenderPassEncoderRelease(renderPass);

  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.nextInChain = nullptr;
  cmdBufferDescriptor.label = "Command buffer";
  WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
  wgpuCommandEncoderRelease(encoder);

  std::cout << "Submitting command" << std::endl;
  wgpuQueueSubmit(queue, 1, &command);
  wgpuCommandBufferRelease(command);
  std::cout << "Command submitted." << std::endl;


  wgpuTextureViewRelease(targetView);
#ifndef __EMSCRIPTEN__
  wgpuSurfacePresent(this->surface);
#endif // !__EMSCRIPTEN__

#if defined(WEBGPU_BACKEND_DAWN)
  wgpuDeviceTick(this->device);
#elif defined(WEBGPU_BACKEND_WGPU)
  wgpuDevicePoll(this->device, false, nullptr);
#endif
}

bool Application::IsRunning() { return !glfwWindowShouldClose(this->window); }

WGPUTextureView Application::GetNextSurfaceTextureView() {
  WGPUSurfaceTexture surfaceTexture;
  wgpuSurfaceGetCurrentTexture(this->surface, &surfaceTexture);

  if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
    return nullptr;
  }

  WGPUTextureViewDescriptor viewDescriptor;
  viewDescriptor.nextInChain = nullptr;
  viewDescriptor.label = "Surface teture view";
  viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
  viewDescriptor.dimension = WGPUTextureViewDimension_2D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.aspect = WGPUTextureAspect_All;
  WGPUTextureView targetView =
      wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
  wgpuTextureRelease(surfaceTexture.texture);
#endif // !WEBGPU_BACKEND_WGPU
  return targetView;
}
