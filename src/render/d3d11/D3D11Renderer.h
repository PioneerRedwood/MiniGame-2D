#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include "../../core/App.h"

using Microsoft::WRL::ComPtr;

struct VertexPTC {
    float x, y;
    float u, v;
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
    D3D11_VIEWPORT viewport_{};

    int backBufferW_ = 0;
    int backBufferH_ = 0;
};
