#pragma once
#include "Math3D.hpp"

struct Entity {
    Vec3  pos   = {0.f, 0.f, 0.f};
    Vec3  vel   = {0.f, 0.f, 0.f};
    Vec3  rot   = {0.f, 0.f, 0.f};  // euler: pitch(x), yaw(y), roll(z)
    float scale = 1.f;
    bool  alive = true;

    virtual void update(float dt) { pos += vel * dt; }

    virtual Mat4 worldTransform() const {
        return Mat4::translation(pos.x, pos.y, pos.z)
             * Mat4::rotationY(rot.y)
             * Mat4::rotationX(rot.x)
             * Mat4::rotationZ(rot.z)
             * Mat4::scale(scale);
    }

    virtual ~Entity() = default;
};