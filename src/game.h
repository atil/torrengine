
struct Rect {
    Vec2 min;
    Vec2 max;
};

struct GameObject {
    Rect rect;
    Mat4 transform;
};

struct PongGame {
    EntityIndex field_ref;
    EntityIndex pad1_ref;
    EntityIndex pad2_ref;
    EntityIndex ball_ref;
    Vec2 ball_move_dir;
    u32 score;
    f32 game_speed_coeff;
};

struct PongGameConfig {
    Vec2 pad_size;
    f32 distance_from_center;
    f32 ball_speed;
    Vec2 area_extents;
    f32 pad_move_speed;
    f32 game_speed_increase_coeff;
};

struct PongGameUpdateResult {
    bool is_game_over;
    bool did_score;
};

static Rect rect_new(Vec2 center, Vec2 size) {
    f32 x_min = center.x - size.x / 2.0f;
    f32 x_max = center.x + size.x / 2.0f;
    f32 y_min = center.y - size.y / 2.0f;
    f32 y_max = center.y + size.y / 2.0f;

    Rect r;
    r.min = vec2_new(x_min, y_min);
    r.max = vec2_new(x_max, y_max);
    return r;
}

static GameObject gameobject_new(Vec2 pos, Vec2 size) {
    GameObject go;
    go.rect = rect_new(vec2_zero(), size);
    Mat4 transform = mat4_identity();
    mat4_translate_xy(&transform, pos);
    mat4_set_scale_xy(&transform, size);
    go.transform = transform;

    return go;
}

static Rect gameobject_get_world_rect(const GameObject *go) {
    Vec2 go_pos = mat4_get_pos_xy(&(go->transform));
    Rect rect_world = go->rect;
    rect_world.min = vec2_add(rect_world.min, go_pos);
    rect_world.max = vec2_add(rect_world.max, go_pos);

    return rect_world;
}

static bool gameobject_is_point_in(GameObject *go, Vec2 p) {
    Rect rect_world = gameobject_get_world_rect(go);
    return p.x > rect_world.min.x && p.x < rect_world.max.x && p.y > rect_world.min.y && p.y < rect_world.max.y;
}

static bool pad_resolve_point(GameObject *pad_go, Vec2 p, int resolve_dir, f32 *out_resolved_x) {
    if (gameobject_is_point_in(pad_go, p)) {
        // 1 is right pad, -1 is left
        // TODO @ROBUSNESS: Assert here that resolve_dir is either -1 or 1
        Rect rect_world = gameobject_get_world_rect(pad_go);
        *out_resolved_x = resolve_dir == 1 ? rect_world.min.x : rect_world.max.x;
        return true;
    }

    return false;
}

static bool pad_ball_collision_check(GameObject *pad_go, Vec2 ball_displacement_from, Vec2 ball_displacement_to,
                                     Vec2 *collision_point) {
    Rect rect_world = gameobject_get_world_rect(pad_go);
    Vec2 edges[4] = {
        rect_world.min,
        vec2_new(rect_world.min.x, rect_world.max.y),
        rect_world.max,
        vec2_new(rect_world.max.x, rect_world.min.y),
    };

    for (int i = 0; i < 4; i++) {
        Vec2 intersection;
        bool has_intersection = check_line_segment_intersection(ball_displacement_from, ball_displacement_to,
                                                                edges[i], edges[(i + 1) % 4], &intersection);
        if (has_intersection) {
            *collision_point = intersection;
            return true;
        }
    }

    return false;
}

static void game_init(PongGame *game, Sfx *sfx) {

    // TODO @CLEANUP: Hardcoded entity indices. These should be set by the entity creation system
    game->pad1_ref = 1;
    game->pad2_ref = 2;
    game->ball_ref = 3;

    game->ball_move_dir = vec2_new(1.0f, 0.0f);
    game->score = 0;
    game->game_speed_coeff = 1.0f;

    sfx_play(sfx, SfxId::SfxStart);
}

// TODO @CLEANUP: Signature looks ugly
static PongGameUpdateResult game_update(f32 dt, PongGame *game, std::vector<GameObject> &gos,
                                        PongGameConfig *config, GLFWwindow *window, Sfx *sfx,
                                        ParticlePropRegistry *particle_prop_reg,
                                        ParticleSystemRegistry *particle_system_reg, Renderer *renderer) {

    GameObject &pad1_go = gos[game->pad1_ref];
    GameObject &pad2_go = gos[game->pad2_ref];
    GameObject &ball_go = gos[game->ball_ref];

    PongGameUpdateResult result;
    result.is_game_over = false;
    result.did_score = false;

    Rect pad1_world_rect = gameobject_get_world_rect(&pad1_go);
    Rect pad2_world_rect = gameobject_get_world_rect(&pad2_go);

    f32 pad_move_speed = config->pad_move_speed * game->game_speed_coeff * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && pad2_world_rect.max.y < config->area_extents.y) {
        mat4_translate_xy(&pad2_go.transform, vec2_new(0.0f, pad_move_speed));
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && pad2_world_rect.min.y > -config->area_extents.y) {
        mat4_translate_xy(&pad2_go.transform, vec2_new(0.0f, -pad_move_speed));
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && pad1_world_rect.max.y < config->area_extents.y) {
        mat4_translate_xy(&pad1_go.transform, vec2_new(0.0f, pad_move_speed));
    } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS &&
               pad1_world_rect.min.y > -config->area_extents.y) {
        mat4_translate_xy(&pad1_go.transform, vec2_new(0.0f, -pad_move_speed));
    }

    //
    // Ball move
    //

    Vec2 ball_pos = mat4_get_pos_xy(&ball_go.transform);
    Vec2 ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * game->game_speed_coeff * dt));
    Vec2 ball_next_pos = vec2_add(ball_pos, ball_displacement);

    Vec2 collision_point;
    if (pad_ball_collision_check(&pad1_go, ball_pos, ball_next_pos, &collision_point) ||
        pad_ball_collision_check(&pad2_go, ball_pos, ball_next_pos, &collision_point)) {
        // Hit paddles

        ball_pos = collision_point; // Snap to hit position
        game->ball_move_dir.x *= -1;

        // randomness
        const f32 ball_pad_hit_randomness_coeff = 0.2f;
        game->ball_move_dir.y += rand_range(-1.0f, 1.0f) * ball_pad_hit_randomness_coeff;
        vec2_normalize(&game->ball_move_dir);

        ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
        ball_next_pos = vec2_add(ball_pos, ball_displacement);

        (game->score)++;
        result.did_score = true;
        game->game_speed_coeff += config->game_speed_increase_coeff;

        sfx_play(sfx, SfxId::SfxHitPad);

        ParticleProps *hit_particle_prop =
            collision_point.x > 0 ? &particle_prop_reg->pad_hit_right : &particle_prop_reg->pad_hit_left;

        ParticleSystem ps = particle_system_create(hit_particle_prop, renderer, collision_point);
        particle_system_registry_add(particle_system_reg, ps);
    }

    if (ball_next_pos.y > config->area_extents.y || ball_next_pos.y < -config->area_extents.y) {
        // Reflection from top/bottom
        game->ball_move_dir.y *= -1;

        ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
        ball_next_pos = vec2_add(ball_pos, ball_displacement);

        sfx_play(sfx, SfxId::SfxHitWall);
    }

    result.is_game_over = ball_next_pos.x > config->area_extents.x || ball_next_pos.x < -config->area_extents.x;

    mat4_set_pos_xy(&ball_go.transform, ball_next_pos);

    return result;
}
