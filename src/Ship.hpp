#pragma once
#include <SDL2/SDL.h>
#include "Entity.hpp"
#include "Mesh.hpp"

class Ship : public Entity {
public:
    // Vectores base en espacio mundo (solo para orientación visual)
    Vec3 fwd = {0.f, 0.f, 1.f};
    Vec3 up  = {0.f, 1.f, 0.f};
    Vec3 rgt = {1.f, 0.f, 0.f};

    float shootCooldown  = 0.f;
    bool  wantsShoot     = false;
    bool  invincible     = true;
    float invincibleTimer = 3.f;

    static Mesh createMesh();

    void handleInput(const Uint8* keys, float dt);
    void update(float dt) override;
    Mat4 worldTransform() const override;
    void respawn();
    void setSpeedMultiplier(float m) { speedMult = m; }

private:
    void rotate(Vec3 axis, float angle);
    float speedMult = 1.f;

    static constexpr float FORWARD_SPEED = 30.f;
    static constexpr float SHOOT_DELAY   = 0.22f;
};