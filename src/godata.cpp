#include "godata.h"

Rect::Rect(Vec2 center, Vec2 size) {
    f32 x_min = center.x - size.x / 2.0f;
    f32 x_max = center.x + size.x / 2.0f;
    f32 y_min = center.y - size.y / 2.0f;
    f32 y_max = center.y + size.y / 2.0f;

    min = Vec2(x_min, y_min);
    max = Vec2(x_max, y_max);
}

GoData::GoData(Vec2 pos, Vec2 size) : rect(Vec2::zero(), size) {
    transform = Mat4::identity();
    transform.translate_xy(pos);
    transform.set_scale_xy(size);
}

Rect GoData::get_world_rect() const {
    Vec2 go_pos = transform.get_pos_xy();
    Rect rect_world = rect;
    rect_world.min = rect_world.min + go_pos;
    rect_world.max = rect_world.max + go_pos;

    return rect_world;
}

bool GoData::is_point_in(Vec2 p) const {
    Rect rect_world = get_world_rect();
    return p.x > rect_world.min.x && p.x < rect_world.max.x && p.y > rect_world.min.y &&
           p.y < rect_world.max.y;
}