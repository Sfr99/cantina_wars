/*
 * rendering/GLTFLoader.hpp
 * Carga una malla desde el buffer binario (.bin) de un archivo GLTF.
 * Los offsets de primitivas están hardcodeados para scene.gltf del proyecto.
 */
#pragma once
#include "../core/Mesh.hpp"

namespace GLTFLoader {
    /* Carga la malla de la nave desde el .bin del GLTF; usa fallback si falla la lectura. */
    Mesh loadShipMesh(const char* binPath);
}