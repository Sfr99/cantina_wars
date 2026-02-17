#pragma once
#include <SDL2/SDL.h>
#include <string>
#include "Mesh.hpp"
#include "Math3D.hpp"
#include <vector>

class Renderer {
public:
    Renderer(int w, int h);
    ~Renderer();

    bool init();
    void clear();
    void present();
    void setWindowTitle(const std::string& title);

    void drawMesh(const Mesh& mesh, const Mat4& transform,
                  const Mat4& view, SDL_Color color);
    
    void drawFilledMesh(const Mesh& mesh, const Mat4& transform,
                        const Mat4& view, SDL_Surface* texture);

    int width()  const { return W; }
    int height() const { return H; }
    void updateSpeedLines(float dt, float intensity);
    void drawSpeedLines(float intensity);
private:
    struct SpeedStreak {
        float x, y;
        float vx, vy;
        float life;
        float len;
    };

std::vector<SpeedStreak> streaks;
    int W, H;
    SDL_Window*   window  = nullptr;
    SDL_Renderer* sdlRend = nullptr;
    SDL_Texture* backgroundTexture = nullptr;

    static constexpr float FOV  = 600.f;
    static constexpr float NEAR = 0.5f;

    bool projectPoint(Vec3 viewPos, float& sx, float& sy) const;
    bool clipLine(Vec3& a, Vec3& b) const;
    
    void drawTriangle(Vec3 p0, Vec3 p1, Vec3 p2,
                     Vec3 uv0, Vec3 uv1, Vec3 uv2,
                     SDL_Surface* texture,
                     SDL_Color flatColor = {255,255,255,255});
    
    std::vector<float> zbuf;  // buffer de profundidad para relleno
    Uint32 sampleTexture(SDL_Surface* tex, float u, float v) const;
};