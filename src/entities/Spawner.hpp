/*
 * entities/Spawner.hpp
 * Lógica de generación de asteroides: distribución en anillo, separación
 * mínima entre instancias y selección de tamaño por probabilidad.
 */
#pragma once
#include <vector>
#include "Asteroid.hpp"
#include "../core/Math3D.hpp"

namespace Spawner {
    /* Intenta añadir hasta 2 asteroides delante de shipPos con separación mínima garantizada.
       Usa un fallback si el espacio está saturado tras 40 intentos. */
    void spawnAsteroids(std::vector<Asteroid>& asteroids,
                        Vec3 shipPos, RenderMode mode, int wave);
}