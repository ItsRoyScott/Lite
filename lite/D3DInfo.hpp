#pragma once

#include "ComHandle.hpp"

namespace lite
{
  // Structure containing the usual necessary D3D components.
  struct D3DInfo : LightSingleton < D3DInfo >
  {
    // The D3D device's current context.
    DeviceContextHandle Context;

    // Handle to the depth-stencil texture.
    Texture2DHandle DSTexture;

    // View of the depth-stencil buffer.
    DepthStencilViewHandle DSView;

    // The D3D device.
    DeviceHandle Device;

    // Default sampler state.
    SamplerStateHandle LinearSampler;

    // Rasterizer state which doesn't cull backfaces.
    RasterizerStateHandle NoCullRasterizer;

    // View of the render target (screen we render to by default).
    RenderTargetViewHandle RenderTarget;

    // The D3D swap chain (double-buffering).
    SwapChainHandle SwapChain;
  };
} // namespace lite