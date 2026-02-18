/*
 * entities/Asteroid.hpp
 * Asteroide con dos modos de renderizado: LOW_POLY (vertices de un icosaedro)
 * y HD (esfera subdividida con displacement map y colores por altura).
 * Las texturas de displacement y diffuse se cargan una sola vez de forma lazy.
 */
#pragma once
#include <SDL2/SDL.h>
#include "../core/Entity.hpp"
#include "../core/Mesh.hpp"

enum class AsteroidSize { LARGE, MEDIUM, SMALL };
enum class RenderMode   { LOW_POLY, HD };

class Asteroid : public Entity {
public:
    AsteroidSize size;
    Vec3         rotSpeed;
    float        radius;
    SDL_Surface* diffuseTexture = nullptr;

    Asteroid(Vec3 pos, Vec3 vel, AsteroidSize size, RenderMode mode, int seed = 0);

    /* Integra posición y aplica velocidad de rotación en los tres ejes. */
    void update(float dt) override;

    const Mesh& getMesh() const { return mesh; }

    /* Devuelve el radio de colisión asociado a cada tamaño. */
    static float radiusForSize(AsteroidSize s);

private:
    Mesh mesh;

    /* Genera un icosaedro de baja resolución con vértices perturbados aleatoriamente. */
    static Mesh generateMeshLowPoly(float radius, int seed);

    /* Genera una esfera subdividida con displacement map y colores por altura. */
    static Mesh generateMeshHD(float radius, int seed, int texIndex);
};