#include "Ship.hpp"
#include <algorithm>
#include <cmath>

/* Genera la malla de fallback: pirámide de 7 vértices con alas, aleta y cabina. */
Mesh Ship::createMesh() {
    return {
        {
            { 0.0f,  0.0f,  2.0f},   // 0  morro
            {-1.2f,  0.0f, -1.0f},   // 1  ala izq
            { 1.2f,  0.0f, -1.0f},   // 2  ala der
            { 0.0f,  0.7f, -0.5f},   // 3  aleta superior
            { 0.0f, -0.3f, -0.5f},   // 4  base inferior
            {-0.4f,  0.0f,  0.5f},   // 5  cabina izq
            { 0.4f,  0.0f,  0.5f},   // 6  cabina der
        },
        {
            {0,1},{0,2},{1,2},
            {0,3},{3,1},{3,2},
            {0,4},{4,1},{4,2},
            {1,5},{2,6},{5,6},
            {3,5},{3,6},
        },
        {}, {}, {}
    };
}

/* Rota fwd, up y rgt alrededor de axis en angle radianes (fórmula de Rodrigues). */
void Ship::rotate(Vec3 axis, float angle) {
    auto rodrigues = [](Vec3 v, Vec3 ax, float ang) -> Vec3 {
        float c = cosf(ang), s = sinf(ang);
        return v * c + ax.cross(v) * s + ax * (ax.dot(v) * (1.f - c));
    };
    fwd = rodrigues(fwd, axis, angle).normalized();
    up  = rodrigues(up,  axis, angle).normalized();
    rgt = rodrigues(rgt, axis, angle).normalized();
}

/* Procesa el teclado: acelera/frena en X e Y, aplica inclinación visual y gestiona disparo. */
void Ship::handleInput(const Uint8* keys, float dt) {
    const float STRAFE_SPEED = 22.f;
    const float TILT_AMOUNT  = 0.35f;
    const float ACCEL        = 120.f;
    const float DECEL        = 180.f;

    Vec3 input = {0, 0, 0};
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  input.x =  1.f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) input.x = -1.f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    input.y =  1.f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  input.y = -1.f;

    auto applyAxis = [&](float& v, float inp) {
        if (inp != 0.f) {
            v += inp * ACCEL * dt;
            v  = std::max(-STRAFE_SPEED, std::min(STRAFE_SPEED, v));
        } else {
            float dec = DECEL * dt;
            if      (v >  dec) v -= dec;
            else if (v < -dec) v += dec;
            else                v  = 0.f;
        }
    };

    applyAxis(vel.x, input.x);
    applyAxis(vel.y, input.y);
    vel.z = FORWARD_SPEED;

    // Inclinación proporcional a la velocidad lateral actual
    rot.z = -(vel.x / STRAFE_SPEED) * TILT_AMOUNT;
    rot.x =  (vel.y / STRAFE_SPEED) * TILT_AMOUNT;

    shootCooldown -= dt;
    wantsShoot = keys[SDL_SCANCODE_F] && (shootCooldown <= 0.f);
    if (wantsShoot) shootCooldown = SHOOT_DELAY;
}

/* Integra posición, clampea en los límites del escenario y actualiza el timer de invencibilidad. */
void Ship::update(float dt) {
    Vec3 v = vel;
    v.z *= speedMult;
    pos += v * dt;

    const float LIMIT_X = 32.f;
    const float LIMIT_Y = 20.f;
    pos.x = std::max(-LIMIT_X, std::min(LIMIT_X, pos.x));
    pos.y = std::max(-LIMIT_Y, std::min(LIMIT_Y, pos.y));

    if (invincible) {
        invincibleTimer -= dt;
        if (invincibleTimer <= 0.f) invincible = false;
    }
}

/* Construye la matriz modelo sin rotación Y (la nave siempre mira hacia +Z). */
Mat4 Ship::worldTransform() const {
    return Mat4::translation(pos.x, pos.y, pos.z)
         * Mat4::rotationX(rot.x)
         * Mat4::rotationZ(rot.z)
         * Mat4::scale(scale);
}

/* Resetea todos los parámetros de vuelo y activa la invencibilidad inicial. */
void Ship::respawn() {
    pos            = {0.f, 0.f, 0.f};
    vel            = {0.f, 0.f, 30.f};
    rot            = {0.f, 0.f, 0.f};
    fwd            = {0.f, 0.f, 1.f};
    up             = {0.f, 1.f, 0.f};
    rgt            = {1.f, 0.f, 0.f};
    invincible     = true;
    invincibleTimer = 3.f;
    alive          = true;
}