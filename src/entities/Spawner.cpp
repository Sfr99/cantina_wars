#include "Spawner.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace Spawner {

static float frand01() { return rand() / (float)RAND_MAX; }
static float frand(float a, float b) { return a + (b - a) * frand01(); }

/* Selecciona tamaño con distribución 60% small / 30% medium / 10% large. */
static AsteroidSize randomSize() {
    int r = rand() % 100;
    if (r < 60) return AsteroidSize::SMALL;
    if (r < 90) return AsteroidSize::MEDIUM;
    return AsteroidSize::LARGE;
}

/* Devuelve el radio aproximado de colisión para cada tamaño. */
static float radiusFor(AsteroidSize s) {
    switch (s) {
        case AsteroidSize::LARGE:  return 3.0f;
        case AsteroidSize::MEDIUM: return 2.0f;
        case AsteroidSize::SMALL:  return 1.2f;
    }
    return 1.2f;
}

void spawnAsteroids(std::vector<Asteroid>& asteroids,
                    Vec3 shipPos, RenderMode mode, int /*wave*/) {
    constexpr float MIN_SPAWN_Z_AHEAD = 180.f;
    constexpr float SPAWN_Z_RANGE     = 520.f;
    constexpr float R_MIN             = 10.f;
    constexpr float R_MAX             = 38.f;
    constexpr float SAFE_RADIUS_XY    = 5.0f;
    constexpr float SAFE_Z_NEAR       = 80.f;
    constexpr float SEP_MARGIN        = 1.0f;
    constexpr int   SPAWN_COUNT       = 2;

    for (int n = 0; n < SPAWN_COUNT; ++n) {
        Vec3 spawnPos{};
        bool placed = false;

        for (int tries = 0; tries < 40 && !placed; ++tries) {
            float spawnZ = shipPos.z + MIN_SPAWN_Z_AHEAD + frand(0.f, SPAWN_Z_RANGE);

            // Muestra uniforme en área de anillo
            float r   = std::sqrt(frand01() * (R_MAX*R_MAX - R_MIN*R_MIN) + R_MIN*R_MIN);
            float ang = frand(0.f, 6.28318530718f);
            spawnPos  = { r * std::cos(ang), r * std::sin(ang), spawnZ };

            // Zona segura inmediata alrededor de la nave
            Vec3  rel = spawnPos - shipPos;
            float xy2 = rel.x*rel.x + rel.y*rel.y;
            if (std::fabs(rel.z) < SAFE_Z_NEAR && xy2 < SAFE_RADIUS_XY*SAFE_RADIUS_XY)
                continue;

            AsteroidSize size     = randomSize();
            float        thisRad  = radiusFor(size);

            // Separación mínima respecto a asteroides existentes
            bool tooClose = false;
            for (const auto& a : asteroids) {
                if (!a.alive) continue;
                Vec3  d       = spawnPos - a.pos;
                float minDist = thisRad + a.radius + SEP_MARGIN;
                if (d.dot(d) < minDist * minDist) { tooClose = true; break; }
            }
            if (tooClose) continue;

            Vec3 vel = {
                ((rand() % 100) - 50) * 0.03f,
                ((rand() % 100) - 50) * 0.03f,
                -48.f - (rand() % 30)
            };

            asteroids.emplace_back(spawnPos, vel, size, mode, rand());
            placed = true;
        }

        // Fallback si el campo está saturado
        if (!placed) {
            Vec3 fallback = { R_MAX, 0.f, shipPos.z + MIN_SPAWN_Z_AHEAD + SPAWN_Z_RANGE };
            asteroids.emplace_back(fallback, Vec3{0.f, 0.f, -55.f},
                                   AsteroidSize::SMALL, mode, rand());
        }
    }
}

} // namespace Spawner