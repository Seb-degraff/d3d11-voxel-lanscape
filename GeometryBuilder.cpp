#include "types.h"
#include "GeometryBuilder.h"

void GeometryBuilder::PushQuad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, vec3_t col, float col_alpha)
{
    int first_index = vert.size();
    vert.push_back({ { a.x, a.y, a.z }, { 0, 0 }, { col.x, col.y, col.z, col_alpha } });
    vert.push_back({ { b.x, b.y, b.z }, { 0, 1 }, { col.x, col.y, col.z, col_alpha } });
    vert.push_back({ { c.x, c.y, c.z }, { 1, 1 }, { col.x, col.y, col.z, col_alpha } });
    vert.push_back({ { d.x, d.y, d.z }, { 1, 0 }, { col.x, col.y, col.z, col_alpha } });

    ind.push_back(first_index + 0);
    ind.push_back(first_index + 1);
    ind.push_back(first_index + 2);
    ind.push_back(first_index + 0);
    ind.push_back(first_index + 2);
    ind.push_back(first_index + 3);
}


void GeometryBuilder::PushQuad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, vec3_t col)
{
    GeometryBuilder::PushQuad(a, b, c, d, col, 1.f);
}

