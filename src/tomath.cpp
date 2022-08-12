#pragma warning(push, 0) // TODO @CLEANUP: Introduce a macro for these pragmas
#include <cmath>
#include <cstring>
#include "tomath.h"
#pragma warning(pop)
#pragma warning(disable : 5045) // Spectre thing

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float rand_range(float lo, float hi) {
    // Assign to a variable first to avoid the clang's cast warning
    // It's fine, float holds more than int
    int random_int = rand();
    float zero_one = (float)random_int / (float)(RAND_MAX);
    return lerp(lo, hi, zero_one);
}

Vec2::Vec2() : x(0), y(0) {
}

Vec2::Vec2(float x, float y) : x(x), y(y) {
}

void Vec2::normalize() {
    float len = sqrtf(x * x + y * y);
    x = x / len;
    y = y / len;
}

Vec2 Vec2::zero() {
    return Vec2(0.0f, 0.0f);
}

Vec2 Vec2::one() {
    return Vec2(1.0f, 1.0f);
}

Vec2 Vec2::operator+(Vec2 b) {
    return Vec2(x + b.x, y + b.y);
}

Vec2 Vec2::operator*(f32 s) {
    return Vec2(x * s, y * s);
}

void Mat4::translate_xy(Vec2 vec) {
    data[12] += vec.x;
    data[13] += vec.y;
}

Vec2 Mat4::get_pos_xy() const {
    return Vec2(data[12], data[13]);
}

void Mat4::set_pos_xy(Vec2 vec) {
    data[12] = vec.x;
    data[13] = vec.y;
}

void Mat4::set_scale_xy(Vec2 vec) {
    data[0] *= vec.x;
    data[5] *= vec.y;
}

Mat4 Mat4::identity() {
    Mat4 identity;
    float identity_data[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // TODO @LEAK: Check if this is leaked
    memcpy(&identity.data, identity_data, sizeof(identity_data));
    return identity;
}

Mat4 Mat4::ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4 m = Mat4::identity();

    m.data[0 * 4 + 0] = 2 / (right - left);
    m.data[1 * 4 + 1] = 2 / (top - bottom);
    m.data[2 * 4 + 2] = -2 / (far - near);
    m.data[3 * 4 + 0] = -(right + left) / (right - left);
    m.data[3 * 4 + 1] = -(top + bottom) / (top - bottom);
    m.data[3 * 4 + 2] = -(far + near) / (far - near);

    return m;
}

bool check_line_segment_intersection(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2 &intersection) {
    f32 x1 = p1.x;
    f32 x2 = p2.x;
    f32 x3 = p3.x;
    f32 x4 = p4.x;
    f32 y1 = p1.y;
    f32 y2 = p2.y;
    f32 y3 = p3.y;
    f32 y4 = p4.y;

    // https://en.wikipedia.org/wiki/Line-line_intersection
    f32 t_nom = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
    f32 t_den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    f32 u_nom = (x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2);
    f32 u_den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    if (t_nom < 0 || t_nom > t_den || u_nom < 0 || u_nom > u_den) {
        return false; // Not intersecting
    }

    if (fabs(t_den) < 0.00001f || fabs(u_den) < 0.00001f) {
        return false; // Overlapping
    }

    f32 t = t_nom / t_den;
    intersection = Vec2(x1 + t * (x2 - x1), y1 + t * (y2 - y1));

    return true;
}
