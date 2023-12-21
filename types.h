#pragma once
#include <vector>

struct vec2_t vec2(float x, float y);

struct vec2_t
{
    float x;
    float y;

    inline vec2_t operator+(const vec2_t& other)
    {
        return vec2(x + other.x, y + other.y);
    }
};

inline vec2_t vec2(float x, float y)
{
    vec2_t vec;

    vec.x = x;
    vec.y = y;

    return vec;
}

struct vec3_t vec3(float x, float y, float z);

struct vec3_t
{
    float x;
    float y;
    float z;

    inline vec3_t operator+(const vec3_t& other)
    {
        return vec3(x + other.x, y + other.y, z + other.z);
    }
};

inline vec3_t vec3(float x, float y, float z)
{
    vec3_t vec;

    vec.x = x;
    vec.y = y;
    vec.z = z;

    return vec;
}

inline vec3_t vec3_scale(vec3_t vec, float scale)
{
    return vec3(vec.x * scale, vec.y * scale, vec.z * scale);
}

struct Vertex {
    float position[3];
    float uv[2];
    float color[3];
};

