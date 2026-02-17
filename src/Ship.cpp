#include "Ship.hpp"

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
            {0,1},{0,2},{1,2},        // alas
            {0,3},{3,1},{3,2},        // aleta sup
            {0,4},{4,1},{4,2},        // base inf
            {1,5},{2,6},{5,6},        // cabina
            {3,5},{3,6},              // aleta a cabina
        },
        {}, {}, {}  // tris, uvs, colors vacíos
    };
}

void Ship::rotate(Vec3 axis, float angle) {
    auto rodrigues = [](Vec3 v, Vec3 ax, float ang) -> Vec3 {
        float c = cosf(ang), s = sinf(ang);
        return v * c + ax.cross(v) * s + ax * (ax.dot(v) * (1.f - c));
    };
    fwd = rodrigues(fwd, axis, angle).normalized();
    up  = rodrigues(up,  axis, angle).normalized();
    rgt = rodrigues(rgt, axis, angle).normalized();
}

void Ship::handleInput(const Uint8* keys, float dt) {
    // Solo rotación suave lateral (sin roll completo)
    const float STRAFE_SPEED = 22.f;
    const float TILT_AMOUNT = 0.35f;
    
    Vec3 movement = {0, 0, 0};
    
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  movement.x += 1.f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) movement.x -= 1.f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    movement.y += 1.f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  movement.y -= 1.f;
    
    // Aplicar movimiento lateral
    vel.x = movement.x * STRAFE_SPEED;
    vel.y = movement.y * STRAFE_SPEED;
    vel.z = FORWARD_SPEED;  // velocidad constante hacia adelante
    
    // Inclinación visual según movimiento lateral
    rot.z = -movement.x * TILT_AMOUNT;
    rot.x = movement.y * TILT_AMOUNT;
    
    // Disparo
    shootCooldown -= dt;
    wantsShoot = keys[SDL_SCANCODE_F] && (shootCooldown <= 0.f);
    if (wantsShoot) shootCooldown = SHOOT_DELAY;
}

void Ship::update(float dt) {
    Vec3 v = vel;
    v.z *= speedMult;   // solo forward
    pos += v * dt;

    
    // Límites laterales
    const float LIMIT_X = 32.f;
    const float LIMIT_Y = 20.f;
    
    if (pos.x < -LIMIT_X) pos.x = -LIMIT_X;
    if (pos.x >  LIMIT_X) pos.x =  LIMIT_X;
    if (pos.y < -LIMIT_Y) pos.y = -LIMIT_Y;
    if (pos.y >  LIMIT_Y) pos.y =  LIMIT_Y;
    
    if (invincible) {
        invincibleTimer -= dt;
        if (invincibleTimer <= 0.f) invincible = false;
    }
}

Mat4 Ship::worldTransform() const {
    return Mat4::translation(pos.x, pos.y, pos.z)
         * Mat4::rotationY(rot.y)
         * Mat4::rotationX(rot.x)
         * Mat4::rotationZ(rot.z)
         * Mat4::scale(scale);
}

void Ship::respawn() {
    pos = {0.f, 0.f, 0.f};
    vel = {0.f, 0.f, 30.f};  // velocidad constante forward
    rot = {0.f, 0.f, 0.f};
    fwd = {0.f, 0.f, 1.f};
    up  = {0.f, 1.f, 0.f};
    rgt = {1.f, 0.f, 0.f};
    invincible      = true;
    invincibleTimer = 3.f;
    alive = true;
}