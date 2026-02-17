#include "Asteroid.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <map>
#include <array>
#include <cstdio>

static SDL_Surface* g_dispMaps[7] = {nullptr};
static SDL_Surface* g_diffuseMaps[7] = {nullptr};
static int g_dispMapCount = 0;

static void loadDisplacementMaps() {
    static bool loaded = false;
    if (loaded) return;
    loaded = true;

   /* if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        printf("Warning: IMG_Init failed: %s\n", IMG_GetError());
        return;
    }*/

    for (int i = 0; i < 7; i++) {
        char dispPath[64], diffPath[64];

        snprintf(dispPath, sizeof(dispPath), "../assets/rocks/asteroid_disp_%d.png", i);
        g_dispMaps[i] = IMG_Load(dispPath);
        if (!g_diffuseMaps[i]) {
            snprintf(diffPath, sizeof(diffPath), "../assets/rocks/asteroid_diff_%d.png", i);
            g_diffuseMaps[i] = IMG_Load(diffPath);
        }

        if (g_diffuseMaps[i]) {
            SDL_Surface* conv = SDL_ConvertSurfaceFormat(g_diffuseMaps[i], SDL_PIXELFORMAT_ARGB8888, 0);
            if (conv) {
                SDL_FreeSurface(g_diffuseMaps[i]);
                g_diffuseMaps[i] = conv;
            }
            SDL_SetSurfaceRLE(g_diffuseMaps[i], 0);
        }

        if (g_dispMaps[i]) {
            g_dispMapCount++;
            /*printf(" Loaded: %s (%dx%d)", dispPath, g_dispMaps[i]->w, g_dispMaps[i]->h);
            if (g_diffuseMaps[i]) {
                printf(" + diffuse (%dx%d)", g_diffuseMaps[i]->w, g_diffuseMaps[i]->h);
            }
            printf("\n");*/
        }
    }

    /*if (g_dispMapCount == 0) {
        printf(" Warning: No displacement maps found\n");
        printf("   Expected: asteroid_disp_0.png ... asteroid_disp_6.png\n");
        printf("   Optional: asteroid_diff_0.jpg ... asteroid_diff_6.jpg\n");
    } else {
        printf("✓ Total displacement maps loaded: %d\n", g_dispMapCount);
    }*/
}


static float sampleHeightmap(SDL_Surface* map, float u, float v) {
    if (!map) return 0.f;
    int x = (int)(u * (map->w - 1)) % map->w;
    int y = (int)((1.0f - v) * (map->h - 1)) % map->h;
    if (x < 0) x += map->w;
    if (y < 0) y += map->h;

    Uint8* pixels = (Uint8*)map->pixels;
    int bpp = map->format->BytesPerPixel;
    Uint8* p = pixels + y * map->pitch + x * bpp;

    Uint8 gray = (bpp == 1) ? p[0] : (p[0] + p[1] + p[2]) / 3;
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

    int texIndex = -1;
    if (mode == RenderMode::HD) {
        for (int attempt = 0; attempt < 10 && texIndex < 0; attempt++) {
            int idx = rand() % 7;
            if (g_dispMaps[idx]) {
                texIndex = idx;
                break;
            }
        }
        diffuseTexture = (texIndex >= 0) ? g_diffuseMaps[texIndex] : nullptr;
    }

    mesh = (mode == RenderMode::HD) ? generateMeshHD(radius, seed, texIndex)
                                     : generateMeshLowPoly(radius, seed);
}

void Asteroid::update(float dt) {
    pos += vel * dt;
    rot.x += rotSpeed.x * dt;
    rot.y += rotSpeed.y * dt;
    rot.z += rotSpeed.z * dt;
}

Mesh Asteroid::generateMeshLowPoly(float radius, int seed) {
    srand((unsigned)(seed * 1337 + 42));

    const float phi = (1.f + sqrtf(5.f)) * 0.5f;

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

static int subdivideEdge(std::vector<Vec3>& verts, std::map<std::pair<int,int>, int>& cache,
                         int a, int b) {
    auto key = (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    Vec3 mid = (verts[a] + verts[b]) * 0.5f;
    int idx = (int)verts.size();
    verts.push_back(mid.normalized());
    cache[key] = idx;
    return idx;
}

Mesh Asteroid::generateMeshHD(float radius, int seed, int texIndex) {
    srand((unsigned)(seed * 1337 + 42));

    SDL_Surface* dispMap = (texIndex >= 0) ? g_dispMaps[texIndex] : nullptr;
    
    /*if (dispMap) {
        printf("  → Using maps %d for asteroid (seed=%d)\n", texIndex, seed);
    } else {
        printf("  → No displacement map, using random perturbation\n");
    }*/

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

    // Subdividir 2 veces
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

    // Generar UVs esféricas
    std::vector<Vec3> uvs;
    uvs.reserve(verts.size());
    for (auto& v : verts) {
        float theta = atan2f(v.z, v.x);
        float phi_angle = asinf(v.y);
        float u = (theta / (2.f * 3.14159f)) + 0.5f;
        float v_coord = (phi_angle / 3.14159f) + 0.5f;
        uvs.push_back({u, v_coord, 0});
    }

    // Aplicar displacement
    std::vector<float> heights;
    heights.reserve(verts.size());
    
    for (size_t i = 0; i < verts.size(); i++) {
        float u = uvs[i].x;
        float v = uvs[i].y;

        float height = dispMap ? sampleHeightmap(dispMap, u, v)
                                : ((rand() % 100) / 100.f - 0.5f);
        heights.push_back(height);
        
        float disp = radius * (1.f + height * 0.35f);
        verts[i] = verts[i] * disp;
    }

    // Calcular colores por altura (fallback)
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
            col.r = (Uint8)(50  + s * 60);
            col.g = (Uint8)(30  + s * 40);
            col.b = (Uint8)(20  + s * 20);
        } else if (t < 0.66f) {
            float s = (t - 0.33f) / 0.33f;
            col.r = (Uint8)(110 + s * 50);
            col.g = (Uint8)(70  + s * 60);
            col.b = (Uint8)(40  + s * 50);
        } else {
            float s = (t - 0.66f) / 0.34f;
            col.r = (Uint8)(160 + s * 95);
            col.g = (Uint8)(130 + s * 100);
            col.b = (Uint8)(90  + s * 60);
        }
        col.a = 255;
        colors.push_back(col);
    }

    // Extraer aristas únicas
    std::map<std::pair<int,int>, bool> edgeSet;
    for (auto& tri : tris) {
        auto addEdge = [&](int a, int b) {
            auto key = (a < b) ? std::make_pair(a,b) : std::make_pair(b,a);
            edgeSet[key] = true;
        };
        addEdge(tri[0], tri[1]);
        addEdge(tri[1], tri[2]);
        addEdge(tri[2], tri[0]);
    }

    std::vector<Edge> edges;
    edges.reserve(edgeSet.size());
    for (auto& [key, _] : edgeSet) {
        edges.push_back({key.first, key.second});
    }

    // Convertir triangulos a struct Triangle
    std::vector<Triangle> triangles;
    triangles.reserve(tris.size());
    for (auto& t : tris) {
        triangles.push_back({t[0], t[1], t[2]});
    }

    return {verts, edges, triangles, uvs, colors};
}