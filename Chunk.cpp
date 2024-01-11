#include "Chunk.h"
#include <assert.h>
#include "Map.h"

Chunk::Chunk(int chunk_x, int chunk_y)
{
    chunk_x_ = chunk_x;
    chunk_y_ = chunk_y;
}

void Chunk::SetCellLocal(int x, int y, int z, uint8_t val)
{
	int idx = x + y * Chunk::sx + z * Chunk::sx * Chunk::sy;
	assert(idx < (sizeof(cells) / sizeof(*cells)));
	cells[idx] = val;
}

uint8_t Chunk::GetCellLocal(int x, int y, int z)
{
	int idx = x + y * Chunk::sx + z * Chunk::sx * Chunk::sy;
	assert(idx < (sizeof(cells) / sizeof(*cells)));
	return cells[idx];
}

void Chunk::UpdateGeometryBuffers(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // Create the buffers if they don't exist yet
    if (vbuffer_ == nullptr)
    {
        constexpr int max_faces = 2048 * 4;
        {
            D3D11_BUFFER_DESC desc =
            {
                .ByteWidth = static_cast<UINT>(max_faces * 4 * sizeof(Vertex)),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                .CPUAccessFlags = D3D10_CPU_ACCESS_WRITE,
            };

            device->CreateBuffer(&desc, nullptr, &vbuffer_);
        }
        {
            D3D11_BUFFER_DESC desc =
            {
                .ByteWidth = static_cast<UINT>(max_faces * 6 * sizeof(builder_.ind[0])),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_INDEX_BUFFER,
                .CPUAccessFlags = D3D10_CPU_ACCESS_WRITE,
            };

            device->CreateBuffer(&desc, nullptr, &ibuffer_);
        }
    }

    static vec3_t grass_light = vec3(0.1f, 0.9f, 0.1f);
    static vec3_t grass_dark = vec3(0.05f, 0.8f, 0.05f);
    static vec3_t dirt_col = vec3(115.f / 256, 63.f / 256, 23.f / 256);
    static vec3_t stone_col = vec3(120.f / 256, 120.f / 256, 120.f / 256);
    static vec3_t water_col = vec3(100.f / 256, 110.f / 256, 220.f / 256);

    for (int z = 0; z < Chunk::sz; z++) {
        for (int y = 0; y < Chunk::sy; y++) {
            for (int x = 0; x < Chunk::sx; x++) {
                uint8_t cell_type = GetCellLocal(x, y, z);
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

                int wx = x + chunk_x_ * Chunk::sx;
                int wy = z;
                int wz = y + chunk_y_ * Chunk::sy;

                int cx = x + chunk_x_ * Chunk::sx;
                int cy = y + chunk_y_ * Chunk::sy;
                int cz = z;

                // x-
                if (Map::cell_at(cx - 1, cy, cz) == 0) {
                    builder_.PushQuad(vec3(wx, wy, wz), vec3(wx, wy, wz + 1), vec3(wx, wy + 1, wz + 1), vec3(wx, wy + 1, wz), vec3_scale(side_col, 0.6f));
                }
                // x+
                if (Map::cell_at(cx + 1, cy, cz) == 0) {
                    builder_.PushQuad(vec3(wx + 1, wy, wz), vec3(wx + 1, wy + 1, wz), vec3(wx + 1, wy + 1, wz + 1), vec3(wx + 1, wy, wz + 1), vec3_scale(side_col, 0.9f));
                }
                // y+
                if (Map::cell_at(cx, cy + 1, cz) == 0) {
                    builder_.PushQuad(vec3(wx, wy, wz + 1), vec3(wx + 1, wy, wz + 1), vec3(wx + 1, wy + 1, wz + 1), vec3(wx, wy + 1, wz + 1), side_col);
                }
                // y-
                if (Map::cell_at(cx, cy - 1, cz) == 0) {
                    builder_.PushQuad(vec3(wx, wy, wz), vec3(wx, wy + 1, wz), vec3(wx + 1, wy + 1, wz), vec3(wx + 1, wy, wz), vec3_scale(side_col, 0.5f));
                }
                // z+
                if (Map::cell_at(cx, cy, cz + 1) == 0) {
                    builder_.PushQuad(vec3(wx, wy + 1, wz), vec3(wx, wy + 1, wz + 1), vec3(wx + 1, wy + 1, wz + 1), vec3(wx + 1, wy + 1, wz), top_col);
                }
                // z-
                if (Map::cell_at(cx, cy, cz - 1) == 0) {
                    builder_.PushQuad(vec3(wx, wy, wz), vec3(wx + 1, wy, wz), vec3(wx + 1, wy, wz + 1), vec3(wx, wy, wz + 1), side_col);
                }
            }
        }
    }

    // Update dx11 buffers
    {
        // Update vertex buffer
        {
            D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
            context->Map(vbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
            memcpy(mapped_resource.pData, builder_.vert.data(), builder_.vert.size() * sizeof(builder_.vert[0]));
            context->Unmap(vbuffer_, 0);
        }

        // Update index buffer
        {
            D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
            context->Map(ibuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
            memcpy(mapped_resource.pData, builder_.ind.data(), builder_.ind.size() * sizeof(builder_.ind[0]));
            context->Unmap(ibuffer_, 0);
        }
    }
}

void Chunk::Render(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (vbuffer_ == nullptr) {
        UpdateGeometryBuffers(device, context);
    }

    UINT stride = sizeof(struct Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vbuffer_, &stride, &offset);
    context->IASetIndexBuffer(ibuffer_, DXGI_FORMAT_R32_UINT, 0);

    // draw
    context->DrawIndexed(builder_.ind.size(), 0, 0);
}
