// #define WORLD_DISABLE_BALL_RANDOMNESS

struct PongWorld {
    Vec2 ball_move_dir;
    u32 score;
    f32 game_speed_coeff;
};

struct PongWorldConfig {
    Vec2 pad_size;
    f32 distance_from_center;
    f32 ball_speed;
    Vec2 area_extents;
    f32 pad_move_speed;
    f32 game_speed_increase_coeff;
};

struct PongWorldUpdateResult {
    bool is_game_over;
    bool did_score;
};

static bool pad_resolve_point(const GoData &pad_go, Vec2 p, int resolve_dir, f32 *out_resolved_x) {
    if (gameobject_is_point_in(pad_go, p)) {
        // 1 is right pad, -1 is left
        // TODO @ROBUSNESS: Assert here that resolve_dir is either -1 or 1
        Rect rect_world = gameobject_get_world_rect(pad_go);
        *out_resolved_x = resolve_dir == 1 ? rect_world.min.x : rect_world.max.x;
        return true;
    }

    return false;
}

static bool pad_ball_collision_check(const GoData &pad_go, Vec2 ball_displacement_from,
                                     Vec2 ball_displacement_to, Vec2 *out_collision_point) {
    Rect rect_world = gameobject_get_world_rect(pad_go);
    Vec2 edges[4] = {
        rect_world.min,
        Vec2(rect_world.min.x, rect_world.max.y),
        rect_world.max,
        Vec2(rect_world.max.x, rect_world.min.y),
    };

    for (int i = 0; i < 4; i++) {
        Vec2 intersection;
        bool has_intersection = check_line_segment_intersection(ball_displacement_from, ball_displacement_to,
                                                                edges[i], edges[(i + 1) % 4], &intersection);
        if (has_intersection) {
            *out_collision_point = intersection;
            return true;
        }
    }

    return false;
}

struct PongGame {

    PongWorld world;
    PongWorldConfig config;

    void init(Engine &engine) {
        engine.core.register_gameobject("field", Vec2::zero(), Vec2(((f32)WIDTH / (f32)HEIGHT) * 10, 10),
                                        "assets/Field.png", world_shader);
        // TODO @INCOMPLETE: ... and others
        // start from here: add ^these and move world_shader and ui_shader to engine
        // it's gonna end up being a singleton kind of thing, but we're ok with that
        // let's get things running first and we'll refactor later.

        world.ball_move_dir = Vec2(1.0f, 0.0f);
        world.score = 0;
        world.game_speed_coeff = 1.0f;

        const f32 cam_size = 5.0f; // TODO @CLEANUP: Duplicate
        config.area_extents = Vec2(cam_size * engine.render_info.aspect, cam_size);
        config.pad_size = Vec2(0.3f, 2.0f);
        config.ball_speed = 4.0f;
        config.distance_from_center = 4.0f;
        config.pad_move_speed = 10.0f;
        config.game_speed_increase_coeff = 0.05f;
    }

    void update(f32 dt, Engine &engine) {

        GoData &pad1_go = engine.core.get_go("pad1").data;
        GoData &pad2_go = engine.core.get_go("pad2").data;
        GoData &ball_go = engine.core.get_go("ball").data;

        PongWorldUpdateResult result;
        result.is_game_over = false;
        result.did_score = false;

        Rect pad1_world_rect = gameobject_get_world_rect(pad1_go);
        Rect pad2_world_rect = gameobject_get_world_rect(pad2_go);

        f32 pad_move_speed = config.pad_move_speed * world.game_speed_coeff * dt;

        if (engine.input.is_down(KeyCode::W) && pad2_world_rect.max.y < config.area_extents.y) {
            pad2_go.transform.translate_xy(Vec2(0.0f, pad_move_speed));
        } else if (engine.input.is_down(KeyCode::S) && pad2_world_rect.min.y > -config.area_extents.y) {
            pad2_go.transform.translate_xy(Vec2(0.0f, -pad_move_speed));
        }

        if (engine.input.is_down(KeyCode::Up) && pad1_world_rect.max.y < config.area_extents.y) {
            pad1_go.transform.translate_xy(Vec2(0.0f, pad_move_speed));
        } else if (engine.input.is_down(KeyCode::Down) && pad1_world_rect.min.y > -config.area_extents.y) {
            pad1_go.transform.translate_xy(Vec2(0.0f, -pad_move_speed));
        }

        //
        // Ball move
        //

        Vec2 ball_pos = ball_go.transform.get_pos_xy();
        Vec2 ball_displacement = world.ball_move_dir * (config.ball_speed * world.game_speed_coeff * dt);
        Vec2 ball_next_pos = ball_pos + ball_displacement;

        Vec2 collision_point;
        if (pad_ball_collision_check(pad1_go, ball_pos, ball_next_pos, &collision_point) ||
            pad_ball_collision_check(pad2_go, ball_pos, ball_next_pos, &collision_point)) {
            // Hit paddles

            ball_pos = collision_point; // Snap to hit position
            world.ball_move_dir.x *= -1;

            // randomness
#ifndef WORLD_DISABLE_BALL_RANDOMNESS
            const f32 ball_pad_hit_randomness_coeff = 0.2f;
            world.ball_move_dir.y += rand_range(-1.0f, 1.0f) * ball_pad_hit_randomness_coeff;
            world.ball_move_dir.normalize();
#endif

            ball_displacement = world.ball_move_dir * (config.ball_speed * dt);
            ball_next_pos = ball_pos + ball_displacement;

            (world.score)++;
            result.did_score = true;
            world.game_speed_coeff += config.game_speed_increase_coeff;

            sfx_play(&engine.sfx, SfxId::SfxHitPad);

            const ParticleProps &hit_particle_prop = collision_point.x > 0
                                                         ? engine.particle_prop_reg.pad_hit_right
                                                         : engine.particle_prop_reg.pad_hit_left;

            engine.core.register_particle(hit_particle_prop, engine.render_info, collision_point);
        }

        if (ball_next_pos.y > config.area_extents.y || ball_next_pos.y < -config.area_extents.y) {
            // Reflection from top/bottom
            world.ball_move_dir.y *= -1;

            ball_displacement = world.ball_move_dir * (config.ball_speed * dt);
            ball_next_pos = ball_pos + ball_displacement;

            sfx_play(&engine.sfx, SfxId::SfxHitWall);
        }

        result.is_game_over =
            ball_next_pos.x > config.area_extents.x || ball_next_pos.x < -config.area_extents.x;

        ball_go.transform.set_pos_xy(ball_next_pos);

        // return result; // TODO @INCOMPLETE: Change state on game over
    }
};