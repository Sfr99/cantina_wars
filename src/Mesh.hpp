#pragma once
#include "Math3D.hpp"
#include <SDL2/SDL.h>
#include <vector>

struct Edge { int a, b; };
struct Triangle { int a, b, c; };

struct Mesh {
    std::vector<Vec3>      verts;
    std::vector<Edge>      edges;      // para wireframe
    std::vector<Triangle>  tris;       // para relleno
    std::vector<Vec3>      uvs;        // coordenadas UV (x,y), z no usado
    std::vector<SDL_Color> colors;     
};