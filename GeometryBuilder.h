#pragma once

#include <vector>
#include "types.h"

struct GeometryBuilder {
    std::vector<Vertex> vert;
    std::vector<uint32_t> ind;

    void PushQuad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, vec3_t col);
    void PushQuad(vec3_t a, vec3_t b, vec3_t c, vec3_t d, vec3_t col, float col_alpha);
};

