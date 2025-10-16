#include "App.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

App::App(const AppConfig& cfg) : cfg_(cfg) {}

void App::Update(float dt) {
    state_.playerX = std::clamp(state_.playerX, 0.0f, (float)cfg_.width - 64.0f);
    state_.playerY = std::clamp(state_.playerY, 0.0f, (float)cfg_.height - 64.0f);
}

void App::Render() {
    if(!renderer_) return;
    renderer_->BeginFrame(0.07f, 0.08f, 0.1f, 1.0f);
    const float w = 96.0f;
    const float h = 96.0f;
    if(hasTexture_ && playerTex_) {
        renderer_->DrawTexturedQuad(state_.playerX, state_.playerY, w, h, playerTex_);
    } else {
        renderer_->DrawQuad(state_.playerX, state_.playerY, w, h);
    }
    renderer_->EndFrame();
}

void App::OnKey(bool down, int key) {
    const float s = state_.speed / 60.0f; // per-frame approx; real dt applied in Main
    if(!down) return;
    switch (key) {
        case 'W': state_.playerY -= s; break;
        case 'S': state_.playerY += s; break;
        case 'A': state_.playerX -= s; break;
        case 'D': state_.playerX += s; break;
        default: break;
    }
}

void App::SetPlayerTexture(void* texture) {
    playerTex_ = texture;
    hasTexture_ = texture != nullptr;
}
