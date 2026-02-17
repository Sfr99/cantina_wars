#pragma once
#include "Math3D.hpp"

class Camera {
public:
    Vec3 eye     = {0.f, 3.f, -10.f};
    Vec3 target  = {0.f, 0.f,   0.f};
    Vec3 worldUp = {0.f, 1.f,   0.f};

    Mat4 viewMatrix() const;

    // Reposiciona la cámara detrás y encima de la nave
    void follow(Vec3 shipPos, Vec3 shipFwd, Vec3 shipUp);
};