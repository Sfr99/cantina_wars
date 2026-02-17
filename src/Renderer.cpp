#include "Renderer.hpp"
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>

#include <SDL2/SDL_image.h>

Renderer::Renderer(int w, int h) : W(w), H(h) {}

static SDL_Surface* ConvertToARGB8888(SDL_Surface* s) {
    if (!s) return nullptr;

    // Ya está en el formato deseado
    if (s->format && s->format->format == SDL_PIXELFORMAT_ARGB8888) {
        SDL_SetSurfaceRLE(s, 0);
        return s;
    }

    SDL_Surface* conv = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(s);

    if (conv) SDL_SetSurfaceRLE(conv, 0);
    return conv; // puede ser nullptr si falla
}

Renderer::~Renderer() {
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (sdlRend) SDL_DestroyRenderer(sdlRend);
    if (window)  SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();
}

bool Renderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return false;
    }

    const int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        // Se puede continuar sin imágenes, pero lo dejamos registrado.
    }

    window = SDL_CreateWindow("Asteroid 3D",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              W, H, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_CreateWindow error: %s\n", SDL_GetError());
        return false;
    }

    sdlRend = SDL_CreateRenderer(window, -1,
                                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdlRend) {
        printf("SDL_CreateRenderer error: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderDrawBlendMode(sdlRend, SDL_BLENDMODE_BLEND);

    SDL_Surface* tempSurface = IMG_Load("../assets/space.png");
    if (!tempSurface) {
        printf("No se pudo cargar space.png: %s\n", IMG_GetError());
    } else {
        tempSurface = ConvertToARGB8888(tempSurface);
        if (!tempSurface) {
            printf("No se pudo convertir space.png a ARGB8888\n");
        } else {
            backgroundTexture = SDL_CreateTextureFromSurface(sdlRend, tempSurface);
            if (!backgroundTexture) {
                printf("SDL_CreateTextureFromSurface error: %s\n", SDL_GetError());
            }
            SDL_FreeSurface(tempSurface);
        }
    }
    generateStars(300);
    // Si tu zbuf depende de tamaño, asegúrate de que está dimensionado
    // (si ya lo haces en el .hpp/ctor, esto no hace daño)
    if ((int)zbuf.size() != W * H) zbuf.assign(W * H, 1e9f);

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

void Renderer::clear(bool hdMode) {
    // Limpia color
    SDL_SetRenderDrawColor(sdlRend, 0, 0, 0, 255);
    SDL_RenderClear(sdlRend);

    // Dibuja fondo (si existe)
    if (hdMode && backgroundTexture) {
        SDL_RenderCopy(sdlRend, backgroundTexture, nullptr, nullptr);
    } else {
        drawStars();
    }

    // Reset del z-buffer de CPU (si lo usas)
    std::fill(zbuf.begin(), zbuf.end(), 1e9f);

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

    bool hasVertexColors = !mesh.colors.empty() && mesh.colors.size() == mesh.verts.size();

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
            SDL_Color avg = {
                (Uint8)((c1.r + c2.r) / 2),
                (Uint8)((c1.g + c2.g) / 2),
                (Uint8)((c1.b + c2.b) / 2),
                255
            };
            SDL_SetRenderDrawColor(sdlRend, avg.r, avg.g, avg.b, avg.a);
        } else {
            SDL_SetRenderDrawColor(sdlRend, color.r, color.g, color.b, color.a);
        }

        SDL_RenderDrawLine(sdlRend, (int)sx1, (int)sy1, (int)sx2, (int)sy2);
    }
}

Uint32 Renderer::sampleTexture(SDL_Surface* tex, float u, float v) const {
    if (!tex) return 0xFFFFFFFF;

    // Requisito: tex debe ser ARGB8888 para que este path sea correcto y rápido.
    // Si no lo es, devuelve blanco (mejor que leer basura).
    if (!tex->format || tex->format->format != SDL_PIXELFORMAT_ARGB8888) {
        return 0xFFFFFFFF;
    }

    u = u - floorf(u);
    v = v - floorf(v);
    v = 1.0f - v;

    int x = (int)(u * (tex->w - 1));
    int y = (int)(v * (tex->h - 1));
    x = (x < 0) ? 0 : (x >= tex->w ? tex->w - 1 : x);
    y = (y < 0) ? 0 : (y >= tex->h ? tex->h - 1 : y);

    if (SDL_MUSTLOCK(tex)) SDL_LockSurface(tex);

    const Uint8* pixels = (const Uint8*)tex->pixels;
    const Uint32* row = (const Uint32*)(pixels + y * tex->pitch);
    Uint32 color = row[x];

    if (SDL_MUSTLOCK(tex)) SDL_UnlockSurface(tex);
    return color;
}

void Renderer::drawTriangle(Vec3 p0, Vec3 p1, Vec3 p2,
                            Vec3 uv0, Vec3 uv1, Vec3 uv2,
                            SDL_Surface* texture, SDL_Color flatColor) {
    int minX = (int)std::min({p0.x, p1.x, p2.x});
    int maxX = (int)std::max({p0.x, p1.x, p2.x});
    int minY = (int)std::min({p0.y, p1.y, p2.y});
    int maxY = (int)std::max({p0.y, p1.y, p2.y});

    minX = std::max(0, minX);
    maxX = std::min(W - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(H - 1, maxY);

    float area = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
    if (fabsf(area) < 1e-6f) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 = ((p1.x - px) * (p2.y - py) - (p2.x - px) * (p1.y - py)) / area;
            float w1 = ((p2.x - px) * (p0.y - py) - (p0.x - px) * (p2.y - py)) / area;
            float w2 = 1.0f - w0 - w1;

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                float z = w0 * p0.z + w1 * p1.z + w2 * p2.z;
                if (z < NEAR) continue;

                const int idx = y * W + x;

                // Depth test: menor z => más cercano (en tu cámara z crece hacia delante)
                if (z >= zbuf[idx]) continue;
                zbuf[idx] = z;

                Uint8 r, g, b;
                if (texture) {
                    float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
                    float v_coord = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;

                    Uint32 texColor = sampleTexture(texture, u, v_coord);
                    SDL_GetRGB(texColor, texture->format, &r, &g, &b);
                } else {
                    float t = (z + 20.f) / 40.f;
                    t = (t < 0.f) ? 0.f : (t > 1.f ? 1.f : t);
                    r = flatColor.r;
                    g = flatColor.g;
                    b = flatColor.b;
                }

                SDL_SetRenderDrawColor(sdlRend, r, g, b, 255);
                SDL_RenderDrawPoint(sdlRend, x, y);
                static int pixCount = 0;
                static bool reported = false;
                pixCount++;
                if (pixCount > 100 && !reported) {
                    reported = true;
                    printf("drawTriangle IS writing pixels\n");
                }
            }

        }
    }
}

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
        if (projectPoint(viewVerts[i], sx, sy)) {
            screenVerts[i] = {sx, sy, cam.z};
        } else {
            screenVerts[i] = {-9999, -9999, -9999};
        }
    }
    int drawn = 0;
    for (const auto& tri : mesh.tris) {
        Vec3 v0 = viewVerts[tri.a];
        Vec3 v1 = viewVerts[tri.b];
        Vec3 v2 = viewVerts[tri.c];

        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;
        //Vec3 normal = e1.cross(e2);
        //if (normal.dot(v0) >= 0) continue;

        if (v0.z < NEAR && v1.z < NEAR && v2.z < NEAR) continue;

        Vec3 p0 = screenVerts[tri.a];
        Vec3 p1 = screenVerts[tri.b];
        Vec3 p2 = screenVerts[tri.c];

        if (p0.z < 0 || p1.z < 0 || p2.z < 0) continue;

        Vec3 uv0 = !mesh.uvs.empty() ? mesh.uvs[tri.a] : Vec3{0,0,0};
        Vec3 uv1 = !mesh.uvs.empty() ? mesh.uvs[tri.b] : Vec3{0,0,0};
        Vec3 uv2 = !mesh.uvs.empty() ? mesh.uvs[tri.c] : Vec3{0,0,0};
        static int tc = 0;
        if (tc++ < 5) printf("tri screen: (%.1f,%.1f,z=%.1f) (%.1f,%.1f,z=%.1f) (%.1f,%.1f,z=%.1f)\n",
            p0.x,p0.y,p0.z, p1.x,p1.y,p1.z, p2.x,p2.y,p2.z);
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
        drawn++;
    }
    static int frame = 0;
    //if (frame++ < 3) printf("Dibujando mesh con %zu tris, se dibujaron %d\n", mesh.tris.size(), drawn);
}

static float frand01() { return (float)(rand() % 10000) / 10000.f; }

void Renderer::updateSpeedLines(float dt, float intensity) {
    if (intensity < 0.f) intensity = 0.f;
    if (intensity > 1.f) intensity = 1.f;

    const float cx = W * 0.5f;
    const float cy = H * 0.5f;

    const int   targetCount = (int)(intensity * 90.f);
    const float baseSpeed   = 400.f + intensity * 2400.f;

    while ((int)streaks.size() < targetCount) {
        SpeedStreak s{};

        // Spawn en anillo (evita el centro)
        const float deadR  = 70.f;
        const float outerR = deadR + (30.f + intensity * 60.f);

        float ang = frand01() * 6.28318530718f;
        float u   = frand01();
        float r   = sqrtf(u * (outerR*outerR - deadR*deadR) + deadR*deadR);

        s.x = cx + cosf(ang) * r;
        s.y = cy + sinf(ang) * r;

        // Dirección radial hacia fuera
        float dx = s.x - cx;
        float dy = s.y - cy;
        float inv = 1.f / (sqrtf(dx*dx + dy*dy) + 1e-6f);
        dx *= inv; dy *= inv;

        s.vx = dx * baseSpeed;
        s.vy = dy * baseSpeed;

        s.life = 0.25f + frand01() * 0.35f;
        s.len  = 10.f + intensity * 60.f + frand01() * 25.f;

        streaks.push_back(s);
    }

    for (auto& s : streaks) {
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.life -= dt;
    }

    streaks.erase(
        std::remove_if(streaks.begin(), streaks.end(),
            [&](const SpeedStreak& s) {
                return s.life <= 0.f || s.x < -50.f || s.x > W + 50.f || s.y < -50.f || s.y > H + 50.f;
            }),
        streaks.end()
    );

    if (targetCount == 0) streaks.clear();
}

static bool segmentHitsCircle(float x0, float y0, float x1, float y1,
                              float cx, float cy, float r) {
    // distancia mínima del centro al segmento <= r
    float vx = x1 - x0;
    float vy = y1 - y0;
    float wx = cx - x0;
    float wy = cy - y0;

    float vv = vx*vx + vy*vy;
    if (vv < 1e-6f) {
        float dx = x0 - cx, dy = y0 - cy;
        return (dx*dx + dy*dy) <= r*r;
    }

    float t = (wx*vx + wy*vy) / vv;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;

    float px = x0 + t * vx;
    float py = y0 + t * vy;

    float dx = px - cx;
    float dy = py - cy;
    return (dx*dx + dy*dy) <= r*r;
}


void Renderer::drawSpeedLines(float intensity) {
    if (intensity <= 0.01f) return;

    if (intensity < 0.f) intensity = 0.f;
    if (intensity > 1.f) intensity = 1.f;

    const Uint8 a = (Uint8)(30 + intensity * 180);
    SDL_SetRenderDrawColor(sdlRend, 255, 255, 255, a);

    const float deadR = 70.f; // mismo valor que arriba
    const float deadR2 = deadR * deadR;
    const float cx = W * 0.5f;
    const float cy = H * 0.5f;
    for (const auto& s : streaks) {
        float inv = 1.f / (sqrtf(s.vx*s.vx + s.vy*s.vy) + 1e-6f);
        float dx = s.vx * inv;
        float dy = s.vy * inv;
        float dxC = s.x - cx;
        float dyC = s.y - cy;
        if (dxC*dxC + dyC*dyC < deadR2) continue;
        // Cabeza en (x,y), cola hacia atrás (hacia el centro)
        float x0 = s.x;
        float y0 = s.y;
        float x1 = s.x - dx * s.len;
        float y1 = s.y - dy * s.len;

        // Si el segmento toca la zona central, no se dibuja
        if (segmentHitsCircle(x0, y0, x1, y1, cx, cy, deadR)) continue;



        SDL_RenderDrawLine(sdlRend, (int)x0, (int)y0, (int)x1, (int)y1);
    }
}


