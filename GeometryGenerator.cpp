#include "types.h"
#include "GeometryBuilder.h"
#include "GeometryGenerator.h"
#include "Noise.h"

namespace GeoGen
{
    constexpr int chunk_width = 32*4;
    constexpr int chunk_height = 32;

    uint8_t cells[chunk_width * chunk_width * chunk_height];

    uint8_t cell_at(int x, int y, int z)
    {
        if (x < 0 || x >= chunk_width || y < 0 || y >= chunk_height || z < 0 || z >= chunk_width) {
            return 0;
        }
        return cells[z * chunk_width * chunk_height + y * chunk_width + x];
    }

    void cell_set_at(int x, int y, int z, uint8_t val)
    {
        if (x < 0 || x >= chunk_width || y < 0 || y >= chunk_height || z < 0 || z >= chunk_width) {
            //LOG(L"Tried to set a tile out of bounds (%i, %i, %i).", x, y, z);
            return;
        }
        cells[z * chunk_width * chunk_height + y * chunk_width + x] = val;
    }

    void GenerateTerrain()
    {
        for (int z = 0; z < chunk_width; z++) {
            for (int y = 0; y < chunk_height; y++) {
                for (int x = 0; x < chunk_width; x++) {
                    float secondary_noise = +Noise::perlin2d(x, z, 0.11f, 1);
                    float perlin = Noise::perlin2d(x + 9000, z, 0.03f, 3) * chunk_height;
                    perlin -= 10;
                    perlin += secondary_noise * 4;
                    if (y <= perlin) {
                        if (y <= perlin - 1) {
                            cell_set_at(x, y, z, 2);
                        }
                        else {
                            cell_set_at(x, y, z, 1);
                        }
                    } else if (y <= 5) {
                        cell_set_at(x, y, z, 3);
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

        for (int z = 0; z < chunk_width; z++) {
            for (int y = 0; y < chunk_height; y++) {
                for (int x = 0; x < chunk_width; x++) {
                    uint8_t cell_type = cell_at(x, y, z);
                    if (cell_type == 0) {
                        continue;
                    }

                    vec3_t side_col = {};
                    vec3_t top_col = {};
                    top_col = (x & 1) == (z & 1) ? grass_light : grass_dark;
                    switch (cell_type) {
                        case 1: side_col = dirt_col; break;
                        case 2: side_col = stone_col; top_col = stone_col; break;
                        case 3: side_col = water_col; top_col = water_col; break;
                    }

                    // x-
                    if (cell_at(x - 1, y, z) == 0) {
                        builder->PushQuad(vec3(x, y, z), vec3(x, y, z + 1), vec3(x, y + 1, z + 1), vec3(x, y + 1, z), vec3_scale(side_col, 0.6f));
                    }
                    // x+
                    if (cell_at(x + 1, y, z) == 0) {
                        builder->PushQuad(vec3(x + 1, y, z), vec3(x + 1, y + 1, z), vec3(x + 1, y + 1, z + 1), vec3(x + 1, y, z + 1), vec3_scale(side_col, 0.9f));
                    }
                    // z+
                    if (cell_at(x, y, z + 1) == 0) {
                        builder->PushQuad(vec3(x, y, z + 1), vec3(x + 1, y, z + 1), vec3(x + 1, y + 1, z + 1), vec3(x, y + 1, z + 1), side_col);
                    }
                    // z-
                    if (cell_at(x, y, z - 1) == 0) {
                        builder->PushQuad(vec3(x, y, z), vec3(x, y + 1, z), vec3(x + 1, y + 1, z), vec3(x + 1, y, z), vec3_scale(side_col, 0.5f));
                    }
                    // y+
                    if (cell_at(x, y + 1, z) == 0) {
                        builder->PushQuad(vec3(x, y + 1, z), vec3(x, y + 1, z + 1), vec3(x + 1, y + 1, z + 1), vec3(x + 1, y + 1, z), top_col);
                    }
                    // y-
                    if (cell_at(x, y - 1, z) == 0) {
                        builder->PushQuad(vec3(x, y, z), vec3(x + 1, y, z), vec3(x + 1, y, z + 1), vec3(x, y, z + 1), side_col);
                    }
                }
            }
        }
    }
}

