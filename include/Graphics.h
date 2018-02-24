//
//  Graphics.h
//

#pragma once

#include <memory>
#include <glm/glm.hpp>

#if defined(_DIRECTX)
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#endif

namespace dg {

  class Window;
  class OpenGLGraphics;
  class DirectXGraphics;

  class Graphics {

    public:

#if defined(_OPENGL)
      typedef OpenGLGraphics graphics_class;
#elif defined(_DIRECTX)
      typedef DirectXGraphics graphics_class;
#endif

      static std::unique_ptr<graphics_class> Instance;

      static void Initialize(const Window& window);
      static void Shutdown();

      virtual ~Graphics() = default;

      virtual void OnWindowResize(const Window& window) {};

      virtual void Clear(glm::vec3 color) = 0;

    protected:

      virtual void InitializeGraphics() = 0;
      virtual void InitializeResources();

  }; // class Graphics

#if defined(_OPENGL)
  class OpenGLGraphics : public Graphics {
    friend class Graphics;

    public:

      OpenGLGraphics(const Window& window);
      virtual ~OpenGLGraphics();

      virtual void Clear(glm::vec3 color);

    protected:

      virtual void InitializeGraphics();
      virtual void InitializeResources();

  }; // class OpenGLGraphics
#endif

#if defined(_DIRECTX)
  class DirectXGraphics : public Graphics {
    friend class Graphics;

    public:

      DirectXGraphics(const Window& window);
      virtual ~DirectXGraphics();

      virtual void OnWindowResize(const Window& window);

      virtual void Clear(glm::vec3 color);

      ID3D11Device *device;
      ID3D11DeviceContext *context;
      D3D_FEATURE_LEVEL dxFeatureLevel;
      IDXGISwapChain *swapChain;

      ID3D11RenderTargetView *backBufferRTV;
      ID3D11DepthStencilView *depthStencilView;

    protected:

      virtual void InitializeGraphics();

    private:

      const Window& window;
      glm::vec2 contentSize;

  }; // class OpenGLGraphics
#endif

} // namespace dg
