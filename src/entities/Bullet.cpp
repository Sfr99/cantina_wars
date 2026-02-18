#include "Bullet.hpp"

SDL_Surface* Bullet::laserTexture = nullptr;

/* Genera una textura 8x32 con degradado radial naranja-rojo para el efecto laser. */
SDL_Surface* Bullet::createLaserTexture() {
    const int W = 8, H = 32;
    SDL_Surface* s = SDL_CreateRGBSurface(0, W, H, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    SDL_LockSurface(s);
    for (int y = 0; y < H; y++) {
        float ty = 1.f - fabsf((y - H*0.5f) / (H*0.5f));
        for (int x = 0; x < W; x++) {
            float tx = 1.f - fabsf((x - W*0.5f) / (W*0.5f));
            float t  = ty * tx;
            Uint8 r  = 255;
            Uint8 g  = (Uint8)(180 * t);
            Uint8 b  = (Uint8)(50  * t);
            Uint8 a  = (Uint8)(255 * t);
            Uint32* p = (Uint32*)((Uint8*)s->pixels + y * s->pitch) + x;
            *p = SDL_MapRGBA(s->format, r, g, b, a);
        }
    }
    SDL_UnlockSurface(s);
    return s;
}

Bullet::Bullet(Vec3 p, Vec3 v) {
    pos = p;
    vel = v;
}

/* Integra la posición y consume el tiempo de vida. */
void Bullet::update(float dt) {
    pos      += vel * dt;
    lifetime -= dt;
}

/* Construye un octaedro regular de radio RADIUS con aristas y triángulos. */
Mesh Bullet::createMesh() {
    const float r = RADIUS;

    std::vector<Vec3> verts = {
        { r,  0,  0}, {-r,  0,  0},
        { 0,  r,  0}, { 0, -r,  0},
        { 0,  0,  r}, { 0,  0, -r},
    };
    std::vector<Triangle> tris = {
        {0,2,4},{0,4,3},{0,3,5},{0,5,2},
        {1,4,2},{1,3,4},{1,5,3},{1,2,5},
    };
    std::vector<Edge> edges = {
        {0,2},{0,3},{0,4},{0,5},
        {1,2},{1,3},{1,4},{1,5},
        {2,4},{4,3},{3,5},{5,2},
    };

    return {verts, edges, tris, {}, {}};
}