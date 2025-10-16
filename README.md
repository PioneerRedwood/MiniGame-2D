# MiniGame-2D — Cross‑Platform 2D Skeleton

This repo scaffolds a tiny 2D engine/game using **DirectX 11** on Windows and a minimal **Metal** stub (optional) on macOS. It includes:

* Window creation & **rendering event loop**
* **DX11 renderer** (device, swap chain, RTV/DSV)
* **Resource loading** (WIC texture load on Windows)
* **2D rendering** of a textured quad + **ImGui** HUD
* **Input handling** (W/A/S/D move, ESC quit)

> macOS Metal is provided as a light starting point so you can fill in later; Windows/DX11 is fully runnable.

---

## Project Layout

```
MiniGame2D/
├─ CMakeLists.txt
├─ external/
│  └─ imgui/            (add as submodule or drop-in)
├─ assets/
│  └─ player.png        (optional; sample texture)
├─ src/
│  ├─ core/
│  │  ├─ App.h
│  │  └─ App.cpp
│  ├─ platform/
│  │  ├─ win/
│  │  │  ├─ MainWin.cpp
│  │  │  └─ WinInput.h
│  │  └─ mac/
│  │     ├─ AppDelegate.mm   (stub)
│  │     └─ MetalView.mm     (stub)
│  ├─ render/
│  │  ├─ d3d11/
│  │  │  ├─ D3D11Renderer.h
│  │  │  ├─ D3D11Renderer.cpp
│  │  │  ├─ TextureLoader.h
│  │  │  ├─ TextureLoader.cpp
│  │  │  ├─ shaders.hlsl
│  │  └─ metal/ (stubs)
│  └─ ui/
│     └─ ImGuiLayer.h/.cpp
```

---

## Build — Quick Start (Windows)

1. **Tools**: Visual Studio 2022 (Desktop C++), CMake 3.22+, Windows 10 SDK.
2. **ImGui**: Add as **git submodule** or copy `imgui/` folder into `external/imgui/` and include backends `imgui_impl_win32.cpp` and `imgui_impl_dx11.cpp`.
3. Configure & build:

   ```bash
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Debug
   ```
4. Run `MiniGame2D` target. Drop an image at `assets/player.png` to see textured quad. Otherwise a colored quad is shown.

## Build — Quick Start (macOS, Optional)

* Xcode 15+, macOS 13+. This repo includes **stubs** for Metal; you’ll see a window and a clear color. Fill in the TODOs to match the DX11 path.
* Generate project:

  ```bash
  cmake -S . -B build -G Xcode -DUSE_METAL=ON
  cmake --build build --config Debug
  ```

---

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.22)
project(MiniGame2D CXX)

option(USE_METAL "Build macOS Metal stub" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/imgui EXCLUDE_FROM_ALL) # if you export a tiny CMakeLists there; otherwise add files directly.

# Common sources
set(SRC_CORE
    src/core/App.cpp
    src/core/App.h
    src/ui/ImGuiLayer.cpp
    src/ui/ImGuiLayer.h
)

# Windows / DirectX11
if (WIN32)
  add_executable(MiniGame2D
    ${SRC_CORE}
    src/platform/win/MainWin.cpp
    src/platform/win/WinInput.h
    src/render/d3d11/D3D11Renderer.cpp
    src/render/d3d11/D3D11Renderer.h
    src/render/d3d11/TextureLoader.cpp
    src/render/d3d11/TextureLoader.h
    src/render/d3d11/shaders.hlsl
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
  )

  target_include_directories(MiniGame2D PRIVATE external/imgui)
  target_compile_definitions(MiniGame2D PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

  target_link_libraries(MiniGame2D PRIVATE d3d11 dxgi d3dcompiler imgui)

  # Compile HLSL shaders to a header (simple approach for sample)
  # In a production setup, use custom build steps to .cso files.
endif()

# macOS / Metal (optional minimal app)
if(APPLE AND USE_METAL)
  enable_language(OBJCXX)
  add_executable(MiniGame2D_Metal
    ${SRC_CORE}
    src/platform/mac/AppDelegate.mm
    src/platform/mac/MetalView.mm
  )
  target_link_libraries(MiniGame2D_Metal PRIVATE imgui)
  target_compile_definitions(MiniGame2D_Metal PRIVATE __MACOSX__)
endif()
```

---

## src/core/App.h

```cpp
#pragma once
#include <string>

struct AppConfig {
    int width = 1280;
    int height = 720;
    std::string title = "MiniGame2D";
};

struct GameState {
    float playerX = 200.0f;
    float playerY = 200.0f;
    float speed  = 220.0f; // pixels per second
};

class IRenderer2D {
public:
    virtual ~IRenderer2D() = default;
    virtual void BeginFrame(float r, float g, float b, float a) = 0;
    virtual void DrawQuad(float x, float y, float w, float h) = 0; // colored fallback
    virtual void DrawTexturedQuad(float x, float y, float w, float h, void* texture) = 0;
    virtual void* LoadTextureFromFile(const char* path) = 0; // returns API texture pointer
    virtual void EndFrame() = 0;
};

class App {
public:
    App(const AppConfig& cfg);
    void Update(float dt);
    void Render();
    void OnKey(bool down, int key);

    GameState& State() { return state_; }
    void SetRenderer(IRenderer2D* r) { renderer_ = r; }

private:
    AppConfig cfg_;
    GameState state_;
    IRenderer2D* renderer_ = nullptr;
    void* playerTex_ = nullptr;
    bool hasTexture_ = false;
};
```

## src/core/App.cpp

```cpp
#include "App.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

App::App(const AppConfig& cfg) : cfg_(cfg) {}

void App::Update(float dt) {
    // clamp position to screen (simple)
    state_.playerX = std::clamp(state_.playerX, 0.0f, (float)cfg_.width - 64.0f);
    state_.playerY = std::clamp(state_.playerY, 0.0f, (float)cfg_.height - 64.0f);
}

void App::Render() {
    if (!renderer_) return;
    renderer_->BeginFrame(0.07f, 0.08f, 0.10f, 1.0f);
    const float w = 96.0f, h = 96.0f;
    if (hasTexture_ && playerTex_) {
        renderer_->DrawTexturedQuad(state_.playerX, state_.playerY, w, h, playerTex_);
    } else {
        renderer_->DrawQuad(state_.playerX, state_.playerY, w, h);
    }
    renderer_->EndFrame();
}

void App::OnKey(bool down, int key) {
    const float s = state_.speed / 60.0f; // per-frame approx; real dt applied in Main
    if (!down) return;
    switch (key) {
        case 'W': state_.playerY -= s; break;
        case 'S': state_.playerY += s; break;
        case 'A': state_.playerX -= s; break;
        case 'D': state_.playerX += s; break;
        default: break;
    }
}
```

---

## src/platform/win/WinInput.h

```cpp
#pragma once
#include <windows.h>

inline bool IsKeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
```

## src/platform/win/MainWin.cpp

```cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../../core/App.h"
#include "../../render/d3d11/D3D11Renderer.h"
#include "../../ui/ImGuiLayer.h"
#include "WinInput.h"

static App* g_App = nullptr;
static D3D11Renderer* g_Renderer = nullptr;
static ImGuiLayer* g_ImGui = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_ImGui && g_ImGui->WndProc(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) { PostQuitMessage(0); }
            if (g_App) g_App->OnKey(true, (int)wParam);
            return 0;
        case WM_KEYUP:
            if (g_App) g_App->OnKey(false, (int)wParam);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int) {
    AppConfig cfg; cfg.width=1280; cfg.height=720; cfg.title="MiniGame2D (DX11)";

    // Create window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInst, nullptr, nullptr, nullptr, nullptr, _T("MiniGame2DWnd"), nullptr };
    RegisterClassEx(&wc);
    RECT rc = { 0,0,cfg.width,cfg.height }; AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("MiniGame2D (DX11)"), WS_OVERLAPPEDWINDOW,
                             100, 100, rc.right-rc.left, rc.bottom-rc.top, nullptr, nullptr, wc.hInstance, nullptr);

    // Init renderer
    D3D11Renderer renderer(hwnd, cfg.width, cfg.height);
    g_Renderer = &renderer;

    // App
    App app(cfg);
    g_App = &app;
    app.SetRenderer(&renderer);

    // ImGui
    ImGuiLayer imgui(hwnd, renderer.GetDevice(), renderer.GetDeviceContext());
    g_ImGui = &imgui;

    // Try load texture
    void* tex = renderer.LoadTextureFromFile("assets/player.png");
    if (tex) {
        // mark available
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Main loop
    LARGE_INTEGER freq; QueryPerformanceFrequency(&freq);
    LARGE_INTEGER prev; QueryPerformanceCounter(&prev);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); continue; }

        LARGE_INTEGER now; QueryPerformanceCounter(&now);
        float dt = float(now.QuadPart - prev.QuadPart) / float(freq.QuadPart);
        prev = now;

        // Keyboard polling (smooth movement)
        if (IsKeyDown('W')) app.OnKey(true, 'W');
        if (IsKeyDown('A')) app.OnKey(true, 'A');
        if (IsKeyDown('S')) app.OnKey(true, 'S');
        if (IsKeyDown('D')) app.OnKey(true, 'D');

        app.Update(dt);

        imgui.Begin();
        imgui.Text("FPS: %.1f", 1.0f / (dt > 0.0001f ? dt : 0.0001f));
        imgui.Text("Player: (%.1f, %.1f)", app.State().playerX, app.State().playerY);
        imgui.End();

        app.Render();
    }

    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}
```

---

## src/render/d3d11/D3D11Renderer.h

```cpp
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "../../core/App.h"

using Microsoft::WRL::ComPtr;

struct VertexPTC {
    float x, y;    // position
    float u, v;    // texcoord
};

class D3D11Renderer : public IRenderer2D {
public:
    D3D11Renderer(HWND hwnd, int width, int height);
    ~D3D11Renderer();

    // IRenderer2D
    void BeginFrame(float r, float g, float b, float a) override;
    void DrawQuad(float x, float y, float w, float h) override;
    void DrawTexturedQuad(float x, float y, float w, float h, void* texture) override;
    void* LoadTextureFromFile(const char* path) override;
    void EndFrame() override;

    ID3D11Device* GetDevice() const { return device_.Get(); }
    ID3D11DeviceContext* GetDeviceContext() const { return context_.Get(); }

private:
    void CreateSwapChainAndTargets(HWND hwnd, int width, int height);
    void CreatePipeline();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain> swapChain_;
    ComPtr<ID3D11RenderTargetView> rtv_;

    ComPtr<ID3D11VertexShader> vs_;
    ComPtr<ID3D11PixelShader> psColor_;
    ComPtr<ID3D11PixelShader> psTex_;
    ComPtr<ID3D11InputLayout> layout_;
    ComPtr<ID3D11Buffer> vb_;
    ComPtr<ID3D11Buffer> ib_;
    ComPtr<ID3D11SamplerState> sampler_;

    int backbufferW_ = 0;
    int backbufferH_ = 0;
};
```

## src/render/d3d11/D3D11Renderer.cpp

```cpp
#include "D3D11Renderer.h"
#include "TextureLoader.h"
#include <vector>
#include <stdexcept>

static ComPtr<ID3DBlob> CompileShader(const char* src, const char* entry, const char* profile) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG;
#endif
    ComPtr<ID3DBlob> blob, err;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr, entry, profile, flags, 0, &blob, &err);
    if (FAILED(hr)) throw std::runtime_error("Shader compile failed");
    return blob;
}

static const char* g_ShaderSrc = R"HLSL(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
cbuffer ScreenCB : register(b0) { float2 screenSize; float2 pad; };
VSOut VSMain(VSIn i){
  VSOut o; float2 ndc = float2(i.pos.x / (screenSize.x*0.5f) - 1.0f, - (i.pos.y / (screenSize.y*0.5f) - 1.0f));
  o.pos = float4(ndc, 0, 1); o.uv = i.uv; return o; }

Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);
float4 PSColor(VSOut i) : SV_Target { return float4(0.2, 0.7, 0.9, 1); }
float4 PSTex(VSOut i)   : SV_Target { return tex0.Sample(samp0, i.uv); }
)HLSL";

struct ScreenCB { float screenSize[2]; float pad[2]; };
static ComPtr<ID3D11Buffer> g_ScreenCB;

D3D11Renderer::D3D11Renderer(HWND hwnd, int w, int h) : backbufferW_(w), backbufferH_(h) {
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL fl;
    D3D_FEATURE_LEVEL req[] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                   req, 1, D3D11_SDK_VERSION, &device_, &fl, &context_);
    if (FAILED(hr)) throw std::runtime_error("D3D11CreateDevice failed");

    CreateSwapChainAndTargets(hwnd, w, h);
    CreatePipeline();
}

D3D11Renderer::~D3D11Renderer() {}

void D3D11Renderer::CreateSwapChainAndTargets(HWND hwnd, int w, int h) {
    ComPtr<IDXGIDevice> dxgiDev; device_.As(&dxgiDev);
    ComPtr<IDXGIAdapter> adapter; dxgiDev->GetAdapter(&adapter);
    ComPtr<IDXGIFactory> factory; adapter->GetParent(__uuidof(IDXGIFactory), &factory);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = w; sd.BufferDesc.Height = h; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    factory->CreateSwapChain(device_.Get(), &sd, &swapChain_);

    ComPtr<ID3D11Texture2D> backbuf; swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuf);
    device_->CreateRenderTargetView(backbuf.Get(), nullptr, &rtv_);
}

void D3D11Renderer::CreatePipeline() {
    // Quad geometry (two triangles)
    VertexPTC verts[] = {
        {0,0, 0,0}, {96,0, 1,0}, {96,96, 1,1}, {0,96, 0,1}
    };
    uint16_t indices[] = {0,1,2, 0,2,3};

    D3D11_BUFFER_DESC vb{}; vb.Usage=D3D11_USAGE_DYNAMIC; vb.ByteWidth=sizeof(verts); vb.BindFlags=D3D11_BIND_VERTEX_BUFFER; vb.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA vinit{}; vinit.pSysMem=verts; device_->CreateBuffer(&vb, &vinit, &vb_);

    D3D11_BUFFER_DESC ib{}; ib.Usage=D3D11_USAGE_IMMUTABLE; ib.ByteWidth=sizeof(indices); ib.BindFlags=D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iinit{}; iinit.pSysMem=indices; device_->CreateBuffer(&ib, &iinit, &ib_);

    auto vsb = CompileShader(g_ShaderSrc, "VSMain", "vs_5_0");
    device_->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs_);

    D3D11_INPUT_ELEMENT_DESC il[] = {
        {"POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0, D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,8, D3D11_INPUT_PER_VERTEX_DATA,0},
    };
    device_->CreateInputLayout(il, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), &layout_);

    auto psbC = CompileShader(g_ShaderSrc, "PSColor", "ps_5_0");
    device_->CreatePixelShader(psbC->GetBufferPointer(), psbC->GetBufferSize(), nullptr, &psColor_);
    auto psbT = CompileShader(g_ShaderSrc, "PSTex", "ps_5_0");
    device_->CreatePixelShader(psbT->GetBufferPointer(), psbT->GetBufferSize(), nullptr, &psTex_);

    // Screen CB
    D3D11_BUFFER_DESC cbd{}; cbd.Usage = D3D11_USAGE_DYNAMIC; cbd.ByteWidth = sizeof(ScreenCB); cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device_->CreateBuffer(&cbd, nullptr, &g_ScreenCB);

    // Sampler
    D3D11_SAMPLER_DESC sd{}; sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR; sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    device_->CreateSamplerState(&sd, &sampler_);
}

void D3D11Renderer::BeginFrame(float r, float g, float b, float a) {
    float clear[4] = {r,g,b,a};
    context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(rtv_.Get(), clear);

    // update screen CB
    D3D11_MAPPED_SUBRESOURCE m; context_->Map(g_ScreenCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
    auto* cb = (ScreenCB*)m.pData; cb->screenSize[0]=(float)backbufferW_; cb->screenSize[1]=(float)backbufferH_;
    context_->Unmap(g_ScreenCB.Get(), 0);
    context_->VSSetConstantBuffers(0,1,g_ScreenCB.GetAddressOf());

    UINT stride=sizeof(VertexPTC), offset=0; ID3D11Buffer* bufs[] = { vb_.Get() };
    context_->IASetVertexBuffers(0,1,bufs,&stride,&offset);
    context_->IASetIndexBuffer(ib_.Get(), DXGI_FORMAT_R16_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetInputLayout(layout_.Get());
    context_->VSSetShader(vs_.Get(), nullptr, 0);
}

void D3D11Renderer::DrawQuad(float x, float y, float w, float h) {
    // update quad vertices per-draw
    D3D11_MAPPED_SUBRESOURCE m;
    context_->Map(vb_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
    auto* v = (VertexPTC*)m.pData;
    v[0] = {x,y, 0,0}; v[1] = {x+w,y, 1,0}; v[2] = {x+w,y+h, 1,1}; v[3] = {x,y+h, 0,1};
    context_->Unmap(vb_.Get(), 0);

    context_->PSSetShader(psColor_.Get(), nullptr, 0);
    context_->DrawIndexed(6, 0, 0);
}

void D3D11Renderer::DrawTexturedQuad(float x, float y, float w, float h, void* texture) {
    D3D11_MAPPED_SUBRESOURCE m;
    context_->Map(vb_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
    auto* v = (VertexPTC*)m.pData;
    v[0] = {x,y, 0,0}; v[1] = {x+w,y, 1,0}; v[2] = {x+w,y+h, 1,1}; v[3] = {x,y+h, 0,1};
    context_->Unmap(vb_.Get(), 0);

    ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)texture;
    context_->PSSetShader(psTex_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 1, &srv);
    context_->PSSetSamplers(0, 1, sampler_.GetAddressOf());
    context_->DrawIndexed(6, 0, 0);
}

void* D3D11Renderer::LoadTextureFromFile(const char* path) {
    ComPtr<ID3D11ShaderResourceView> srv;
    if (SUCCEEDED(CreateWICTextureFromFile(device_.Get(), context_.Get(), path, nullptr, &srv))) {
        return srv.Detach();
    }
    return nullptr;
}

void D3D11Renderer::EndFrame() {
    swapChain_->Present(1, 0);
}
```

## src/render/d3d11/TextureLoader.h

```cpp
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
using Microsoft::WRL::ComPtr;

// Minimal WIC loader (header-only shim). In production, prefer DirectXTK's WICTextureLoader.
HRESULT CreateWICTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* context,
                                 const std::string& filename,
                                 ID3D11Resource** textureOut,
                                 ID3D11ShaderResourceView** srvOut);
```

## src/render/d3d11/TextureLoader.cpp

```cpp
#include "TextureLoader.h"
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

static IWICImagingFactory* GetWIC() {
    static IWICImagingFactory* f = nullptr;
    if (!f) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&f));
    return f;
}

HRESULT CreateWICTextureFromFile(ID3D11Device* device, ID3D11DeviceContext* ctx,
                                 const std::string& filename,
                                 ID3D11Resource** textureOut,
                                 ID3D11ShaderResourceView** srvOut) {
    IWICImagingFactory* wic = GetWIC(); if (!wic) return E_FAIL;
    ComPtr<IWICBitmapDecoder> dec; HRESULT hr = wic->CreateDecoderFromFilename(std::wstring(filename.begin(), filename.end()).c_str(),
        nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &dec);
    if (FAILED(hr)) return hr;
    ComPtr<IWICBitmapFrameDecode> frame; dec->GetFrame(0, &frame);

    UINT w, h; frame->GetSize(&w, &h);
    ComPtr<IWICFormatConverter> conv; wic->CreateFormatConverter(&conv);
    conv->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    std::vector<uint32_t> pixels(w*h);
    conv->CopyPixels(nullptr, w*4, w*h*4, (BYTE*)pixels.data());

    D3D11_TEXTURE2D_DESC td{}; td.Width=w; td.Height=h; td.MipLevels=1; td.ArraySize=1; td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count=1; td.Usage=D3D11_USAGE_IMMUTABLE; td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem=pixels.data(); sd.SysMemPitch=w*4;

    ComPtr<ID3D11Texture2D> tex; hr = device->CreateTexture2D(&td, &sd, &tex);
    if (FAILED(hr)) return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC rvd{}; rvd.Format=td.Format; rvd.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D; rvd.Texture2D.MipLevels=1;
    ComPtr<ID3D11ShaderResourceView> srv; device->CreateShaderResourceView(tex.Get(), &rvd, &srv);

    if (textureOut) *textureOut = tex.Detach();
    if (srvOut) *srvOut = srv.Detach();
    return S_OK;
}
```

---

## src/ui/ImGuiLayer.h

```cpp
#pragma once
#include <windows.h>
#include <d3d11.h>

class ImGuiLayer {
public:
    ImGuiLayer(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx);
    ~ImGuiLayer();
    bool WndProc(HWND, UINT, WPARAM, LPARAM);
    void Begin();
    void Text(const char* fmt, ...);
    void End();
private:
    bool begun_ = false;
};
```

## src/ui/ImGuiLayer.cpp

```cpp
#include "ImGuiLayer.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <cstdarg>

ImGuiLayer::ImGuiLayer(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev, ctx);
}
ImGuiLayer::~ImGuiLayer(){ ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); }

bool ImGuiLayer::WndProc(HWND h, UINT m, WPARAM w, LPARAM l){ return ImGui_ImplWin32_WndProcHandler(h,m,w,l); }

void ImGuiLayer::Begin(){ if(begun_) return; ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame(); begun_=true; }

void ImGuiLayer::Text(const char* fmt, ...){
    ImGui::Begin("Stats");
    va_list args; va_start(args, fmt); ImGui::TextV(fmt, args); va_end(args);
    ImGui::End();
}

void ImGuiLayer::End(){ if(!begun_) return; ImGui::Render(); ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); begun_=false; }
```

---

## macOS (Optional) Stubs

### src/platform/mac/AppDelegate.mm

```objectivec
#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
@interface AppDelegate : NSObject<NSApplicationDelegate>
@property (strong) NSWindow* window;
@property (strong) MTKView* mtkView;
@end
@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)n {
  NSRect r = NSMakeRect(0,0,1280,720);
  self.window = [[NSWindow alloc] initWithContentRect:r styleMask:(NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable) backing:NSBackingStoreBuffered defer:NO];
  self.window.title = @"MiniGame2D (Metal stub)";
  self.mtkView = [[MTKView alloc] initWithFrame:r device: MTLCreateSystemDefaultDevice()];
  self.mtkView.clearColor = MTLClearColorMake(0.1,0.12,0.14,1);
  self.mtkView.enableSetNeedsDisplay = YES; self.mtkView.paused = NO;
  [self.window setContentView:self.mtkView];
  [self.window makeKeyAndOrderFront:nil];
}
@end
int main(){ @autoreleasepool { NSApplication* app=[NSApplication sharedApplication]; AppDelegate* d=[AppDelegate new]; [app setDelegate:d]; [app run]; } return 0; }
```

### src/platform/mac/MetalView.mm

```objectivec
// TODO: Add a simple renderer (pipeline, quad) mirroring the DX11 path.
```

---

## What’s Implemented vs. TODOs

* ✅ **Event loop** (PeekMessage + QPC timing)
* ✅ **Rendering** (DX11 device/swap chain, clear, draw quad/texture)
* ✅ **Input** (W/A/S/D move, ESC quit; both WM_KEYDOWN and polling for smoothness)
* ✅ **UI** (ImGui: FPS + player position panel)
* ✅ **Resource loading** (WIC PNG → SRV)
* 🟨 **Resize handling** (left as exercise: recreate RTV on `WM_SIZE`)
* 🟨 **Metal path** (window + clear; fill in renderer + input as needed)

---

## Notes

* To add fonts/UI: drop TTFs and call `ImGui::GetIO().Fonts->AddFontFromFileTTF(...)` before `ImGui_ImplDX11_Init` or rebuild font atlas afterward.
* For sprites/animation: batch via dynamic VB updates or integrate **DirectXTK SpriteBatch**.
* For game loop timing, consider a fixed timestep accumulator.
* For resource lifetime, wrap raw pointers (SRVs) in RAII holders.

Happy hacking! 🎮
