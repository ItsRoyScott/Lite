#pragma once

#include "CameraDefinition.hpp"
#include "ComHandle.hpp"
#include "D3DInfo.hpp"
#include "DebugDrawer.hpp"
#include "EventHandler.hpp"
#include "ModelInstance.hpp"
#include <unordered_map>
#include "Window.hpp"

namespace lite
{
  class Graphics : public LightSingleton<Graphics>
  {
  private: // types

    struct SceneConstants
    {
      float4x4 viewProjection;
      array<float4, 2> lightDirections;
      array<float4, 2> lightColors;
    };

  private: // data

    BufferHandle cbScene;
    D3DInfo d3d;
    InputLayoutHandle inputLayout;
    size_t modelCounter = 0;
    unordered_map<size_t, shared_ptr<ModelInstance>> models;

  public: // data

    CameraDefinition Camera;

  public: // methods

    Graphics(Window& window)
    {
      // Create the device and swap chain.
      DXGI_SWAP_CHAIN_DESC swapChainDesc;
      ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
      swapChainDesc.BufferCount = 1;
      swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      swapChainDesc.BufferDesc.Height = window.Height;
      swapChainDesc.BufferDesc.Width = window.Width;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      swapChainDesc.OutputWindow = window.Handle;
      swapChainDesc.SampleDesc.Count = 1;
      swapChainDesc.Windowed = TRUE;

      UINT creationFlags = DebugMode ? D3D11_CREATE_DEVICE_DEBUG : 0;
      DX(D3D11CreateDeviceAndSwapChain(
        nullptr,                        // adapter pointer
        D3D_DRIVER_TYPE_HARDWARE,       // driver type
        nullptr,                        // software module handle
        creationFlags,                  // creation flags
        nullptr,                        // feature levels pointer
        0U,                             // number of feature levels
        D3D11_SDK_VERSION,              // D3D sdk version
        &swapChainDesc,                 // swap chain desc
        d3d.SwapChain,                  // swap chain object to be filled in
        d3d.Device,                     // device to be filled in
        nullptr,                        // feature levels to be filled in
        d3d.Context));                  // context object to be filled in

      // Define the input layout for vertices.
      D3D11_INPUT_ELEMENT_DESC vertexLayout [] =
      {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
      };

      ShaderData& vs = ShaderManager::Instance().Get(VertexShader, "Default.vs");
      if (!vs.IsLoaded()) return;
      DX(d3d.Device->CreateInputLayout(
        vertexLayout,                     // input element descs
        ARRAYSIZE(vertexLayout),          // number of descs
        vs.Bytecode()->GetBufferPointer(),  // shader bytecode
        vs.Bytecode()->GetBufferSize(),     // length of bytecode
        inputLayout));                    // input layout to be filled in

      // Create a render target view to the back buffer.
      Texture2DHandle backBuffer;
      DX(d3d.SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) backBuffer.Address()));
      DX(d3d.Device->CreateRenderTargetView(backBuffer, nullptr, d3d.RenderTarget));

      // Create the depth-stencil texture.
      D3D11_TEXTURE2D_DESC dsTexture;
      ZeroMemory(&dsTexture, sizeof(dsTexture));
      dsTexture.Width = (UINT) window.Width;
      dsTexture.Height = (UINT) window.Height;
      dsTexture.MipLevels = 1;
      dsTexture.ArraySize = 1;
      dsTexture.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      dsTexture.SampleDesc.Count = 1;
      dsTexture.BindFlags = D3D11_BIND_DEPTH_STENCIL;
      DX(d3d.Device->CreateTexture2D(&dsTexture, 0, d3d.DSTexture));

      // Create the depth-stencil view.
      D3D11_DEPTH_STENCIL_VIEW_DESC dsView;
      ZeroMemory(&dsView, sizeof(dsView));
      dsView.Format = dsTexture.Format;
      dsView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      DX(d3d.Device->CreateDepthStencilView(d3d.DSTexture, &dsView, d3d.DSView));

      // Set viewport information for the rasterizer stage.
      D3D11_VIEWPORT viewport;
      viewport.Width = (FLOAT) window.Width;
      viewport.Height = (FLOAT) window.Height;
      viewport.MinDepth = 0;
      viewport.MaxDepth = 1;
      viewport.TopLeftX = 0;
      viewport.TopLeftY = 0;
      d3d.Context->RSSetViewports(1, &viewport);

      // Create the scene constant buffer.
      {
        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(bufferDesc));
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.ByteWidth = sizeof(SceneConstants);
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        DX(d3d.Device->CreateBuffer(&bufferDesc, nullptr, cbScene));
      }

      Camera.Climb(1);
      Camera.SetLens(XM_PI / 3, float(window.Width) / float(window.Height), 0.01f, 1000);
      Camera.Walk(-10);

      D3D11_SAMPLER_DESC samplerDesc;
      ZeroMemory(&samplerDesc, sizeof(samplerDesc));
      samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
      samplerDesc.MinLOD = 0;
      samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
      DX(d3d.Device->CreateSamplerState(&samplerDesc, d3d.LinearSampler));

      D3D11_RASTERIZER_DESC rasterDesc;
      ZeroMemory(&rasterDesc, sizeof(rasterDesc));
      rasterDesc.AntialiasedLineEnable = false;
      rasterDesc.CullMode = D3D11_CULL_NONE;
      rasterDesc.DepthBias = 0;
      rasterDesc.DepthBiasClamp = 0;
      rasterDesc.DepthClipEnable = true;
      rasterDesc.FillMode = D3D11_FILL_SOLID;
      rasterDesc.FrontCounterClockwise = false;
      rasterDesc.MultisampleEnable = false;
      rasterDesc.ScissorEnable = false;
      rasterDesc.SlopeScaledDepthBias = 0;
      d3d.Device->CreateRasterizerState(&rasterDesc, d3d.NoCullRasterizer);
    }

    // Adds a new model instance.
    shared_ptr<ModelInstance> AddModel()
    {
      auto result = models.emplace(modelCounter++, make_shared<ModelInstance>());
      auto it = result.first;
      return it->second;
    }

    void Update(float dt)
    {
      // Remove all models no longer being referenced.
      for (auto it = models.begin(); it != models.end();)
      {
        if (it->second.unique())
        {
          it = models.erase(it);
        }
        else
        {
          ++it;
        }
      }

      if (!d3d.Context || !d3d.RenderTarget) return;

      // Clear the render target.
      float clearColor [] = { 0.0f, 0.125f, 0.6f, 1.0f };
      d3d.Context->ClearRenderTargetView(d3d.RenderTarget, clearColor);

      // Clear the depth buffer to max depth (1.0).
      d3d.Context->ClearDepthStencilView(d3d.DSView, D3D11_CLEAR_DEPTH, 1, 0);

      // Set the vertex layout for the input assembler stage.
      d3d.Context->IASetInputLayout(inputLayout);
      // Set the render target to the depth-stencil view.
      d3d.Context->OMSetRenderTargets(1, d3d.RenderTarget, d3d.DSView);

      // Some hard-coded light directions and colors.
      XMFLOAT4 lightDirections [] =
      {
        { -.577f, .577f, -.577f, 1 },
        { 0, 0, -1, 1 }
      };
      XMFLOAT4 lightColors [] =
      {
        { 1, 1, 1, 1 },
        { 1, 1, 0, 1 }
      };

      SceneConstants sceneConstants;

      // Update elapsed time.
      static float t = 0.0f;
      static DWORD dwTimeStart = 0;
      DWORD dwTimeCur = GetTickCount();
      if (dwTimeStart == 0)
        dwTimeStart = dwTimeCur;
      t = (dwTimeCur - dwTimeStart) / 1000.0f;

      // Rotate the second light's direction.
      auto rotate = XMMatrixRotationY(-2 * t);
      XMStoreFloat4(&lightDirections[1], XMVector3Transform(XMLoadFloat4(&lightDirections[1]), rotate));

      // Store the view projection and light info in the constant buffer.
      XMStoreFloat4x4(
        &sceneConstants.viewProjection,
        XMMatrixTranspose(
        XMLoadFloat4x4(&Camera.ViewProjectionMatrix())));
      sceneConstants.lightDirections = { lightDirections[0], lightDirections[1] };
      sceneConstants.lightColors = { lightColors[0], lightColors[1] };

      d3d.Context->UpdateSubresource(cbScene, 0, nullptr, &sceneConstants, 0, 0);
      d3d.Context->VSSetConstantBuffers(0, 1, cbScene);
      d3d.Context->PSSetConstantBuffers(0, 1, cbScene);

      // Draw all models.
      for (auto& modelPair : models)
      {
        modelPair.second->Draw();
      }

      // Debug drawing.
      DebugDrawer::Instance().Update();

      // Present the back buffer to the display.
      d3d.SwapChain->Present(0, 0);
    }
  };
} // namespace lite