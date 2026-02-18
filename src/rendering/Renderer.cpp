#include "Renderer.hpp"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

Renderer::Renderer(int w, int h) : W(w), H(h) {}

/* Convierte una superficie a ARGB8888 in-place; libera la original si hace falta. */
static SDL_Surface* convertToARGB8888(SDL_Surface* s) {
    if (!s) return nullptr;
    if (s->format && s->format->format == SDL_PIXELFORMAT_ARGB8888) {
        SDL_SetSurfaceRLE(s, 0);
        return s;
    }
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(s);
    if (conv) SDL_SetSurfaceRLE(conv, 0);
    return conv;
}

Renderer::~Renderer() {
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (sdlRend)           SDL_DestroyRenderer(sdlRend);
    if (window)            SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

/* Crea ventana y renderer SDL, carga el fondo y genera el campo de estrellas. */
bool Renderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    const int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    IMG_Init(imgFlags);

    window = SDL_CreateWindow("Asteroid 3D",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              W, H, SDL_WINDOW_SHOWN);
    if (!window) return false;

    sdlRend = SDL_CreateRenderer(window, -1,
                                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdlRend) return false;

    SDL_SetRenderDrawBlendMode(sdlRend, SDL_BLENDMODE_BLEND);

    SDL_Surface* bg = convertToARGB8888(IMG_Load("../assets/space.png"));
    if (bg) {
        backgroundTexture = SDL_CreateTextureFromSurface(sdlRend, bg);
        SDL_FreeSurface(bg);
    }

    generateStars(300);

    zbuf.assign(W * H, 1e9f);
    return true;
}

/* Rellena el array de estrellas con posiciones y brillos aleatorios. */
void Renderer::generateStars(int count) {
    stars.clear();
    for (int i = 0; i < count; i++) {
        stars.push_back({
            (float)(rand() % W),
            (float)(rand() % H),
            (Uint8)(100 + rand() % 155)
        });
    }
}

/* Renderiza el campo de estrellas como puntos grises sobre fondo negro. */
void Renderer::drawStars() {
    for (const auto& s : stars) {
        SDL_SetRenderDrawColor(sdlRend, s.brightness, s.brightness, s.brightness, 255);
        SDL_RenderDrawPoint(sdlRend, (int)s.x, (int)s.y);
    }
}

/* Limpia color y z-buffer; muestra fondo de textura en modo HD o estrellas en LOW_POLY. */
void Renderer::clear(bool hdMode) {
    SDL_SetRenderDrawColor(sdlRend, 0, 0, 0, 255);
    SDL_RenderClear(sdlRend);

    if (hdMode && backgroundTexture)
        SDL_RenderCopy(sdlRend, backgroundTexture, nullptr, nullptr);
    else
        drawStars();

    std::fill(zbuf.begin(), zbuf.end(), 1e9f);
}

void Renderer::present() {
    SDL_RenderPresent(sdlRend);
}

void Renderer::setWindowTitle(const std::string& title) {
    SDL_SetWindowTitle(window, title.c_str());
}

/* Proyecta vp (espacio cámara) a píxeles de pantalla; devuelve false si está detrás del near. */
bool Renderer::projectPoint(Vec3 vp, float& sx, float& sy) const {
    if (vp.z < NEAR) return false;
    sx =  (vp.x / vp.z) * FOV + W * 0.5f;
    sy = -(vp.y / vp.z) * FOV + H * 0.5f;
    return true;
}

/* Recorta el segmento [a,b] contra el plano z=NEAR; devuelve false si queda completamente detrás. */
bool Renderer::clipLine(Vec3& a, Vec3& b) const {
    if (a.z < NEAR && b.z < NEAR) return false;
    if (a.z < NEAR) {
        float t = (NEAR - a.z) / (b.z - a.z);
        a = a + (b - a) * t;
    } else if (b.z < NEAR) {
        float t = (NEAR - b.z) / (a.z - b.z);
        b = b + (a - b) * t;
    }
    return true;
}

/* Transforma, proyecta y dibuja las aristas de la malla; usa color por vértice si está disponible. */
void Renderer::drawMesh(const Mesh& mesh, const Mat4& transform,
                        const Mat4& view, SDL_Color color) {
    std::vector<Vec3> viewVerts(mesh.verts.size());
    for (size_t i = 0; i < mesh.verts.size(); i++) {
        const Vec3& v = mesh.verts[i];
        Vec4 world = transform.multiply({v.x, v.y, v.z, 1.f});
        Vec4 cam   = view.multiply(world);
        viewVerts[i] = {cam.x, cam.y, cam.z};
    }

    const bool hasVertexColors = !mesh.colors.empty()
                               && mesh.colors.size() == mesh.verts.size();

    for (const auto& edge : mesh.edges) {
        Vec3 a = viewVerts[edge.a];
        Vec3 b = viewVerts[edge.b];
        if (!clipLine(a, b)) continue;

        float sx1, sy1, sx2, sy2;
        if (!projectPoint(a, sx1, sy1)) continue;
        if (!projectPoint(b, sx2, sy2)) continue;

        if (hasVertexColors) {
            SDL_Color c1 = mesh.colors[edge.a];
            SDL_Color c2 = mesh.colors[edge.b];
            SDL_SetRenderDrawColor(sdlRend,
                (c1.r + c2.r) / 2, (c1.g + c2.g) / 2, (c1.b + c2.b) / 2, 255);
        } else {
            SDL_SetRenderDrawColor(sdlRend, color.r, color.g, color.b, color.a);
        }

        SDL_RenderDrawLine(sdlRend, (int)sx1, (int)sy1, (int)sx2, (int)sy2);
    }
}

/* Muestrea la textura ARGB8888 en UV con wrap y flip-V; devuelve blanco si no es ARGB8888. */
Uint32 Renderer::sampleTexture(SDL_Surface* tex, float u, float v) const {
    if (!tex) return 0xFFFFFFFF;
    if (!tex->format || tex->format->format != SDL_PIXELFORMAT_ARGB8888)
        return 0xFFFFFFFF;

    u = u - floorf(u);
    v = 1.0f - (v - floorf(v));

    int x = std::max(0, std::min((int)(u * (tex->w - 1)), tex->w - 1));
    int y = std::max(0, std::min((int)(v * (tex->h - 1)), tex->h - 1));

    if (SDL_MUSTLOCK(tex)) SDL_LockSurface(tex);
    Uint32 color = *((const Uint32*)((const Uint8*)tex->pixels + y * tex->pitch) + x);
    if (SDL_MUSTLOCK(tex)) SDL_UnlockSurface(tex);

    return color;
}

/* Rasteriza un triángulo en pantalla con interpolación baricéntrica; aplica textura o color plano. */
void Renderer::drawTriangle(Vec3 p0, Vec3 p1, Vec3 p2,
                            Vec3 uv0, Vec3 uv1, Vec3 uv2,
                            SDL_Surface* texture, SDL_Color flatColor) {
    int minX = std::max(0,     (int)std::min({p0.x, p1.x, p2.x}));
    int maxX = std::min(W - 1, (int)std::max({p0.x, p1.x, p2.x}));
    int minY = std::max(0,     (int)std::min({p0.y, p1.y, p2.y}));
    int maxY = std::min(H - 1, (int)std::max({p0.y, p1.y, p2.y}));

    float area = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
    if (fabsf(area) < 1e-6f) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = ((p1.x - px) * (p2.y - py) - (p2.x - px) * (p1.y - py)) / area;
            float w1 = ((p2.x - px) * (p0.y - py) - (p0.x - px) * (p2.y - py)) / area;
            float w2 = 1.0f - w0 - w1;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float z = w0 * p0.z + w1 * p1.z + w2 * p2.z;
            if (z < NEAR) continue;

            const int idx = y * W + x;
            if (z >= zbuf[idx]) continue;
            zbuf[idx] = z;

            Uint8 r, g, b;
            if (texture) {
                float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
                float v = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;
                Uint32 texColor = sampleTexture(texture, u, v);
                SDL_GetRGB(texColor, texture->format, &r, &g, &b);
            } else {
                r = flatColor.r;
                g = flatColor.g;
                b = flatColor.b;
            }

            SDL_SetRenderDrawColor(sdlRend, r, g, b, 255);
            SDL_RenderDrawPoint(sdlRend, x, y);
        }
    }
}

/* Proyecta todos los vértices y rasteriza los triángulos con color plano o por vértice. */
void Renderer::drawFilledMesh(const Mesh& mesh, const Mat4& transform,
                              const Mat4& view, SDL_Surface* texture) {
    if (mesh.tris.empty()) return;

    std::vector<Vec3> screenVerts(mesh.verts.size());
    std::vector<Vec3> viewVerts(mesh.verts.size());

    for (size_t i = 0; i < mesh.verts.size(); i++) {
        const Vec3& v = mesh.verts[i];
        Vec4 world = transform.multiply({v.x, v.y, v.z, 1.f});
        Vec4 cam   = view.multiply(world);
        viewVerts[i] = {cam.x, cam.y, cam.z};

        float sx, sy;
        screenVerts[i] = projectPoint(viewVerts[i], sx, sy)
                       ? Vec3{sx, sy, cam.z}
                       : Vec3{-9999.f, -9999.f, -9999.f};
    }

    for (const auto& tri : mesh.tris) {
        Vec3 v0 = viewVerts[tri.a];
        Vec3 v1 = viewVerts[tri.b];
        Vec3 v2 = viewVerts[tri.c];

        if (v0.z < NEAR && v1.z < NEAR && v2.z < NEAR) continue;

        Vec3 p0 = screenVerts[tri.a];
        Vec3 p1 = screenVerts[tri.b];
        Vec3 p2 = screenVerts[tri.c];
        if (p0.z < 0 || p1.z < 0 || p2.z < 0) continue;

        Vec3 uv0 = !mesh.uvs.empty() ? mesh.uvs[tri.a] : Vec3{0,0,0};
        Vec3 uv1 = !mesh.uvs.empty() ? mesh.uvs[tri.b] : Vec3{0,0,0};
        Vec3 uv2 = !mesh.uvs.empty() ? mesh.uvs[tri.c] : Vec3{0,0,0};

        SDL_Color flatCol = {180, 100, 40, 255};
        if (!mesh.colors.empty()) {
            SDL_Color c0 = mesh.colors[tri.a];
            SDL_Color c1 = mesh.colors[tri.b];
            SDL_Color c2 = mesh.colors[tri.c];
            flatCol = {
                (Uint8)((c0.r + c1.r + c2.r) / 3),
                (Uint8)((c0.g + c1.g + c2.g) / 3),
                (Uint8)((c0.b + c1.b + c2.b) / 3),
                255
            };
        }

        drawTriangle(p0, p1, p2, uv0, uv1, uv2, texture, flatCol);
    }
}

static float frand01() { return (float)(rand() % 10000) / 10000.f; }

/* Añade nuevas líneas de velocidad y avanza las existentes; elimina las expiradas o fuera de pantalla. */
void Renderer::updateSpeedLines(float dt, float intensity) {
    intensity = std::max(0.f, std::min(1.f, intensity));

    const float cx = W * 0.5f;
    const float cy = H * 0.5f;

    const int   targetCount = (int)(intensity * 90.f);
    const float baseSpeed   = 400.f + intensity * 2400.f;

    while ((int)streaks.size() < targetCount) {
        const float deadR  = 70.f;
        const float outerR = deadR + (30.f + intensity * 60.f);

        float ang = frand01() * 6.28318530718f;
        float r   = sqrtf(frand01() * (outerR*outerR - deadR*deadR) + deadR*deadR);

        float dx = cosf(ang), dy = sinf(ang);

        SpeedStreak s{};
        s.x    = cx + dx * r;
        s.y    = cy + dy * r;
        s.vx   = dx * baseSpeed;
        s.vy   = dy * baseSpeed;
        s.life = 0.25f + frand01() * 0.35f;
        s.len  = 10.f + intensity * 60.f + frand01() * 25.f;
        streaks.push_back(s);
    }

    for (auto& s : streaks) {
        s.x    += s.vx * dt;
        s.y    += s.vy * dt;
        s.life -= dt;
    }

    streaks.erase(
        std::remove_if(streaks.begin(), streaks.end(),
            [&](const SpeedStreak& s) {
                return s.life <= 0.f
                    || s.x < -50.f || s.x > W + 50.f
                    || s.y < -50.f || s.y > H + 50.f;
            }),
        streaks.end()
    );

    if (targetCount == 0) streaks.clear();
}

/* Devuelve true si el segmento (x0,y0)-(x1,y1) intersecta el círculo (cx,cy,r). */
static bool segmentHitsCircle(float x0, float y0, float x1, float y1,
                              float cx, float cy, float r) {
    float vx = x1 - x0, vy = y1 - y0;
    float wx = cx - x0, wy = cy - y0;
    float vv = vx*vx + vy*vy;

    float t = (vv < 1e-6f) ? 0.f
                            : std::max(0.f, std::min(1.f, (wx*vx + wy*vy) / vv));
    float px = x0 + t * vx;
    float py = y0 + t * vy;
    float dx = px - cx, dy = py - cy;
    return (dx*dx + dy*dy) <= r*r;
}

/* Dibuja las líneas de velocidad activas; omite las que solapan la zona central. */
void Renderer::drawSpeedLines(float intensity) {
    if (intensity <= 0.01f) return;
    intensity = std::max(0.f, std::min(1.f, intensity));

    const float deadR  = 70.f;
    const float deadR2 = deadR * deadR;
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;

    const Uint8 alpha = (Uint8)(30 + intensity * 180);
    SDL_SetRenderDrawColor(sdlRend, 255, 255, 255, alpha);

    for (const auto& s : streaks) {
        float dxC = s.x - cx, dyC = s.y - cy;
        if (dxC*dxC + dyC*dyC < deadR2) continue;

        float inv = 1.f / (sqrtf(s.vx*s.vx + s.vy*s.vy) + 1e-6f);
        float dx = s.vx * inv;
        float dy = s.vy * inv;

        float x0 = s.x,          y0 = s.y;
        float x1 = s.x - dx * s.len, y1 = s.y - dy * s.len;

        if (segmentHitsCircle(x0, y0, x1, y1, cx, cy, deadR)) continue;

        SDL_RenderDrawLine(sdlRend, (int)x0, (int)y0, (int)x1, (int)y1);
    }
}