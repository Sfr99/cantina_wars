#include "Camera.hpp"

Mat4 Camera::viewMatrix() const {
    return Mat4::lookAt(eye, target, worldUp);
}

void Camera::follow(Vec3 shipPos, Vec3 shipFwd, Vec3 shipUp) {
    // Cámara detrás y arriba de la nave para ver adelante claramente
    eye     = shipPos + Vec3{0.f, 4.f, -12.f};
    target  = shipPos + Vec3{0.f, 0.f, 20.f};  // mirar bien adelante
    worldUp = {0.f, 1.f, 0.f};
}