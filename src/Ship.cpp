#include "Ship.hpp"
#include <map>
#include <ostream>

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
    const float STRAFE_SPEED = 22.f;
    const float TILT_AMOUNT  = 0.35f;
    const float ACCEL        = 120.f;  // qué tan rápido acelera
    const float DECEL        = 180.f;  // qué tan rápido frena

    Vec3 input = {0, 0, 0};
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  input.x =  1.f;
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) input.x = -1.f;
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    input.y =  1.f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  input.y = -1.f;

    // X
    if (input.x != 0.f) {
        vel.x += input.x * ACCEL * dt;
        vel.x = std::max(-STRAFE_SPEED, std::min(STRAFE_SPEED, vel.x));
    } else {
        float dec = DECEL * dt;
        if (vel.x >  dec) vel.x -= dec;
        else if (vel.x < -dec) vel.x += dec;
        else vel.x = 0.f;
    }

    // Y
    if (input.y != 0.f) {
        vel.y += input.y * ACCEL * dt;
        vel.y = std::max(-STRAFE_SPEED, std::min(STRAFE_SPEED, vel.y));
    } else {
        float dec = DECEL * dt;
        if (vel.y >  dec) vel.y -= dec;
        else if (vel.y < -dec) vel.y += dec;
        else vel.y = 0.f;
    }

    vel.z = FORWARD_SPEED;

    // Inclinación proporcional a la velocidad actual
    rot.z = -(vel.x / STRAFE_SPEED) * TILT_AMOUNT;
    rot.x =  (vel.y / STRAFE_SPEED) * TILT_AMOUNT;

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

Mesh Ship::loadGLTFMesh(const char* binPath) {
    FILE* f = fopen(binPath, "rb");
    if (!f) {
        printf(" Cannot open %s — using fallback mesh\n", binPath);
        return createMesh();
    }
    fseek(f, 0, SEEK_END);
    size_t sz = (size_t)ftell(f);
    rewind(f);
    std::vector<uint8_t> bin(sz);
    fread(bin.data(), 1, sz, f);
    fclose(f);

    // Hardcoded from scene.gltf
    // bufferView 2 (positions, float VEC3 stride=12) starts at file byte 59736
    // bufferView 0 (indices,   uint32 SCALAR)        starts at file byte 0
    struct PrimDesc {
        size_t posOff; int posCount;
        size_t idxOff; int idxCount;
    };
    const size_t BV2 = 59736;
    PrimDesc prims[] = {
        {BV2+    0, 982,  0,     2274},   // body
        {BV2+23568, 106,  9096,  228 },   // cockpit
        {BV2+26112, 327,  10008, 708 },   // accent
        {BV2+33960, 1264, 12840, 2454},   // rubber
        {BV2+64296, 970,  22656, 1740},   // weapons
        {BV2+87576, 56,   29616, 120 },   // thruster
    };

    SDL_Color matColors[] = {
        {132,  26,   6, 255},   // Metal_body      — rojo
        { 13,  34,  40, 255},   // Cockpit         — azul oscuro
        { 66,  66,  66, 255},   // Metal_accent    — gris
        {  5,   5,   5, 255},   // Dull_metalrubber— negro
        { 63,  93, 114, 255},   // Weapons         — azul gris
        { 54, 255, 255, 255},   // Thruster        — cyan emisivo
    };

    std::vector<Vec3>      allVerts;
    std::vector<Triangle>  allTris;
    std::vector<SDL_Color> allColors;   // ← añadir

    const float SCALE = 0.22f;

    for (int pi = 0; pi < 6; pi++) {
        auto& p = prims[pi];
        const int base = (int)allVerts.size();

        const float* pos = (const float*)(bin.data() + p.posOff);
        for (int i = 0; i < p.posCount; i++) {
            allVerts.push_back({ pos[i*3]*SCALE, pos[i*3+1]*SCALE, pos[i*3+2]*SCALE });
            allColors.push_back(matColors[pi]);   // ← un color por vértice
        }

        const uint32_t* idx = (const uint32_t*)(bin.data() + p.idxOff);
        for (int i = 0; i < p.idxCount; i += 3)
            allTris.push_back({ (int)idx[i]+base, (int)idx[i+1]+base, (int)idx[i+2]+base });
    }

    // Derive edges from triangles (needed for wireframe fallback)
    std::map<std::pair<int,int>, bool> edgeSet;
    for (auto& t : allTris) {
        auto add = [&](int a, int b) {
            edgeSet[(a<b) ? std::make_pair(a,b) : std::make_pair(b,a)] = true;
        };
        add(t.a,t.b); add(t.b,t.c); add(t.c,t.a);
    }
    std::vector<Edge> edges;
    for (auto& [k,_] : edgeSet) edges.push_back({k.first, k.second});

    //printf(" Ship GLTF: %zu verts, %zu tris\n", allVerts.size(), allTris.size());
    return {allVerts, edges, allTris, {}, allColors};
}   