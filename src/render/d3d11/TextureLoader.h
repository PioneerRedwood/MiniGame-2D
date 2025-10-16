#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <string>

using Microsoft::WRL::ComPtr;

HRESULT CreateWICTextureFromFile(ID3D11Device* device,
                                 ID3D11DeviceContext* context,
                                 const std::string& filename,
                                 ID3D11Resource** textureOut,
                                 ID3D11ShaderResourceView** srvOut);
