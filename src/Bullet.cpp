#include "Bullet.hpp"

Bullet::Bullet(Vec3 p, Vec3 v) {
    pos = p;
    vel = v;
}

void Bullet::update(float dt) {
    Vec3 v = vel;
    pos += v * dt;
    lifetime -= dt;
}

Mesh Bullet::createMesh() {
    const float r = RADIUS;
    return {
        {
            { r,  0,  0}, {-r,  0,  0},
            { 0,  r,  0}, { 0, -r,  0},
            { 0,  0,  r}, { 0,  0, -r},
        },
        {
            {0,1},{2,3},{4,5},           // ejes principales
            {0,2},{0,3},{1,2},{1,3},     // cara XY
            {0,4},{0,5},{1,4},{1,5},     // cara XZ
            {2,4},{2,5},{3,4},{3,5},     // cara YZ
        },
        {}, {}, {}  // tris, uvs, colors vacíos
    };
}