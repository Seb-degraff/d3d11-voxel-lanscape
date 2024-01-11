#pragma once

#include "GeometryBuilder.h"
#include "types.h"
#include "Chunk.h"
#include <vector>

namespace Map
{
    constexpr int max_chunks_x = 64;
    constexpr int max_chunks_y = 64;
    extern Chunk* chunks[max_chunks_x * max_chunks_y];
    extern std::vector<Chunk*> free_chunks;
    void Generate(GeometryBuilder* builder);
    void GenerateTerrain();
    uint8_t cell_at(int x, int y, int z);
}

