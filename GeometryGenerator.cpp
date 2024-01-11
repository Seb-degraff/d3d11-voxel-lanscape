#include "types.h"
#include "GeometryBuilder.h"
#include "GeometryGenerator.h"
#include "Noise.h"
#include "Chunk.h"

namespace GeoGen
{
    constexpr int max_chunks_x = 16;
    constexpr int max_chunks_y = 16;

    Chunk* chunks[max_chunks_x * max_chunks_y];

    uint8_t cell_at(int x, int y, int z)
    {
        int chunk_x = x / Chunk::sx;
        int chunk_y = y / Chunk::sy;

        if (chunk_x < 0 || chunk_x >= max_chunks_x || chunk_y < 0 || chunk_y >= max_chunks_y || z < 0 || z >= Chunk::sz) {
            return 0;
        }

        int cx = x % Chunk::sx;
        int cy = y % Chunk::sy;
        int cz = z;

        // This could be an assert. Or could be removed, it's litterally imposible to be true.
        if (cx >= Chunk::sx || cy >= Chunk::sy || cy >= Chunk::sy) {
            exit(-1);
        }

        Chunk* chunk = chunks[chunk_x + chunk_y * max_chunks_x];
        return chunk->cells[cx + cy * Chunk::sx + cz * Chunk::sx * Chunk::sy];
    }

    //void cell_set_at(int x, int y, int z, uint8_t val)
    //{
    //    if (x < 0 || x >= chunk_width || y < 0 || y >= chunk_height || z < 0 || z >= chunk_width) {
    //        //LOG(L"Tried to set a tile out of bounds (%i, %i, %i).", x, y, z);
    //        return;
    //    }
    //    cells[z * chunk_width * chunk_height + y * chunk_width + x] = val;
    //}

    void GenerateTerrain()
    {
        for (int chunk_y = 0; chunk_y < max_chunks_y; chunk_y++) {
            for (int chunk_x = 0; chunk_x < max_chunks_x; chunk_x++) {
                chunks[chunk_x + chunk_y * max_chunks_x] = new Chunk();
            }
        }

        for (int chunk_y = 0; chunk_y < max_chunks_y; chunk_y++) {
            for (int chunk_x = 0; chunk_x < max_chunks_x; chunk_x++) {
                Chunk* chunk = chunks[chunk_x + chunk_y * max_chunks_x];
                // iterate the chunk's cells
                for (int z = 0; z < Chunk::sz; z++) {
                    for (int y = 0; y < Chunk::sy; y++) {
                        for (int x = 0; x < Chunk::sx; x++) {
                            int global_x = chunk_x * Chunk::sx + x;
                            int global_y = chunk_y * Chunk::sy + y;

                            float secondary_noise = +Noise::perlin2d(global_x, global_y, 0.11f, 1);
                            float perlin = Noise::perlin2d(global_x, global_y, 0.03f, 3) * Chunk::sz;
                            perlin -= 10;
                            perlin += secondary_noise * 4;
                            if (z <= perlin) {
                                if (z <= perlin - 1) {
                                    chunk->SetCellLocal(x, y, z, 2);
                                }
                                else {
                                    chunk->SetCellLocal(x, y, z, 1);
                                }
                            } else if (z <= 5) {
                                chunk->SetCellLocal(x, y, z, 3);
                            }
                        }
                    }
                }
                
            }
        }
    }

    void Generate(GeometryBuilder* builder)
    {
        vec3_t grass_light = vec3(0.1f, 0.9f, 0.1f);
        vec3_t grass_dark = vec3(0.05f, 0.8f, 0.05f);
        vec3_t dirt_col = vec3(115.f / 256, 63.f / 256, 23.f / 256);
        vec3_t stone_col = vec3(120.f / 256, 120.f / 256, 120.f / 256);
        vec3_t water_col = vec3(100.f / 256, 110.f / 256, 220.f / 256);

        for (int chunk_y = 0; chunk_y < max_chunks_y; chunk_y++) {
            for (int chunk_x = 0; chunk_x < max_chunks_x; chunk_x++) {
                Chunk* chunk = chunks[chunk_x + chunk_y * max_chunks_x];
                for (int z = 0; z < Chunk::sz; z++) {
                    for (int y = 0; y < Chunk::sy; y++) {
                        for (int x = 0; x < Chunk::sx; x++) {
                            uint8_t cell_type = chunk->GetCellLocal(x, y, z);
                            if (cell_type == 0) {
                                continue;
                            }

                            vec3_t side_col = {};
                            vec3_t top_col = {};
                            top_col = (x & 1) == (y & 1) ? grass_light : grass_dark;
                            switch (cell_type) {
                            case 1: side_col = dirt_col; break;
                            case 2: side_col = stone_col; top_col = stone_col; break;
                            case 3: side_col = water_col; top_col = water_col; break;
                            }

                            // convert chunk cell coords to dx11 coords. Z up -> Y up, chunk local -> world.

                            int wx = x + chunk_x * Chunk::sx;
                            int wy = z;
                            int wz = y + chunk_y * Chunk::sy;

                            int cx = x + chunk_x * Chunk::sx;
                            int cy = y + chunk_y * Chunk::sy;
                            int cz = z;

                            // x-
                            if (cell_at(cx - 1, cy, cz) == 0) {
                                builder->PushQuad(vec3(wx, wy, wz), vec3(wx, wy, wz + 1), vec3(wx, wy + 1, wz + 1), vec3(wx, wy + 1, wz), vec3_scale(side_col, 0.6f));
                            }
                            // x+
                            if (cell_at(cx + 1, cy, cz) == 0) {
                                builder->PushQuad(vec3(wx + 1, wy, wz), vec3(wx + 1, wy + 1, wz), vec3(wx + 1, wy + 1, wz + 1), vec3(wx + 1, wy, wz + 1), vec3_scale(side_col, 0.9f));
                            }
                            // y+
                            if (cell_at(cx, cy + 1, cz) == 0) {
                                builder->PushQuad(vec3(wx, wy, wz + 1), vec3(wx + 1, wy, wz + 1), vec3(wx + 1, wy + 1, wz + 1), vec3(wx, wy + 1, wz + 1), side_col);
                            }
                            // y-
                            if (cell_at(cx, cy - 1, cz) == 0) {
                                builder->PushQuad(vec3(wx, wy, wz), vec3(wx, wy + 1, wz), vec3(wx + 1, wy + 1, wz), vec3(wx + 1, wy, wz), vec3_scale(side_col, 0.5f));
                            }
                            // z+
                            if (cell_at(cx, cy, cz + 1) == 0) {
                                builder->PushQuad(vec3(wx, wy + 1, wz), vec3(wx, wy + 1, wz + 1), vec3(wx + 1, wy + 1, wz + 1), vec3(wx + 1, wy + 1, wz), top_col);
                            }
                            // z-
                            if (cell_at(cx, cy, cz - 1) == 0) {
                                builder->PushQuad(vec3(wx, wy, wz), vec3(wx + 1, wy, wz), vec3(wx + 1, wy, wz + 1), vec3(wx, wy, wz + 1), side_col);
                            }
                        }
                    }
                }
            }
        }
    }
}

