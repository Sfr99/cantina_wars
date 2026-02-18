/*
 * Entity.hpp
 * Clase base para todos los objetos del juego. Encapsula posición, velocidad,
 * rotación (Euler) y escala, con lógica de actualización y transformación mundial.
 */
#pragma once
#include "Math3D.hpp"

struct Entity {
    Vec3  pos   = {0.f, 0.f, 0.f};
    Vec3  vel   = {0.f, 0.f, 0.f};
    Vec3  rot   = {0.f, 0.f, 0.f};  // pitch(x), yaw(y), roll(z) en radianes
    float scale = 1.f;
    bool  alive = true;

    /* Integra la posición con la velocidad actual durante dt segundos. */
    virtual void update(float dt) { pos += vel * dt; }

    /* Devuelve la matriz modelo: traslación * rotación YXZ * escala uniforme. */
    virtual Mat4 worldTransform() const {
        return Mat4::translation(pos.x, pos.y, pos.z)
             * Mat4::rotationY(rot.y)
             * Mat4::rotationX(rot.x)
             * Mat4::rotationZ(rot.z)
             * Mat4::scale(scale);
    }

    virtual ~Entity() = default;
};