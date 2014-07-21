#pragma once

#include "D3DInclude.hpp"
#include "Essentials.hpp"

namespace lite
{
  // Stores a handle to a COM interface (most DirectX objects use COM).
  template <class T>
  class ComHandle
  {
  private: // data

    // Pointer to the COM interface
    T* pointer = nullptr;

  public: // methods

    ComHandle() = default;
    ComHandle(const ComHandle&) = delete;
    ComHandle& operator=(const ComHandle&) = delete;

    ComHandle(ComHandle&& b) :
      pointer(b.pointer)
    {
      b.pointer = nullptr;
    }

    ComHandle& operator=(ComHandle&& b)
    {
      pointer = b.pointer;
      b.pointer = nullptr;
      return *this;
    }

    ~ComHandle()
    {
      Reset();
    }

    // Returns a pointer to the interface* (interface**).
    T** Address() const
    {
      return const_cast<T**>(&pointer);
    }

    // Returns a pointer to the interface.
    T* Get() const
    {
      return pointer;
    }

    // Resets the interface pointer.
    template <class U = T>
    void Reset(U* pointer_ = nullptr)
    {
      if (pointer)
      {
        pointer->Release();
      }

      pointer = pointer_;
    }

    // Returns whether the handle is valid.
    explicit operator bool() const
    {
      return pointer != nullptr;
    }

    // Returns whether the handle is invalid.
    bool operator!() const
    {
      return !pointer;
    }

    // Accesses members of the interface.
    T* operator->() const
    {
      return pointer;
    }

    // Implicit conversion to interface*.
    operator T*() const
    {
      return pointer;
    }

    // Implicit conversion to interface**.
    operator T**() const
    {
      return Address();
    }
  };

  // COM handles to common Direct3D interfaces.
  typedef ComHandle<ID3D10Blob>                BlobHandle;
  typedef ComHandle<ID3D11Buffer>              BufferHandle;
  typedef ComHandle<ID3D11DepthStencilView>    DepthStencilViewHandle;
  typedef ComHandle<ID3D11Device>              DeviceHandle;
  typedef ComHandle<ID3D11DeviceContext>       DeviceContextHandle;
  typedef ComHandle<ID3D11GeometryShader>      GeometryShaderHandle;
  typedef ComHandle<ID3D11InputLayout>         InputLayoutHandle;
  typedef ComHandle<ID3D11PixelShader>         PixelShaderHandle;
  typedef ComHandle<ID3D11RasterizerState>     RasterizerStateHandle;
  typedef ComHandle<ID3D11RenderTargetView>    RenderTargetViewHandle;
  typedef ComHandle<ID3D11Resource>            ResourceHandle;
  typedef ComHandle<ID3D11SamplerState>        SamplerStateHandle;
  typedef ComHandle<ID3D11ShaderResourceView>  ShaderResourceViewHandle;
  typedef ComHandle<IDXGISwapChain>            SwapChainHandle;
  typedef ComHandle<ID3D11Texture2D>           Texture2DHandle;
  typedef ComHandle<IUnknown>                  UnknownHandle;
  typedef ComHandle<ID3D11VertexShader>        VertexShaderHandle;
} // namespace lite