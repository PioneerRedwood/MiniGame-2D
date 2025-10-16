#pragma once
#include <windows.h>

inline bool IsKeyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }