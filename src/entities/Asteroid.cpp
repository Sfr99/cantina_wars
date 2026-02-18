#include "Asteroid.hpp"
#include <SDL2/SDL_image.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <map>
#include <array>

// Texturas compartidas entre todos los asteroides; cargadas una sola vez.
static SDL_Surface* g_dispMaps[7]    = {nullptr};
static SDL_Surface* g_diffuseMaps[7] = {nullptr};
static int          g_dispMapCount   = 0;

/* Carga displacement y diffuse maps desde assets/rocks/ la primera vez que se llama. */
static void loadDisplacementMaps() {
    static bool loaded = false;
    if (loaded) return;
    loaded = true;

    for (int i = 0; i < 7; i++) {
        char dispPath[64], diffPath[64];
        snprintf(dispPath, sizeof(dispPath), "../assets/rocks/asteroid_disp_%d.png", i);
        snprintf(diffPath, sizeof(diffPath), "../assets/rocks/asteroid_diff_%d.png", i);

        g_dispMaps[i] = IMG_Load(dispPath);

        if (!g_diffuseMaps[i]) {
            g_diffuseMaps[i] = IMG_Load(diffPath);
        }

        // Convertir a ARGB8888 para lecturas de píxel consistentes
        // ARGB8888 es un formato común que facilita el muestreo de píxeles con SDL_MapRGBA
        if (g_diffuseMaps[i]) {
            SDL_Surface* conv = SDL_ConvertSurfaceFormat(g_diffuseMaps[i], SDL_PIXELFORMAT_ARGB8888, 0);
            if (conv) {
                SDL_FreeSurface(g_diffuseMaps[i]);
                g_diffuseMaps[i] = conv;
            }
            SDL_SetSurfaceRLE(g_diffuseMaps[i], 0);
        }

        if (g_dispMaps[i]) g_dispMapCount++;
    }
}

/* Muestrea un heightmap en coordenadas UV normalizadas; devuelve valor en [-1, 1]. */
static float sampleHeightmap(SDL_Surface* map, float u, float v) {
    if (!map) return 0.f;

    int x = (int)(u * (map->w - 1)) % map->w;
    int y = (int)((1.0f - v) * (map->h - 1)) % map->h;
    if (x < 0) x += map->w;
    if (y < 0) y += map->h;

    Uint8* p   = (Uint8*)map->pixels + y * map->pitch + x * map->format->BytesPerPixel;
    Uint8  gray = (map->format->BytesPerPixel == 1) ? p[0] : (p[0] + p[1] + p[2]) / 3;

    return (gray / 255.f) * 2.f - 1.f;
}

float Asteroid::radiusForSize(AsteroidSize s) {
    switch (s) {
        case AsteroidSize::LARGE:  return 4.275f;
        case AsteroidSize::MEDIUM: return 2.7f;
        case AsteroidSize::SMALL:  return 1.485f;
    }
    return 1.1f;
}

Asteroid::Asteroid(Vec3 p, Vec3 v, AsteroidSize sz, RenderMode mode, int seed) {
    loadDisplacementMaps();

    pos    = p;
    vel    = v;
    size   = sz;
    scale  = 1.f;
    radius = radiusForSize(sz);

    srand((unsigned)(seed + (int)sz * 137));
    rotSpeed = {
        ((rand() % 200) - 100) * 0.008f,
        ((rand() % 200) - 100) * 0.008f,
        ((rand() % 200) - 100) * 0.008f
    };

    // Seleccionar textura disponible para modo HD
    int texIndex = -1;
    if (mode == RenderMode::HD) {
        for (int attempt = 0; attempt < 10 && texIndex < 0; attempt++) {
            int idx = rand() % 7;
            if (g_dispMaps[idx]) texIndex = idx;
        }
        diffuseTexture = (texIndex >= 0) ? g_diffuseMaps[texIndex] : nullptr;
    }

    mesh = (mode == RenderMode::HD) ? generateMeshHD(radius, seed, texIndex)
                                    : generateMeshLowPoly(radius, seed);
}

/* Integra posición y acumula rotación según rotSpeed. */
void Asteroid::update(float dt) {
    pos   += vel * dt;
    rot.x += rotSpeed.x * dt;
    rot.y += rotSpeed.y * dt;
    rot.z += rotSpeed.z * dt;
}

Mesh Asteroid::generateMeshLowPoly(float radius, int seed) {
    srand((unsigned)(seed * 1337 + 42));

    const float phi = (1.f + sqrtf(5.f)) * 0.5f;

    // Vértices base del icosaedro
    std::vector<Vec3> raw = {
        { 0,  1,  phi}, { 0, -1,  phi}, { 0,  1, -phi}, { 0, -1, -phi},
        { 1,  phi, 0},  {-1,  phi, 0},  { 1, -phi, 0},  {-1, -phi, 0},
        { phi, 0,  1},  {-phi, 0,  1},  { phi, 0, -1},  {-phi, 0, -1}
    };

    std::vector<Vec3> verts;
    verts.reserve(raw.size());
    for (auto& v : raw) {
        float perturb = radius * (0.75f + (rand() % 100) * 0.005f);
        verts.push_back(v * (perturb / v.length()));
    }

    std::vector<Edge> edges = {
        {0,1},{0,4},{0,5},{0,8},{0,9},
        {1,6},{1,7},{1,8},{1,9},
        {2,3},{2,4},{2,5},{2,10},{2,11},
        {3,6},{3,7},{3,10},{3,11},
        {4,5},{4,8},{4,10},
        {5,9},{5,11},
        {6,7},{6,8},{6,10},
        {7,9},{7,11},
        {8,10},
        {9,11}
    };

    return {verts, edges, {}, {}, {}};
}

/* Inserta el punto medio de la arista (a,b) en verts si aún no existe; devuelve su índice. */
static int subdivideEdge(std::vector<Vec3>& verts,
                         std::map<std::pair<int,int>, int>& cache,
                         int a, int b) {
    auto key = (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
    auto it  = cache.find(key);
    if (it != cache.end()) return it->second;

    Vec3 mid = (verts[a] + verts[b]) * 0.5f;
    int  idx = (int)verts.size();
    verts.push_back(mid.normalized());
    cache[key] = idx;
    return idx;
}

Mesh Asteroid::generateMeshHD(float radius, int seed, int texIndex) {
    srand((unsigned)(seed * 1337 + 42));

    SDL_Surface* dispMap = (texIndex >= 0) ? g_dispMaps[texIndex] : nullptr;

    const float phi = (1.f + sqrtf(5.f)) * 0.5f;

    std::vector<Vec3> verts = {
        Vec3{ 0,  1,  phi}.normalized(), Vec3{ 0, -1,  phi}.normalized(),
        Vec3{ 0,  1, -phi}.normalized(), Vec3{ 0, -1, -phi}.normalized(),
        Vec3{ 1,  phi, 0}.normalized(),  Vec3{-1,  phi, 0}.normalized(),
        Vec3{ 1, -phi, 0}.normalized(),  Vec3{-1, -phi, 0}.normalized(),
        Vec3{ phi, 0,  1}.normalized(),  Vec3{-phi, 0,  1}.normalized(),
        Vec3{ phi, 0, -1}.normalized(),  Vec3{-phi, 0, -1}.normalized()
    };

    std::vector<std::array<int,3>> tris = {
        {0,1,8},{0,8,4},{0,4,5},{0,5,9},{0,9,1},
        {1,9,7},{1,7,6},{1,6,8},{2,3,11},{2,11,5},
        {2,5,4},{2,4,10},{2,10,3},{3,10,6},{3,6,7},
        {3,7,11},{4,8,10},{5,11,9},{6,10,8},{7,9,11}
    };

    // Dos pasadas de subdivisión para aumentar la resolución de la esfera
    for (int sub = 0; sub < 2; sub++) {
        std::vector<std::array<int,3>> newTris;
        std::map<std::pair<int,int>, int> edgeCache;

        for (auto& tri : tris) {
            int a = tri[0], b = tri[1], c = tri[2];
            int ab = subdivideEdge(verts, edgeCache, a, b);
            int bc = subdivideEdge(verts, edgeCache, b, c);
            int ca = subdivideEdge(verts, edgeCache, c, a);
            newTris.push_back({a, ab, ca});
            newTris.push_back({b, bc, ab});
            newTris.push_back({c, ca, bc});
            newTris.push_back({ab, bc, ca});
        }
        tris = newTris;
    }

    // Coordenadas UV esféricas por proyección
    std::vector<Vec3> uvs;
    uvs.reserve(verts.size());
    for (auto& v : verts) {
        float theta   = atan2f(v.z, v.x);
        float phi_ang = asinf(v.y);
        uvs.push_back({ (theta / (2.f * 3.14159f)) + 0.5f,
                        (phi_ang / 3.14159f)        + 0.5f,
                        0.f });
    }

    // Desplazar vértices según heightmap o perturbación aleatoria
    std::vector<float> heights;
    heights.reserve(verts.size());
    for (size_t i = 0; i < verts.size(); i++) {
        float height = dispMap ? sampleHeightmap(dispMap, uvs[i].x, uvs[i].y)
                               : ((rand() % 100) / 100.f - 0.5f);
        heights.push_back(height);
        verts[i] = verts[i] * (radius * (1.f + height * 0.35f));
    }

    // Colorear vértices según altura relativa (oscuro=valle, claro=pico)
    float minH = 1e9f, maxH = -1e9f;
    for (float h : heights) {
        if (h < minH) minH = h;
        if (h > maxH) maxH = h;
    }

    std::vector<SDL_Color> colors;
    colors.reserve(verts.size());
    for (float h : heights) {
        float t = (maxH > minH) ? (h - minH) / (maxH - minH) : 0.5f;
        SDL_Color col;
        if (t < 0.33f) {
            float s = t / 0.33f;
            col = { (Uint8)(50 + s*60), (Uint8)(30 + s*40), (Uint8)(20 + s*20), 255 };
        } else if (t < 0.66f) {
            float s = (t - 0.33f) / 0.33f;
            col = { (Uint8)(110 + s*50), (Uint8)(70 + s*60), (Uint8)(40 + s*50), 255 };
        } else {
            float s = (t - 0.66f) / 0.34f;
            col = { (Uint8)(160 + s*95), (Uint8)(130 + s*100), (Uint8)(90 + s*60), 255 };
        }
        colors.push_back(col);
    }

    // Extraer aristas únicas a partir de los triángulos
    std::map<std::pair<int,int>, bool> edgeSet;
    for (auto& tri : tris) {
        auto addEdge = [&](int a, int b) {
            edgeSet[(a < b) ? std::make_pair(a,b) : std::make_pair(b,a)] = true;
        };
        addEdge(tri[0], tri[1]);
        addEdge(tri[1], tri[2]);
        addEdge(tri[2], tri[0]);
    }

    std::vector<Edge> edges;
    edges.reserve(edgeSet.size());
    for (auto& [key, _] : edgeSet)
        edges.push_back({key.first, key.second});

    std::vector<Triangle> triangles;
    triangles.reserve(tris.size());
    for (auto& t : tris)
        triangles.push_back({t[0], t[1], t[2]});

    return {verts, edges, triangles, uvs, colors};
}