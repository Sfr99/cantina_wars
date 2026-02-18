/*
 * rendering/Renderer.hpp
 * Encapsula SDL2 window/renderer y ofrece primitivas de rasterización software.
 *
 * ARQUITECTURA DEL PIXEL BUFFER:
 *   drawFilledMesh → drawTriangle escribe en m_pixelBuf (memoria CPU, sin llamadas SDL)
 *   flushPixelBuffer() → SDL_UpdateTexture + SDL_RenderCopy  (UNA sola llamada GPU)
 *   drawMesh / drawSpeedLines / HUD → SDL_Render* normales, encima del pixel buffer
 *
 * Orden de llamada en Game::render():
 *   clear() → drawFilledMesh×N → flushPixelBuffer() → drawMesh×N → drawSpeedLines → HUD → present()
 */
#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "../core/Mesh.hpp"
#include "../core/Math3D.hpp"

class Renderer {
public:
    Renderer(int w, int h);
    ~Renderer();

    /* Crea la ventana, el renderer SDL y los buffers. Idempotente. */
    bool init();

    /* Limpia el framebuffer, el pixel buffer y el z-buffer. */
    void clear(bool hdMode = true);

    /* Sube el pixel buffer a GPU y lo vuelca sobre el renderer. Llamar después
       de todos los drawFilledMesh y antes de drawMesh / HUD. */
    void flushPixelBuffer();

    /* Vuelca el framebuffer a pantalla. */
    void present();

    void setWindowTitle(const std::string& title);

    /* Dibuja aristas de la malla; usa color por vértice si está disponible. */
    void drawMesh(const Mesh& mesh, const Mat4& transform,
                  const Mat4& view, SDL_Color color);

    /* Rasteriza triángulos al pixel buffer con z-test y backface culling. */
    void drawFilledMesh(const Mesh& mesh, const Mat4& transform,
                        const Mat4& view, SDL_Surface* texture);

    int width()  const { return W; }
    int height() const { return H; }

    /* Expone el SDL_Renderer para dibujo directo (UI, texto, rects). */
    SDL_Renderer* sdlRenderer() const { return sdlRend; }

    void updateSpeedLines(float dt, float intensity);
    void drawSpeedLines(float intensity);

private:
    struct SpeedStreak { float x, y, vx, vy, life, len; };
    struct Star        { float x, y; Uint8 brightness; };

    int           W, H;
    SDL_Window*   window            = nullptr;
    SDL_Renderer* sdlRend           = nullptr;
    SDL_Texture*  backgroundTexture = nullptr;

    // Pixel buffer para rasterización software
    SDL_Surface* m_pixelBuf = nullptr;   // escritura CPU píxel a píxel
    SDL_Texture* m_pixelTex = nullptr;   // textura streaming para subir a GPU

    std::vector<float>       zbuf;
    std::vector<SpeedStreak> streaks;
    std::vector<Star>        stars;

    static constexpr float FOV  = 600.f;
    static constexpr float NEAR = 0.5f;

    bool   projectPoint(Vec3 viewPos, float& sx, float& sy) const;
    bool   clipLine(Vec3& a, Vec3& b) const;
    Uint32 sampleTexture(SDL_Surface* tex, float u, float v) const;

    /* Escribe un triángulo directamente en m_pixelBuf con interpolación baricéntrica. */
    void drawTriangle(Vec3 p0, Vec3 p1, Vec3 p2,
                      Vec3 uv0, Vec3 uv1, Vec3 uv2,
                      SDL_Surface* texture,
                      SDL_Color flatColor = {255, 255, 255, 255});

    void generateStars(int count = 300);
    void drawStars();
};