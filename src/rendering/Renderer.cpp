#include "Renderer.hpp"
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>

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
    if (m_pixelBuf)         SDL_FreeSurface(m_pixelBuf);
    if (m_pixelTex)         SDL_DestroyTexture(m_pixelTex);
    if (backgroundTexture)  SDL_DestroyTexture(backgroundTexture);
    if (sdlRend)            SDL_DestroyRenderer(sdlRend);
    if (window)             SDL_DestroyWindow(window);
}

/* Crea ventana, renderer, pixel buffer y textura streaming. Idempotente. */
bool Renderer::init() {
    if (window) return true;

    window = SDL_CreateWindow("Cantina Wars",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              W, H, SDL_WINDOW_SHOWN);
    if (!window) return false;

    sdlRend = SDL_CreateRenderer(window, -1,
                                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdlRend) return false;

    SDL_SetRenderDrawBlendMode(sdlRend, SDL_BLENDMODE_BLEND);

    // Pixel buffer CPU: escritura directa sin overhead SDL por píxel
    m_pixelBuf = SDL_CreateRGBSurface(0, W, H, 32,
                                      0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!m_pixelBuf) return false;

    // Textura streaming: destino de la subida GPU una vez por frame
    m_pixelTex = SDL_CreateTexture(sdlRend, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!m_pixelTex) return false;

    SDL_Surface* bg = convertToARGB8888(IMG_Load("../assets/space.png"));
    if (bg) {
        backgroundTexture = SDL_CreateTextureFromSurface(sdlRend, bg);
        SDL_FreeSurface(bg);
    }

    generateStars(300);
    zbuf.assign(W * H, 1e9f);
    return true;
}

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

void Renderer::drawStars() {
    for (const auto& s : stars) {
        SDL_SetRenderDrawColor(sdlRend, s.brightness, s.brightness, s.brightness, 255);
        SDL_RenderDrawPoint(sdlRend, (int)s.x, (int)s.y);
    }
}

/* Limpia renderer SDL, pixel buffer (memset) y z-buffer. */
void Renderer::clear(bool hdMode) {
    SDL_SetRenderDrawColor(sdlRend, 0, 0, 0, 255);
    SDL_RenderClear(sdlRend);

    if (hdMode && backgroundTexture)
        SDL_RenderCopy(sdlRend, backgroundTexture, nullptr, nullptr);
    else
        drawStars();

    // Limpiar pixel buffer: todos los píxeles a negro transparente
    SDL_LockSurface(m_pixelBuf);
    memset(m_pixelBuf->pixels, 0, (size_t)H * m_pixelBuf->pitch);
    SDL_UnlockSurface(m_pixelBuf);

    std::fill(zbuf.begin(), zbuf.end(), 1e9f);
}

/* Sube el pixel buffer a GPU en una sola operación y lo vuelca sobre el renderer. */
void Renderer::flushPixelBuffer() {
    SDL_UpdateTexture(m_pixelTex, nullptr, m_pixelBuf->pixels, m_pixelBuf->pitch);
    SDL_SetTextureBlendMode(m_pixelTex, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(sdlRend, m_pixelTex, nullptr, nullptr);
}

void Renderer::present() {
    SDL_RenderPresent(sdlRend);
}

void Renderer::setWindowTitle(const std::string& title) {
    SDL_SetWindowTitle(window, title.c_str());
}

bool Renderer::projectPoint(Vec3 vp, float& sx, float& sy) const {
    if (vp.z < NEAR) return false;
    sx =  (vp.x / vp.z) * FOV + W * 0.5f;
    sy = -(vp.y / vp.z) * FOV + H * 0.5f;
    return true;
}

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
                (c1.r+c2.r)/2, (c1.g+c2.g)/2, (c1.b+c2.b)/2, 255);
        } else {
            SDL_SetRenderDrawColor(sdlRend, color.r, color.g, color.b, color.a);
        }
        SDL_RenderDrawLine(sdlRend, (int)sx1, (int)sy1, (int)sx2, (int)sy2);
    }
}

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

/* Escribe el triángulo directamente en m_pixelBuf; sin llamadas SDL por píxel. */
void Renderer::drawTriangle(Vec3 p0, Vec3 p1, Vec3 p2,
                            Vec3 uv0, Vec3 uv1, Vec3 uv2,
                            SDL_Surface* texture, SDL_Color flatColor) {
    int minX = std::max(0,     (int)std::min({p0.x, p1.x, p2.x}));
    int maxX = std::min(W - 1, (int)std::max({p0.x, p1.x, p2.x}));
    int minY = std::max(0,     (int)std::min({p0.y, p1.y, p2.y}));
    int maxY = std::min(H - 1, (int)std::max({p0.y, p1.y, p2.y}));

    float area = (p1.x-p0.x)*(p2.y-p0.y) - (p2.x-p0.x)*(p1.y-p0.y);
    if (fabsf(area) < 1e-6f) return;

    // Puntero base al pixel buffer; pitch en bytes, no en píxeles
    Uint8* bufBase = (Uint8*)m_pixelBuf->pixels;
    const int pitch = m_pixelBuf->pitch;

    for (int y = minY; y <= maxY; y++) {
        Uint32* row = (Uint32*)(bufBase + y * pitch);
        for (int x = minX; x <= maxX; x++) {
            float px = x + 0.5f, py = y + 0.5f;

            float w0 = ((p1.x-px)*(p2.y-py) - (p2.x-px)*(p1.y-py)) / area;
            float w1 = ((p2.x-px)*(p0.y-py) - (p0.x-px)*(p2.y-py)) / area;
            float w2 = 1.0f - w0 - w1;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float z = w0*p0.z + w1*p1.z + w2*p2.z;
            if (z < NEAR) continue;

            const int idx = y * W + x;
            if (z >= zbuf[idx]) continue;
            zbuf[idx] = z;

            Uint8 r, g, b;
            if (texture) {
                float u = w0*uv0.x + w1*uv1.x + w2*uv2.x;
                float v = w0*uv0.y + w1*uv1.y + w2*uv2.y;
                Uint32 tc = sampleTexture(texture, u, v);
                SDL_GetRGB(tc, texture->format, &r, &g, &b);
            } else {
                r = flatColor.r; g = flatColor.g; b = flatColor.b;
            }

            // Escritura directa: 0xFF000000 = alpha=255
            row[x] = 0xFF000000 | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
        }
    }
}

/* Proyecta vértices, aplica backface culling en espacio cámara y rasteriza. */
void Renderer::drawFilledMesh(const Mesh& mesh, const Mat4& transform,
                              const Mat4& view, SDL_Surface* texture) {
    if (mesh.tris.empty()) return;

    std::vector<Vec3> viewVerts(mesh.verts.size());
    std::vector<Vec3> screenVerts(mesh.verts.size());

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

    SDL_LockSurface(m_pixelBuf);

    for (const auto& tri : mesh.tris) {
        Vec3 v0 = viewVerts[tri.a];
        Vec3 v1 = viewVerts[tri.b];
        Vec3 v2 = viewVerts[tri.c];

        // Backface culling en espacio cámara:
        // si la normal apunta en la misma dirección que el vector cámara→cara, es dorso
        Vec3 normal = (v1 - v0).cross(v2 - v0);
        if (normal.dot(v0) >= 0) continue;

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
                (Uint8)((c0.r+c1.r+c2.r)/3),
                (Uint8)((c0.g+c1.g+c2.g)/3),
                (Uint8)((c0.b+c1.b+c2.b)/3),
                255
            };
        }

        drawTriangle(p0, p1, p2, uv0, uv1, uv2, texture, flatCol);
    }

    SDL_UnlockSurface(m_pixelBuf);
}

static float frand01() { return (float)(rand() % 10000) / 10000.f; }

void Renderer::updateSpeedLines(float dt, float intensity) {
    intensity = std::max(0.f, std::min(1.f, intensity));

    const float cx = W * 0.5f, cy = H * 0.5f;
    const int   targetCount = (int)(intensity * 90.f);
    const float baseSpeed   = 400.f + intensity * 2400.f;

    while ((int)streaks.size() < targetCount) {
        const float deadR  = 70.f;
        const float outerR = deadR + (30.f + intensity * 60.f);
        float ang = frand01() * 6.28318530718f;
        float r   = sqrtf(frand01() * (outerR*outerR - deadR*deadR) + deadR*deadR);
        float dx  = cosf(ang), dy = sinf(ang);
        SpeedStreak s{};
        s.x = cx + dx*r; s.y = cy + dy*r;
        s.vx = dx*baseSpeed; s.vy = dy*baseSpeed;
        s.life = 0.25f + frand01()*0.35f;
        s.len  = 10.f + intensity*60.f + frand01()*25.f;
        streaks.push_back(s);
    }

    for (auto& s : streaks) { s.x += s.vx*dt; s.y += s.vy*dt; s.life -= dt; }

    streaks.erase(
        std::remove_if(streaks.begin(), streaks.end(),
            [&](const SpeedStreak& s) {
                return s.life <= 0.f
                    || s.x < -50.f || s.x > W+50.f
                    || s.y < -50.f || s.y > H+50.f;
            }),
        streaks.end());

    if (targetCount == 0) streaks.clear();
}

static bool segmentHitsCircle(float x0, float y0, float x1, float y1,
                              float cx, float cy, float r) {
    float vx = x1-x0, vy = y1-y0;
    float wx = cx-x0, wy = cy-y0;
    float vv = vx*vx + vy*vy;
    float t  = (vv < 1e-6f) ? 0.f : std::max(0.f, std::min(1.f, (wx*vx+wy*vy)/vv));
    float dx = x0+t*vx-cx, dy = y0+t*vy-cy;
    return (dx*dx + dy*dy) <= r*r;
}

void Renderer::drawSpeedLines(float intensity) {
    if (intensity <= 0.01f) return;
    intensity = std::max(0.f, std::min(1.f, intensity));

    const float deadR  = 70.f, deadR2 = deadR*deadR;
    const float cx = W*0.5f, cy = H*0.5f;

    SDL_SetRenderDrawColor(sdlRend, 255, 255, 255, (Uint8)(30 + intensity*180));

    for (const auto& s : streaks) {
        float dxC = s.x-cx, dyC = s.y-cy;
        if (dxC*dxC + dyC*dyC < deadR2) continue;

        float inv = 1.f / (sqrtf(s.vx*s.vx + s.vy*s.vy) + 1e-6f);
        float x0 = s.x,             y0 = s.y;
        float x1 = s.x-s.vx*inv*s.len, y1 = s.y-s.vy*inv*s.len;

        if (segmentHitsCircle(x0, y0, x1, y1, cx, cy, deadR)) continue;
        SDL_RenderDrawLine(sdlRend, (int)x0, (int)y0, (int)x1, (int)y1);
    }
}