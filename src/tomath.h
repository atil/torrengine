#pragma once
#include "types.h"

#define DEG2RAD 0.0174533

struct Vec2 {
    float x, y;

    Vec2();
    explicit Vec2(float x, float y);

    void normalize();
    static Vec2 zero();
    static Vec2 one();

    Vec2 operator+(Vec2 b);
    Vec2 operator*(f32 s);
};

struct Mat4 {
    float data[16]; // Column major

    void translate_xy(Vec2 vec);
    Vec2 get_pos_xy() const;
    void set_pos_xy(Vec2 vec);
    void set_scale_xy(Vec2 vec);
    static Mat4 identity();
    static Mat4 ortho(float left, float right, float bottom, float top, float near, float far);
};

float lerp(float a, float b, float t);
float rand_range(float lo, float hi);
bool check_line_segment_intersection(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2 &intersection);