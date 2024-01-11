#include "types.h"
#include "GeometryBuilder.h"
#include "Map.h"
#include "Noise.h"
#include "Chunk.h"

namespace Map
{
    Chunk* chunks[max_chunks_x * max_chunks_y];
    std::vector<Chunk*> free_chunks;

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
                chunks[chunk_x + chunk_y * max_chunks_x] = new Chunk(chunk_x, chunk_y);
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
}

