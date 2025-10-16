#include "D3D11Renderer.h"
#include "TextureLoader.h"

#include <d3dcompiler.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

namespace {

void ThrowIfFailed(HRESULT hr, const char* message) {
    if (FAILED(hr)) {
        throw std::runtime_error(message);
    }
}

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const char* src,
                                               const char* entry,
                                               const char* profile) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    HRESULT hr = D3DCompile(src,
                            std::strlen(src),
                            nullptr,
                            nullptr,
                            nullptr,
                            entry,
                            profile,
                            flags,
                            0,
                            &blob,
                            &errors);
    if (FAILED(hr)) {
        if (errors) {
            throw std::runtime_error(
                static_cast<const char*>(errors->GetBufferPointer()));
        }
        ThrowIfFailed(hr, "Shader compile failed");
    }

    return blob;
}

const char* g_ShaderSrc = R"HLSL(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
cbuffer ScreenCB : register(b0) { float2 screenSize; float2 pad; };
VSOut VSMain(VSIn i) {
    VSOut o;
    float2 ndc = float2(i.pos.x / (screenSize.x * 0.5f) - 1.0f,
                        -(i.pos.y / (screenSize.y * 0.5f) - 1.0f));
    o.pos = float4(ndc, 0, 1); o.uv = i.uv;
    return o;
}

Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);
float4 PSColor(VSOut i) : SV_Target { return float4(0.2, 0.7, 0.9, 1); }
float4 PSTex(VSOut i) : SV_Target { return tex0.Sample(samp0, i.uv); }
)HLSL";

struct ScreenCB {
    float screenSize[2];
    float pad[2];
};

Microsoft::WRL::ComPtr<ID3D11Buffer> g_ScreenCB;

} // namespace

D3D11Renderer::D3D11Renderer(HWND hwnd, int w, int h)
    : backBufferW_(w), backBufferH_(h) {
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL obtainedLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDevice(nullptr,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   nullptr,
                                   flags,
                                   &featureLevel,
                                   1,
                                   D3D11_SDK_VERSION,
                                   device_.ReleaseAndGetAddressOf(),
                                   &obtainedLevel,
                                   context_.ReleaseAndGetAddressOf());
    ThrowIfFailed(hr, "D3D11CreateDevice failed");

    CreateSwapChainAndTargets(hwnd, w, h);
    CreatePipeline();
}

D3D11Renderer::~D3D11Renderer() = default;

void D3D11Renderer::CreateSwapChainAndTargets(HWND hwnd, int w, int h) {
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(device_.As(&dxgiDevice), "Failed to query IDXGIDevice");

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    ThrowIfFailed(dxgiDevice->GetAdapter(adapter.GetAddressOf()),
                  "Failed to get IDXGIAdapter");

    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    ThrowIfFailed(adapter->GetParent(__uuidof(IDXGIFactory),
                                     reinterpret_cast<void**>(factory.GetAddressOf())),
                  "Failed to get IDXGIFactory");

    DXGI_SWAP_CHAIN_DESC swapDesc{};
    swapDesc.BufferCount = 2;
    swapDesc.BufferDesc.Width = static_cast<UINT>(w);
    swapDesc.BufferDesc.Height = static_cast<UINT>(h);
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = hwnd;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.Windowed = TRUE;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ThrowIfFailed(factory->CreateSwapChain(device_.Get(),
                                           &swapDesc,
                                           swapChain_.ReleaseAndGetAddressOf()),
                  "CreateSwapChain failed");

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    ThrowIfFailed(swapChain_->GetBuffer(0,
                                        IID_PPV_ARGS(&backBuffer)),
                  "Failed to obtain swap chain back buffer");

    ThrowIfFailed(device_->CreateRenderTargetView(backBuffer.Get(),
                                                  nullptr,
                                                  rtv_.ReleaseAndGetAddressOf()),
                  "CreateRenderTargetView failed");

    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.Width = static_cast<float>(w);
    viewport_.Height = static_cast<float>(h);
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport_);
}

void D3D11Renderer::CreatePipeline() {
    VertexPTC verts[] = {
        {0, 0, 0, 0},
        {96, 0, 1, 0},
        {96, 96, 1, 1},
        {0, 96, 0, 1},
    };

    uint16_t indices[] = {0, 1, 2, 0, 2, 3};

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = verts;
    ThrowIfFailed(device_->CreateBuffer(&vbDesc,
                                        &vbData,
                                        vb_.ReleaseAndGetAddressOf()),
                  "CreateBuffer (vertex) failed");

    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices;
    ThrowIfFailed(device_->CreateBuffer(&ibDesc,
                                        &ibData,
                                        ib_.ReleaseAndGetAddressOf()),
                  "CreateBuffer (index) failed");

    auto vsBlob = CompileShader(g_ShaderSrc, "VSMain", "vs_5_0");
    ThrowIfFailed(device_->CreateVertexShader(vsBlob->GetBufferPointer(),
                                              vsBlob->GetBufferSize(),
                                              nullptr,
                                              vs_.ReleaseAndGetAddressOf()),
                  "CreateVertexShader failed");

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    const UINT inputCount = static_cast<UINT>(sizeof(layoutDesc) / sizeof(layoutDesc[0]));
    ThrowIfFailed(device_->CreateInputLayout(layoutDesc,
                                             inputCount,
                                             vsBlob->GetBufferPointer(),
                                             vsBlob->GetBufferSize(),
                                             layout_.ReleaseAndGetAddressOf()),
                  "CreateInputLayout failed");

    auto psColorBlob = CompileShader(g_ShaderSrc, "PSColor", "ps_5_0");
    ThrowIfFailed(device_->CreatePixelShader(psColorBlob->GetBufferPointer(),
                                             psColorBlob->GetBufferSize(),
                                             nullptr,
                                             psColor_.ReleaseAndGetAddressOf()),
                  "CreatePixelShader (color) failed");

    auto psTexBlob = CompileShader(g_ShaderSrc, "PSTex", "ps_5_0");
    ThrowIfFailed(device_->CreatePixelShader(psTexBlob->GetBufferPointer(),
                                             psTexBlob->GetBufferSize(),
                                             nullptr,
                                             psTex_.ReleaseAndGetAddressOf()),
                  "CreatePixelShader (texture) failed");

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(ScreenCB);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ThrowIfFailed(device_->CreateBuffer(&cbDesc,
                                        nullptr,
                                        g_ScreenCB.ReleaseAndGetAddressOf()),
                  "CreateBuffer (ScreenCB) failed");

    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ThrowIfFailed(device_->CreateSamplerState(&samplerDesc,
                                              sampler_.ReleaseAndGetAddressOf()),
                  "CreateSamplerState failed");
}

void D3D11Renderer::BeginFrame(float r, float g, float b, float a) {
    float clear[4] = {r, g, b, a};
    context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(rtv_.Get(), clear);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ThrowIfFailed(context_->Map(g_ScreenCB.Get(),
                                0,
                                D3D11_MAP_WRITE_DISCARD,
                                0,
                                &mapped),
                  "Map ScreenCB failed");
    auto* cb = static_cast<ScreenCB*>(mapped.pData);
    cb->screenSize[0] = static_cast<float>(backBufferW_);
    cb->screenSize[1] = static_cast<float>(backBufferH_);
    context_->Unmap(g_ScreenCB.Get(), 0);

    ID3D11Buffer* vb = vb_.Get();
    UINT stride = sizeof(VertexPTC);
    UINT offset = 0;
    context_->RSSetViewports(1, &viewport_);
    context_->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context_->IASetIndexBuffer(ib_.Get(), DXGI_FORMAT_R16_UINT, 0);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetInputLayout(layout_.Get());
    context_->VSSetConstantBuffers(0, 1, g_ScreenCB.GetAddressOf());
    context_->VSSetShader(vs_.Get(), nullptr, 0);
}

void D3D11Renderer::DrawQuad(float x, float y, float w, float h) {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    ThrowIfFailed(context_->Map(vb_.Get(),
                                0,
                                D3D11_MAP_WRITE_DISCARD,
                                0,
                                &mapped),
                  "Map VB failed");
    auto* verts = static_cast<VertexPTC*>(mapped.pData);
    verts[0] = {x, y, 0, 0};
    verts[1] = {x + w, y, 1, 0};
    verts[2] = {x + w, y + h, 1, 1};
    verts[3] = {x, y + h, 0, 1};
    context_->Unmap(vb_.Get(), 0);

    context_->PSSetShader(psColor_.Get(), nullptr, 0);
    context_->DrawIndexed(6, 0, 0);
}

void D3D11Renderer::DrawTexturedQuad(float x, float y, float w, float h, void* texture) {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    ThrowIfFailed(context_->Map(vb_.Get(),
                                0,
                                D3D11_MAP_WRITE_DISCARD,
                                0,
                                &mapped),
                  "Map VB failed");
    auto* verts = static_cast<VertexPTC*>(mapped.pData);
    verts[0] = {x, y, 0, 0};
    verts[1] = {x + w, y, 1, 0};
    verts[2] = {x + w, y + h, 1, 1};
    verts[3] = {x, y + h, 0, 1};
    context_->Unmap(vb_.Get(), 0);

    ID3D11ShaderResourceView* srv = static_cast<ID3D11ShaderResourceView*>(texture);
    context_->PSSetShader(psTex_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 1, &srv);
    context_->PSSetSamplers(0, 1, sampler_.GetAddressOf());
    context_->DrawIndexed(6, 0, 0);
}

void* D3D11Renderer::LoadTextureFromFile(const char* path) {
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = CreateWICTextureFromFile(device_.Get(),
                                          context_.Get(),
                                          path,
                                          nullptr,
                                          srv.GetAddressOf());
    if (FAILED(hr)) {
        return nullptr;
    }
    return srv.Detach();
}

void D3D11Renderer::EndFrame() {
    swapChain_->Present(1, 0);
}
