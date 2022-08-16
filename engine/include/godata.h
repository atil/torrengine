#include "tomath.h"

struct Rect {
    Vec2 min;
    Vec2 max;

    explicit Rect(Vec2 center, Vec2 size);
};

struct GoData {
    Rect rect;
    Mat4 transform;

    explicit GoData(Vec2 pos, Vec2 size);

    Rect get_world_rect() const;
    bool is_point_in(Vec2 p) const;
};
