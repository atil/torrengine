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
    bool did_score;
    bool is_gameover;
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
                                     Vec2 ball_displacement_to, Vec2 &out_collision_point) {
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
                                                                edges[i], edges[(i + 1) % 4], intersection);
        if (has_intersection) {
            out_collision_point = intersection;
            return true;
        }
    }

    return false;
}

struct PongGame : IGame {

    PongWorld world;
    PongWorldConfig config;

    void world_init() {
        world.ball_move_dir = Vec2(1.0f, 0.0f);
        world.score = 0;
        world.game_speed_coeff = 1.0f;
    }

    virtual ~PongGame() = default;

    virtual void init(Engine &engine) override {

        const f32 cam_size = 5.0f; // TODO @CLEANUP: Duplicate
        config.area_extents = Vec2(cam_size * engine.render_info.aspect, cam_size);
        config.pad_size = Vec2(0.3f, 2.0f);
        config.ball_speed = 4.0f;
        config.distance_from_center = 4.0f;
        config.pad_move_speed = 10.0f;
        config.game_speed_increase_coeff = 0.05f;

        world_init();

        // These binds need to be after init'ing this Pong object. It captures its state or something
        engine.register_state("splash_state", std::bind(&PongGame::update_splash_state, *this,
                                                        std::placeholders::_1, std::placeholders::_2));

        engine.register_state("game_state", std::bind(&PongGame::update_game_state, *this,
                                                      std::placeholders::_1, std::placeholders::_2));
        engine.register_state("intermission_state", std::bind(&PongGame::update_intermission_state, *this,
                                                              std::placeholders::_1, std::placeholders::_2));

        engine.register_ui_entity("splash", "splash_state", "TorrPong!\0",
                                  TextTransform(Vec2(-0.8f, 0), 0.5f, TextWidthType::FixedWidth, 1.6f));
        engine.register_ui_entity("score", "game_state", "0\0",
                                  TextTransform(Vec2(-0.9f, -0.9f), 0.3f, TextWidthType::FreeWidth, 0.1f));
        engine.register_ui_entity("intermission", "intermission_state", "Game Over\0",
                                  TextTransform(Vec2(-0.75f, 0.0f), 0.5f, TextWidthType::FixedWidth, 1.5f));

        engine.register_gameobject("field", "game_state", Vec2::zero(),
                                   Vec2(((f32)WIDTH / (f32)HEIGHT) * 10, 10), "assets/Field.png");
        engine.register_gameobject("pad1", "game_state", Vec2(config.distance_from_center, 0.0f),
                                   config.pad_size, "assets/PadBlue.png");
        engine.register_gameobject("pad2", "game_state", Vec2(-config.distance_from_center, 0.0f),
                                   config.pad_size, "assets/PadGreen.png");
        engine.register_gameobject("ball", "game_state", Vec2::zero(), Vec2::one() * 0.2f, "assets/Ball.png");
    }

    std::optional<std::string> update_splash_state(f32 dt, Engine &engine) {

        std::optional<std::string> next_state = std::nullopt;
        if (engine.input.just_pressed(KeyCode::Enter)) {

            world_init();
            engine.sfx_play(SfxId::SfxStart);

            next_state = "game_state";
        }

        return next_state;
    }

    std::optional<std::string> update_game_state(f32 dt, Engine &engine) {

        PongWorldUpdateResult result = update_world(dt, engine);

        std::optional<std::string> next_state = std::nullopt;

        if (result.is_gameover) {
            next_state = "intermission_state";
        }

        // UI draw
        Widget &swidget = engine.get_widget("score");
        if (result.did_score) {
            swidget.data.set_str(world.score);
            swidget.ru.update(swidget.data);
        }

        return next_state;
    }

    PongWorldUpdateResult update_world(f32 dt, Engine &engine) {

        PongWorldUpdateResult result;
        result.did_score = false;
        result.is_gameover = false;

        GoData &pad1_go = engine.get_go("pad1").data;
        GoData &pad2_go = engine.get_go("pad2").data;
        GoData &ball_go = engine.get_go("ball").data;

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
        if (pad_ball_collision_check(pad1_go, ball_pos, ball_next_pos, collision_point) ||
            pad_ball_collision_check(pad2_go, ball_pos, ball_next_pos, collision_point)) {

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

            world.score++;
            result.did_score = true;
            world.game_speed_coeff += config.game_speed_increase_coeff;

            engine.sfx_play(SfxId::SfxHitPad);

            const ParticleProps &hit_particle_prop = collision_point.x > 0
                                                         ? engine.particle_prop_reg.pad_hit_right
                                                         : engine.particle_prop_reg.pad_hit_left;

            engine.register_particle("game_state", hit_particle_prop, collision_point);
        }

        if (ball_next_pos.y > config.area_extents.y || ball_next_pos.y < -config.area_extents.y) {
            // Reflection from top/bottom
            world.ball_move_dir.y *= -1;

            ball_displacement = world.ball_move_dir * (config.ball_speed * dt);
            ball_next_pos = ball_pos + ball_displacement;

            engine.sfx_play(SfxId::SfxHitWall);
        }

        result.is_gameover =
            ball_next_pos.x > config.area_extents.x || ball_next_pos.x < -config.area_extents.x;

        if (result.is_gameover) {
            engine.sfx_play(SfxId::SfxGameOver);
        }

        ball_go.transform.set_pos_xy(ball_next_pos);

        return result;
    }

    std::optional<std::string> update_intermission_state(f32 dt, Engine &engine) {

        std::optional<std::string> next_state = std::nullopt;
        if (engine.input.just_pressed(KeyCode::Enter)) {

            // Reset world data
            engine.get_go("pad1").data.transform.set_pos_xy(Vec2(config.distance_from_center, 0));
            engine.get_go("pad2").data.transform.set_pos_xy(Vec2(-config.distance_from_center, 0));
            engine.get_go("ball").data.transform.set_pos_xy(Vec2::zero());
            engine.get_widget("score").data.set_str(0);

            world_init();

            engine.sfx_play(SfxId::SfxStart);

            next_state = "game_state";
        }

        engine.get_widget("intermission").ru.draw();

        return next_state;
    }
};