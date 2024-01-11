#pragma once

#include "GeometryBuilder.h"
#include "types.h"
#include "Chunk.h"

namespace GeoGen
{
    constexpr int max_chunks_x = 16;
    constexpr int max_chunks_y = 16;
    extern Chunk* chunks[max_chunks_x * max_chunks_y];
    void Generate(GeometryBuilder* builder);
    void GenerateTerrain();
    uint8_t cell_at(int x, int y, int z);
}

