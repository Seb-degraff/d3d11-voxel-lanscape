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

    inline vec3_t operator-(const vec3_t& other)
    {
        return vec3(x - other.x, y - other.y, z - other.z);
    }

    inline vec3_t operator*(const vec3_t & other)
    {
        return vec3(x * other.x, y * other.y, z * other.z);
    }

    inline vec3_t operator*(const float& other)
    {
        return vec3(x * other, y * other, z * other);
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

inline vec3_t vec3_dot(vec3_t a, vec3_t b)
{
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.z - a.y * b.x
    );
}

struct Vertex {
    float position[3];
    float uv[2];
    float color[4];
};

