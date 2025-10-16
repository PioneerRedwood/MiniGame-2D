#include "TextureLoader.h"

#include <wincodec.h>

#include <vector>

#pragma comment(lib, "windowscodecs.lib")

static IWICImagingFactory* GetWICFactory() {
    static IWICImagingFactory* factory = nullptr;
    if (!factory) {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    }
    return factory;
}

HRESULT CreateWICTextureFromFile(ID3D11Device* device,
                                 ID3D11DeviceContext* context,
                                 const std::string& filename,
                                 ID3D11Resource** textureOut,
                                 ID3D11ShaderResourceView** srvOut) {
    IWICImagingFactory* wic = GetWICFactory();
    if (!wic) {
        return E_FAIL;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    std::wstring wpath(filename.begin(), filename.end());
    HRESULT hr = wic->CreateDecoderFromFilename(wpath.c_str(), nullptr, GENERIC_READ,
                                                WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) {
        return hr;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        return hr;
    }

    UINT width = 0;
    UINT height = 0;
    frame->GetSize(&width, &height);

    ComPtr<IWICFormatConverter> converter;
    hr = wic->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        return hr;
    }

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
                               nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        return hr;
    }

    std::vector<uint32_t> pixels(width * height);
    hr = converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size() * sizeof(uint32_t)),
                               reinterpret_cast<BYTE*>(pixels.data()));
    if (FAILED(hr)) {
        return hr;
    }

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData{};
    subData.pSysMem = pixels.data();
    subData.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> texture;
    hr = device->CreateTexture2D(&textureDesc, &subData, &texture);
    if (FAILED(hr)) {
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ComPtr<ID3D11ShaderResourceView> srv;
    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
    if (FAILED(hr)) {
        return hr;
    }

    if (textureOut) {
        *textureOut = texture.Detach();
    }
    if (srvOut) {
        *srvOut = srv.Detach();
    }
    return S_OK;
}
