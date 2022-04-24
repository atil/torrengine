#include <math.h>
#define DEG2RAD 0.0174533

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static float rand_range(float lo, float hi) {
    // Assign to a variable first to avoid the clang's cast warning
    // It's fine, float holds more than int
    int random_int = rand();
    float zero_one = (float)random_int / (float)(RAND_MAX);
    return lerp(lo, hi, zero_one);
}

struct Vec2 {
    float x, y;
};

static Vec2 vec2_new(float x, float y) {
    Vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

Vec2 operator+(Vec2 a, Vec2 b) {
    return vec2_new(a.x + b.x, a.y + b.y);
}

static Vec2 vec2_zero(void) {
    return vec2_new(0.0f, 0.0f);
}

static Vec2 vec2_one(void) {
    return vec2_new(1.0f, 1.0f);
}

static Vec2 vec2_scale(Vec2 v, float s) {
    return vec2_new(v.x * s, v.y * s);
}

static Vec2 vec2_add(Vec2 v1, Vec2 v2) {
    return vec2_new(v1.x + v2.x, v1.y + v2.y);
}

static void vec2_normalize(Vec2 *vec) {
    // TODO @SPEED: Measure if moving the fields to local
    // variables has any effect
    float len = sqrtf(vec->x * vec->x + vec->y * vec->y);
    vec->x = vec->x / len;
    vec->y = vec->y / len;
}

typedef struct {
    float data[16];
} Mat4;

static Mat4 mat4_identity(void) {
    Mat4 identity;
    float identity_data[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // TODO @LEAK: Check if this is leaked
    memcpy(&identity.data, identity_data, sizeof(identity_data));
    return identity;
}

static Mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4 m = mat4_identity();

    m.data[0 * 4 + 0] = 2 / (right - left);
    m.data[1 * 4 + 1] = 2 / (top - bottom);
    m.data[2 * 4 + 2] = -2 / (far - near);
    m.data[3 * 4 + 0] = -(right + left) / (right - left);
    m.data[3 * 4 + 1] = -(top + bottom) / (top - bottom);
    m.data[3 * 4 + 2] = -(far + near) / (far - near);

    return m;
}

static void mat4_translate_xy(Mat4 *mat, Vec2 vec) {
    mat->data[12] += vec.x; // Column major
    mat->data[13] += vec.y;
}

static Vec2 mat4_get_pos_xy(const Mat4 *mat) {
    return vec2_new(mat->data[12], mat->data[13]);
}

static void mat4_set_pos_xy(Mat4 *mat, Vec2 vec) {
    mat->data[12] = vec.x;
    mat->data[13] = vec.y;
}

static void mat4_set_scale_xy(Mat4 *mat, Vec2 vec) {
    mat->data[0] *= vec.x;
    mat->data[5] *= vec.y;
}

static bool check_line_segment_intersection(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2 *intersection) {
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
    *intersection = vec2_new(x1 + t * (x2 - x1), y1 + t * (y2 - y1));

    return true;
}
