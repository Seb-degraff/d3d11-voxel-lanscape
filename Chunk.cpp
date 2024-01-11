#include "Chunk.h"
#include <assert.h>

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
