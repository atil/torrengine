
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
} PongGame;

typedef struct
{
    Vec2 pad_size;
    float distance_from_center;
    float ball_speed;
    float area_half_height;
    float pad_move_speed;
} PongGameConfig;

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

static void game_init(PongGame *game, PongGameConfig *config)
{
    game->pad1_go = gameobject_new(vec2_new(config->distance_from_center, 0.0f), config->pad_size);
    game->pad2_go = gameobject_new(vec2_new(-(config->distance_from_center), 0.0f), config->pad_size);
    game->ball_go = gameobject_new(vec2_zero(), vec2_scale(vec2_one(), 0.2f));
    game->ball_move_dir = vec2_new(1.0f, 0.0f);
}

// TODO @CLEANUP: Remove GLFW dependency from here
static void game_update(float dt, PongGame *game, PongGameConfig *config, GLFWwindow *window)
{
    Rect pad1_world_rect = gameobject_get_world_rect(&(game->pad1_go));
    Rect pad2_world_rect = gameobject_get_world_rect(&(game->pad2_go));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && pad2_world_rect.max.y < config->area_half_height)
    {
        mat4_translate_xy(&game->pad2_go.transform, vec2_new(0.0f, config->pad_move_speed * dt));
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && pad2_world_rect.min.y > -config->area_half_height)
    {
        mat4_translate_xy(&game->pad2_go.transform, vec2_new(0.0f, -config->pad_move_speed * dt));
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && pad1_world_rect.max.y < config->area_half_height)
    {
        mat4_translate_xy(&game->pad1_go.transform, vec2_new(0.0f, config->pad_move_speed * dt));
    }
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && pad1_world_rect.min.y > -config->area_half_height)
    {
        mat4_translate_xy(&game->pad1_go.transform, vec2_new(0.0f, -config->pad_move_speed * dt));
    }

    //
    // Ball move
    //

    Vec2 ball_pos = mat4_get_pos_xy(&game->ball_go.transform);
    Vec2 ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
    Vec2 ball_next_pos = vec2_add(ball_pos, ball_displacement);

    if (gameobject_is_point_in(&game->pad1_go, ball_next_pos) || gameobject_is_point_in(&game->pad2_go, ball_next_pos))
    {
        // Hit paddles
        game->ball_move_dir.x *= -1;
        const float ball_pad_hit_randomness_coeff = 0.2f;
        game->ball_move_dir.y += rand_range(1.0f, 1.0f) * ball_pad_hit_randomness_coeff;
        vec2_normalize(&game->ball_move_dir);

        ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
        ball_next_pos = vec2_add(ball_pos, ball_displacement);
    }

    if (ball_next_pos.y > config->area_half_height || ball_next_pos.y < -config->area_half_height)
    {
        // Reflection from top/bottom
        game->ball_move_dir.y *= -1;

        ball_displacement = vec2_scale(game->ball_move_dir, (config->ball_speed * dt));
        ball_next_pos = vec2_add(ball_pos, ball_displacement);
    }

    mat4_set_pos_xy(&game->ball_go.transform, ball_next_pos);
}