#include "GLTFLoader.hpp"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <map>

namespace GLTFLoader {

/*
 * Descripción de cada primitiva: offset en el buffer de posiciones,
 * número de vértices, offset en el buffer de índices y número de índices.
 * Valores extraídos de scene.gltf (bufferView 2 para posiciones, bufferView 0 para índices).
 */
struct PrimDesc {
    size_t posOff; int posCount;
    size_t idxOff; int idxCount;
};

/* Carga la malla completa de la nave desde el binario GLTF; retorna fallback vacío si falla. */
Mesh loadShipMesh(const char* binPath) {
    FILE* f = fopen(binPath, "rb");
    if (!f) return Mesh{};

    fseek(f, 0, SEEK_END);
    size_t sz = (size_t)ftell(f);
    rewind(f);
    std::vector<uint8_t> bin(sz);
    fread(bin.data(), 1, sz, f);
    fclose(f);

    // Offsets hardcodeados de scene.gltf: bufferView 2 (posiciones) empieza en byte 59736
    const size_t BV2 = 59736;
    const PrimDesc prims[] = {
        {BV2+     0,  982,      0, 2274},   // body
        {BV2+ 23568,  106,   9096,  228},   // cockpit
        {BV2+ 26112,  327,  10008,  708},   // accent
        {BV2+ 33960, 1264,  12840, 2454},   // rubber
        {BV2+ 64296,  970,  22656, 1740},   // weapons
        {BV2+ 87576,   56,  29616,  120},   // thruster
    };

    const SDL_Color matColors[] = {
        {132,  26,   6, 255},   // body     — rojo
        { 13,  34,  40, 255},   // cockpit  — azul oscuro
        { 66,  66,  66, 255},   // accent   — gris
        {  5,   5,   5, 255},   // rubber   — negro
        { 63,  93, 114, 255},   // weapons  — azul gris
        { 54, 255, 255, 255},   // thruster — cyan emisivo
    };

    std::vector<Vec3>      allVerts;
    std::vector<Triangle>  allTris;
    std::vector<SDL_Color> allColors;

    const float SCALE = 0.22f;

    for (int pi = 0; pi < 6; pi++) {
        const PrimDesc& p    = prims[pi];
        const int       base = (int)allVerts.size();

        const float* pos = (const float*)(bin.data() + p.posOff);
        for (int i = 0; i < p.posCount; i++) {
            allVerts.push_back({ pos[i*3]*SCALE, pos[i*3+1]*SCALE, pos[i*3+2]*SCALE });
            allColors.push_back(matColors[pi]);
        }

        const uint32_t* idx = (const uint32_t*)(bin.data() + p.idxOff);
        for (int i = 0; i < p.idxCount; i += 3)
            allTris.push_back({ (int)idx[i]+base, (int)idx[i+1]+base, (int)idx[i+2]+base });
    }

    // Derivar aristas únicas desde los triángulos (necesarias para el modo wireframe)
    std::map<std::pair<int,int>, bool> edgeSet;
    for (auto& t : allTris) {
        auto add = [&](int a, int b) {
            edgeSet[(a<b) ? std::make_pair(a,b) : std::make_pair(b,a)] = true;
        };
        add(t.a, t.b); add(t.b, t.c); add(t.c, t.a);
    }

    std::vector<Edge> edges;
    for (auto& [k, _] : edgeSet) edges.push_back({k.first, k.second});

    return {allVerts, edges, allTris, {}, allColors};
}

} // namespace GLTFLoader