/*
 * entities/Bullet.hpp
 * Proyectil disparado por la nave. Tiene vida limitada y geometría de octaedro.
 * La textura laser se genera proceduralmente y se comparte entre todas las instancias.
 */
#pragma once
#include "../core/Entity.hpp"
#include "../core/Mesh.hpp"

class Bullet : public Entity {
public:
    float lifetime = 2.5f;

    static constexpr float RADIUS = 0.3f;
    static constexpr float SPEED  = 40.f;

    Bullet(Vec3 pos, Vec3 vel);

    /* Avanza la posición y decrementa el tiempo de vida restante. */
    void update(float dt) override;

    bool expired() const { return lifetime <= 0.f; }

    /* Genera la malla de octaedro que representa la bala. */
    static Mesh createMesh();

    static SDL_Surface* laserTexture;

    /* Genera proceduralmente una textura degradada naranja-roja para el laser. */
    static SDL_Surface* createLaserTexture();
};