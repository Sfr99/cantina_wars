/*
 * entities/Ship.hpp
 * Nave del jugador. Gestiona movimiento por input, inclinación visual,
 * cooldown de disparo, invencibilidad temporal y respawn.
 * La carga de la malla GLTF se delega a rendering/GLTFLoader.
 */
#pragma once
#include <SDL2/SDL.h>
#include "../core/Entity.hpp"
#include "../core/Mesh.hpp"

class Ship : public Entity {
public:
    Vec3 fwd = {0.f, 0.f, 1.f};
    Vec3 up  = {0.f, 1.f, 0.f};
    Vec3 rgt = {1.f, 0.f, 0.f};

    float shootCooldown   = 0.f;
    bool  wantsShoot      = false;
    bool  invincible      = true;
    float invincibleTimer = 3.f;

    /* Genera la malla procedural de fallback (pirámide de 7 vértices). */
    static Mesh createMesh();

    /* Lee el input de teclado y actualiza velocidad e inclinación. */
    void handleInput(const Uint8* keys, float dt);

    /* Integra posición, aplica límites de pantalla y gestiona el timer de invencibilidad. */
    void update(float dt) override;

    /* Transforma sin rotación Y para mantener la nave siempre alineada con la cámara. */
    Mat4 worldTransform() const override;

    /* Resetea posición, velocidad y estado a los valores iniciales. */
    void respawn();

    void setSpeedMultiplier(float m) { speedMult = m; }

private:
    /* Rota los vectores base fwd/up/rgt alrededor de un eje mediante la fórmula de Rodrigues. */
    void rotate(Vec3 axis, float angle);

    float speedMult = 1.f;

    static constexpr float FORWARD_SPEED = 30.f;
    static constexpr float SHOOT_DELAY   = 0.22f;
};