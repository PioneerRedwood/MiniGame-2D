#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <combaseapi.h>

#include <string>
#include <stdexcept>

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
    if (g_ImGui && g_ImGui->WndProc(hWnd, msg, wParam, lParam)) {
        return TRUE;
    }

    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            if (g_App) {
                g_App->OnKey(true, static_cast<int>(wParam));
            }
            return 0;
        case WM_KEYUP:
            if (g_App) {
                g_App->OnKey(false, static_cast<int>(wParam));
            }
            return 0;
        default:
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static std::wstring ToWide(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    HRESULT coHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    const std::wstring className = L"MiniGame2DWnd";
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className.c_str();

    if (!RegisterClassEx(&wc)) {
        return -1;
    }

    AppConfig cfg;
    cfg.title = "MiniGame2D (DX11)";

    RECT rc{0, 0, cfg.width, cfg.height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    std::wstring windowTitle = ToWide(cfg.title);
    HWND hwnd = CreateWindow(wc.lpszClassName,
                             windowTitle.c_str(),
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             rc.right - rc.left,
                             rc.bottom - rc.top,
                             nullptr,
                             nullptr,
                             wc.hInstance,
                             nullptr);
    if (!hwnd) {
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        if (SUCCEEDED(coHr)) {
            CoUninitialize();
        }
        return -1;
    }

    try {
        D3D11Renderer renderer(hwnd, cfg.width, cfg.height);
        g_Renderer = &renderer;

        App app(cfg);
        g_App = &app;
        app.SetRenderer(&renderer);

        ImGuiLayer imgui(hwnd, renderer.GetDevice(), renderer.GetDeviceContext());
        g_ImGui = &imgui;

        void* tex = renderer.LoadTextureFromFile("assets/player.png");
        if (tex) {
            app.SetPlayerTexture(tex);
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        LARGE_INTEGER freq{};
        LARGE_INTEGER prev{};
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&prev);

        MSG msg{};
        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            LARGE_INTEGER now{};
            QueryPerformanceCounter(&now);
            float dt = static_cast<float>(now.QuadPart - prev.QuadPart) /
                       static_cast<float>(freq.QuadPart);
            prev = now;

            if (IsKeyDown('W')) app.OnKey(true, 'W');
            if (IsKeyDown('A')) app.OnKey(true, 'A');
            if (IsKeyDown('S')) app.OnKey(true, 'S');
            if (IsKeyDown('D')) app.OnKey(true, 'D');

            app.Update(dt);

            imgui.Begin();
            const float fps = (dt > 0.0001f) ? (1.0f / dt) : 0.0f;
            imgui.Text("FPS: %.1f", fps);
            imgui.Text("Player: (%.1f, %.1f)", app.State().playerX, app.State().playerY);
            imgui.End();

            app.Render();
        }

        g_App = nullptr;
        g_Renderer = nullptr;
        g_ImGui = nullptr;
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "MiniGame2D - Error", MB_ICONERROR | MB_OK);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        if (SUCCEEDED(coHr)) {
            CoUninitialize();
        }
        return -1;
    }

    UnregisterClass(wc.lpszClassName, wc.hInstance);
    if (SUCCEEDED(coHr)) {
        CoUninitialize();
    }
    return 0;
}
