/*
 * rendering/Renderer.hpp
 * Encapsula SDL2 window/renderer y ofrece primitivas de rasterización software:
 * wireframe, relleno con z-buffer, texturizado, fondo y líneas de velocidad.
 *
 * NOTA: el ciclo de vida de SDL (SDL_Init/SDL_Quit, IMG_Init/IMG_Quit) es
 * responsabilidad del llamador (main). Renderer solo gestiona la ventana y el
 * renderer SDL internos.
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

    /* Crea la ventana y el renderer SDL. Debe llamarse después de SDL_Init. */
    bool init();

    /* Limpia el framebuffer y el z-buffer; dibuja fondo HD o estrellas según hdMode. */
    void clear(bool hdMode = true);

    /* Vuelca el framebuffer a pantalla. */
    void present();

    void setWindowTitle(const std::string& title);

    /* Dibuja las aristas de la malla en espacio de cámara con color uniforme o por vértice. */
    void drawMesh(const Mesh& mesh, const Mat4& transform,
                  const Mat4& view, SDL_Color color);

    /* Rasteriza los triángulos de la malla con z-buffer; usa textura o color plano por tri. */
    void drawFilledMesh(const Mesh& mesh, const Mat4& transform,
                        const Mat4& view, SDL_Surface* texture);

    int width()  const { return W; }
    int height() const { return H; }

    /* Expone el SDL_Renderer subyacente para dibujo directo (UI, texto, rects). */
    SDL_Renderer* sdlRenderer() const { return sdlRend; }

    /* Actualiza posición y vida de las líneas de velocidad según la intensidad [0,1]. */
    void updateSpeedLines(float dt, float intensity);

    /* Dibuja las líneas de velocidad activas sobre el framebuffer. */
    void drawSpeedLines(float intensity);

private:
    struct SpeedStreak {
        float x, y;
        float vx, vy;
        float life;
        float len;
    };

    struct Star { float x, y; Uint8 brightness; };

    int           W, H;
    SDL_Window*   window            = nullptr;
    SDL_Renderer* sdlRend           = nullptr;
    SDL_Texture*  backgroundTexture = nullptr;

    std::vector<float>       zbuf;
    std::vector<SpeedStreak> streaks;
    std::vector<Star>        stars;

    static constexpr float FOV  = 600.f;
    static constexpr float NEAR = 0.5f;

    bool projectPoint(Vec3 viewPos, float& sx, float& sy) const;
    bool clipLine(Vec3& a, Vec3& b) const;

    void drawTriangle(Vec3 p0, Vec3 p1, Vec3 p2,
                      Vec3 uv0, Vec3 uv1, Vec3 uv2,
                      SDL_Surface* texture,
                      SDL_Color flatColor = {255, 255, 255, 255});

    Uint32 sampleTexture(SDL_Surface* tex, float u, float v) const;

    void generateStars(int count = 300);
    void drawStars();
};