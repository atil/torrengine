
typedef struct
{
    Vec2 min;
    Vec2 max;
} Rect;

typedef struct
{
    Rect rect;
    Mat4 transform;
} GameObject;

typedef struct
{
    GameObject pad1_go;
    GameObject pad2_go;
    GameObject ball_go;
    Vec2 ball_move_dir;
    uint32_t score;
    float game_speed_coeff;
} PongGame;

typedef struct
{
    Vec2 pad_size;
    float distance_from_center;
    float ball_speed;
    Vec2 area_extents;
    float pad_move_speed;
    float game_speed_increase_coeff;
} PongGameConfig;

typedef struct
{
    bool is_game_over;
    bool did_score;
} PongGameUpdateResult;

static Rect rect_new(Vec2 center, Vec2 size)
{
    float x_min = center.x - size.x / 2.0f;
    float x_max = center.x + size.x / 2.0f;
    float y_min = center.y - size.y / 2.0f;
    float y_max = center.y + size.y / 2.0f;

    Rect r;
    r.min = vec2_new(x_min, y_min);
    r.max = vec2_new(x_max, y_max);
    return r;
}

static GameObject gameobject_new(Vec2 pos, Vec2 size)
{
    GameObject go;
    go.rect = rect_new(vec2_zero(), size);
    Mat4 transform = mat4_identity();
    mat4_translate_xy(&transform, pos);
    mat4_set_scale_xy(&transform, size);
    go.transform = transform;

    return go;
}

static Rect gameobject_get_world_rect(const GameObject *go)
{
    Vec2 go_pos = mat4_get_pos_xy(&(go->transform));
    Rect rect_world = go->rect;
    rect_world.min = vec2_add(rect_world.min, go_pos);
    rect_world.max = vec2_add(rect_world.max, go_pos);

    return rect_world;
}

static bool gameobject_is_point_in(const GameObject *go, Vec2 p)
{
    Rect rect_world = gameobject_get_world_rect(go);
    return p.x > rect_world.min.x && p.x < rect_world.max.x && p.y > rect_world.min.y && p.y < rect_world.max.y;
}

static bool pad_resolve_point(const GameObject *pad_go, Vec2 p, int resolve_dir, float *out_resolved_x)
{
    if (gameobject_is_point_in(pad_go, p))
    {
        // 1 is right pad, -1 is left
        // TODO @ROBUSNESS: Assert here that resolve_dir is either -1 or 1
        Rect rect_world = gameobject_get_world_rect(pad_go);
        *out_resolved_x = resolve_dir == 1 ? rect_world.min.x : rect_world.max.x;
        return true;
    }

    return false;
}

static void game_init(PongGame *game, PongGameConfig *config)
{
    game->pad1_go = gameobject_new(vec2_new(config->distance_from_center, 0.0f), config->pad_size);
    game->pad2_go = gameobject_new(vec2_new(-(config->distance_from_center), 0.0f), config->pad_size);
    game->ball_go = gameobject_new(vec2_zero(), vec2_scale(vec2_one(), 0.2f));
    game->ball_move_dir = vec2_new(1.0f, 0.0f);
    game->score = 0;
    game->game_speed_coeff = 1.0f;
}

// TODO @CLEANUP: Signature looks ugly
static PongGameUpdateResult game_update(float dt, PongGame *game, const PongGameConfig *config, GLFWwindow *window,
                                        Sfx *sfx)
{
    PongGameUpdateResult result;
    result.is_game_over = false;
    result.did_score = false;

    Rect pad1_world_rect = gameobject_get_world_rect(&(game->pad1_go));
    Rect pad2_world_rect = gameobject_get_world_rect(&(game->pad2_go));

    float pad_move_speed = config->pad_move_speed * game->game_speed_coeff * dt;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && pad2_world_rect.max.y < config->area_extents.y)
    {
        mat4_translate_xy(&game->pad2_go.transform, vec2_new(0.0f, pad_move_speed));
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && pad2_world_rect.min.y > -config->area_extents.y)
    {
        mat4_translate_xy(&game->pad2_go.transform, vec2_new(0.0f, -pad_move_speed));
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && pad1_world_rect.max.y < config->area_extents.y)
    {
        mat4_translate_xy(&game->pad1_go.transform, vec2_new(0.0f, pad_move_speed));
    }
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && pad1_world_rect.min.y > -config->area_extents.y)
    {
        mat4_translate_xy(&game->pad1_go.transform, vec2_new(0.0f, -pad_move_speed));
    }

    //
    // Ball move
    //

    Vec2 ball_pos = mat4_get_pos_xy(&game->ball_go.transform);
    Vec2 ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * game->game_speed_coeff * dt));
    Vec2 ball_next_pos = vec2_add(ball_pos, ball_displacement);

    float resolved_x;
    if (pad_resolve_point(&game->pad1_go, ball_next_pos, 1, &resolved_x) ||
        pad_resolve_point(&game->pad2_go, ball_next_pos, -1, &resolved_x))
    {
        // Hit paddles

        ball_pos.x = resolved_x; // TODO @ROBUSTNESS: This is funky
        game->ball_move_dir.x *= -1;

        const float ball_pad_hit_randomness_coeff = 0.2f;
        game->ball_move_dir.y += rand_range(-1.0f, 1.0f) * ball_pad_hit_randomness_coeff;
        vec2_normalize(&game->ball_move_dir);

        ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
        ball_next_pos = vec2_add(ball_pos, ball_displacement);

        (game->score)++;
        result.did_score = true;
        game->game_speed_coeff += config->game_speed_increase_coeff;

        sfx_play(sfx);
    }

    if (ball_next_pos.y > config->area_extents.y || ball_next_pos.y < -config->area_extents.y)
    {
        // Reflection from top/bottom
        game->ball_move_dir.y *= -1;

        ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
        ball_next_pos = vec2_add(ball_pos, ball_displacement);
    }

    result.is_game_over = ball_next_pos.x > config->area_extents.x || ball_next_pos.x < -config->area_extents.x;

    mat4_set_pos_xy(&game->ball_go.transform, ball_next_pos);

    return result;
}
