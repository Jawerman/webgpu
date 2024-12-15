#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

#include "application.hpp"
#include "webgpu-utils.h"
#include <iostream>

using namespace wgpu;

bool Application::Initialize() {
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  m_window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
    glfwTerminate();
    return false;
  }

  // Get instance
  Instance instance = wgpuCreateInstance(nullptr);
  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return false;
  }

  // Get adapter
  std::cout << "Requesting Adapter..." << std::endl;
  m_surface = glfwGetWGPUSurface(instance, m_window);
  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.compatibleSurface = m_surface;
  Adapter adapter = instance.requestAdapter(adapterOpts);
  std::cout << "Got adapter: " << adapter << std::endl;

  instance.release();
  // inspectAdapter(adapter);

  std::cout << "Requesting device..." << std::endl;
  DeviceDescriptor deviceDesc = {};
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

  m_device = adapter.requestDevice(deviceDesc);
  std::cout << "Got device" << m_device << std::endl;
  // inspectDevice(m_device);

  m_uncapturedErrorCallbackHandle = m_device.setUncapturedErrorCallback(
      [](ErrorType type, char const *message) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message)
          std::cerr << " (" << message << ")";
        std::cerr << std::endl;
      });

  m_queue = m_device.getQueue();

  // Configura surface
  SurfaceConfiguration surfaceConfig = {};
  surfaceConfig.width = 640;
  surfaceConfig.height = 480;
  surfaceConfig.usage = TextureUsage::RenderAttachment;
  TextureFormat surfaceFormat = m_surface.getPreferredFormat(adapter);
  surfaceConfig.format = surfaceFormat;

  surfaceConfig.viewFormatCount = 0;
  surfaceConfig.viewFormats = nullptr;
  surfaceConfig.device = m_device;
  surfaceConfig.presentMode = PresentMode::Fifo;
  surfaceConfig.alphaMode = CompositeAlphaMode::Auto;

  m_surface.configure(surfaceConfig);

  adapter.release();

  return true;
}

void Application::Terminate() {
  m_surface.unconfigure();
  m_queue.release();
  m_surface.release();
  m_device.release();
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Application::MainLoop() {
  glfwPollEvents();

  TextureView targetView = GetNextSurfaceTextureView();
  if (!targetView)
    return;

  CommandEncoderDescriptor encoderDesc = {};
  encoderDesc.label = "My command encoder";
  CommandEncoder encoder =
      wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);

  RenderPassDescriptor renderPassDesc = {};

 RenderPassColorAttachment renderPassColorAttachment = {};
  renderPassColorAttachment.view = targetView;
  renderPassColorAttachment.resolveTarget = nullptr;
  renderPassColorAttachment.loadOp = LoadOp::Clear;
  renderPassColorAttachment.storeOp = StoreOp::Store;
  renderPassColorAttachment.clearValue = WGPUColor{1.0, 0.0, 0.0, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
  renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // !WEBGPU_BACKEND_WGPU

  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &renderPassColorAttachment;
  renderPassDesc.depthStencilAttachment = nullptr;
  renderPassDesc.timestampWrites = nullptr;

  RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
  renderPass.end();
  renderPass.release();

  CommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.label = "Command buffer";
  CommandBuffer command = encoder.finish(cmdBufferDescriptor);
  encoder.release();

  std::cout << "Submitting command" << std::endl;
  m_queue.submit(1, &command);
  command.release();
  std::cout << "Command submitted." << std::endl;

  targetView.release();
  // wgpuTextureViewRelease(targetView);
#ifndef __EMSCRIPTEN__
  m_surface.present();
#endif // !__EMSCRIPTEN__

#if defined(WEBGPU_BACKEND_DAWN)
  m_device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
  m_device.poll(false);
#endif
}

bool Application::IsRunning() { return !glfwWindowShouldClose(m_window); }

TextureView Application::GetNextSurfaceTextureView() {
  SurfaceTexture surfaceTexture;
  m_surface.getCurrentTexture(&surfaceTexture);

  if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
    return nullptr;
  }

  Texture texture = surfaceTexture.texture;

  TextureViewDescriptor viewDescriptor;
  viewDescriptor.label = "Surface teture view";
  viewDescriptor.format = texture.getFormat();
  viewDescriptor.dimension = TextureViewDimension::_2D;
  viewDescriptor.baseMipLevel = 0;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.baseArrayLayer = 0;
  viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.aspect = TextureAspect::All;
  WGPUTextureView targetView = texture.createView(viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
  texture.release();
#endif // !WEBGPU_BACKEND_WGPU
  return targetView;
}
