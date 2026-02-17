#pragma once
#include <SDL2/SDL.h>
#include "Entity.hpp"
#include "Mesh.hpp"

enum class AsteroidSize { LARGE, MEDIUM, SMALL };
enum class RenderMode   { LOW_POLY, HD };

class Asteroid : public Entity {
public:
    AsteroidSize size;
    Vec3         rotSpeed;
    float        radius;
    SDL_Surface* diffuseTexture = nullptr;

    Asteroid(Vec3 pos, Vec3 vel, AsteroidSize size, RenderMode mode, int seed = 0);

    void update(float dt) override;

    const Mesh& getMesh() const { return mesh; }

    static float radiusForSize(AsteroidSize s);

private:
    Mesh mesh;
    static Mesh generateMeshLowPoly(float radius, int seed);
    static Mesh generateMeshHD(float radius, int seed, int texIndex);
};