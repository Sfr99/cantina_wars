/*
 * Mesh.hpp
 * Estructuras de datos geométricos: aristas para wireframe, triángulos para
 * relleno, UVs y colores por vértice.
 */
#pragma once
#include "Math3D.hpp"
#include <SDL2/SDL.h>
#include <vector>

struct Edge     { int a, b; };
struct Triangle { int a, b, c; };

struct Mesh {
    std::vector<Vec3>      verts;
    std::vector<Edge>      edges;
    std::vector<Triangle>  tris;
    std::vector<Vec3>      uvs;     // coordenadas UV; componente z no utilizada
    std::vector<SDL_Color> colors;
};