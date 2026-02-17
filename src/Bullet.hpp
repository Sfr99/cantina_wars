#pragma once
#include "Entity.hpp"
#include "Mesh.hpp"
#include "Ship.hpp"

class Bullet : public Entity {
public:
    float lifetime = 2.5f;

    static constexpr float RADIUS = 0.3f;
    static constexpr float SPEED  = 40.f;

    Bullet(Vec3 pos, Vec3 vel);

    void update(float dt) override;
    bool expired() const { return lifetime <= 0.f; }

    static Mesh createMesh();
};