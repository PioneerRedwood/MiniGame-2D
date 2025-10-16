#pragma once
#include <windows.h>
#include <d3d11.h>

class ImGuiLayer {
public:
    ImGuiLayer(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx);
    ~ImGuiLayer();

    bool WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void Begin();
    void Text(const char* fmt, ...);
    void End();

private:
    bool begun_ = false;
};
