#pragma once
#include "types.h"

class Chunk
{
	public:
	static constexpr int sx = 8;
	static constexpr int sy = 8;
	static constexpr int sz = 32;

	uint8_t cells[sx * sy * sz];

	void SetCellLocal(int x, int y, int z, uint8_t val);
	uint8_t GetCellLocal(int x, int y, int z);
};
