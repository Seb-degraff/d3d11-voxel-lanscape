#pragma once
#include "types.h"
#include "GeometryBuilder.h"
#include <d3d11.h>

class Chunk
{
	public:
	static constexpr int sx = 8;
	static constexpr int sy = 8;
	static constexpr int sz = 32;

	int chunk_x_;
	int chunk_y_;

	uint8_t cells[sx * sy * sz];

	ID3D11Buffer* vbuffer_;
	ID3D11Buffer* ibuffer_;
	GeometryBuilder builder_;

	Chunk(int chunk_x, int chunk_y);
	void SetCellLocal(int x, int y, int z, uint8_t val);
	uint8_t GetCellLocal(int x, int y, int z);
	void UpdateGeometryBuffers(ID3D11Device* device);
	void Render(ID3D11DeviceContext* context);
};
