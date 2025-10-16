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
    float speed = 220.0f; // pixel per second
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
    void SetPlayerTexture(void* texture);

private:
    AppConfig cfg_;
    GameState state_;
    IRenderer2D* renderer_ = nullptr;
    void* playerTex_ = nullptr;
    bool hasTexture_ = false;
};
