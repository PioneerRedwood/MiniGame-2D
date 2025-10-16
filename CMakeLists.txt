cmake_minimum_required(VERSION 3.22)
project(MiniGame2D CXX)

option(USE_METAL "Build macOS Metal stub" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/imgui EXCLUDE_FROM_ALL) # if you export a tiny CMakeLists there; otherwise add files directly

# Common Sources
set(SRC_CORE
    src/core/App.cpp
    src/core/App.h
    src/ui/ImGuiLayer.cpp
    src/ui/ImGuiLayer.h
)

# Windows / DirectX11
if(WIN32)
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
end()

# macOS / Metal (optional minial app)

