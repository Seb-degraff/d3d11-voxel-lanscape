#include "types.h"
#include "GeometryBuilder.h"

void GeometryBuilder::PushQuad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, vec3_t col)
{
    int first_index = vert.size();
    vert.push_back({ { a.x, a.y, a.z }, { 0, 0 }, { col.x, col.y, col.z } });
    vert.push_back({ { b.x, b.y, b.z }, { 0, 2 }, { col.x, col.y, col.z } });
    vert.push_back({ { c.x, c.y, c.z }, { 2, 2 }, { col.x, col.y, col.z } });
    vert.push_back({ { d.x, d.y, d.z }, { 2, 0 }, { col.x, col.y, col.z } });

    ind.push_back(first_index + 0);
    ind.push_back(first_index + 1);
    ind.push_back(first_index + 2);
    ind.push_back(first_index + 0);
    ind.push_back(first_index + 2);
    ind.push_back(first_index + 3);
}
