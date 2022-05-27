EntityIndex register_particle(Core *core, const ParticleProps &props, const Renderer &renderer, Vec2 emit_point) {
    shader_handle_t particle_shader = load_shader("src/world.glsl"); // Using world shader for now

    glUseProgram(particle_shader);
    Mat4 mat_identity = mat4_identity();
    shader_set_mat4(particle_shader, "u_model", &mat_identity);
    shader_set_mat4(particle_shader, "u_view", &(renderer.view));
    shader_set_mat4(particle_shader, "u_proj", &(renderer.proj));

    core->particle_sources.emplace_back(props, emit_point);
    core->particle_render.emplace_back(props.count, particle_shader, "assets/Ball.png");

    return core->particle_sources.size();
}

static void deregister_particle(Core *core, EntityIndex ent_index) {
    core->particle_sources.erase(core->particle_sources.begin() + (i64)ent_index);
    core->particle_render.erase(core->particle_render.begin() + (i64)ent_index);
}

enum class GameState
{
    Splash,
    Game,
    GameOver
};

EntityIndex register_gameobject(Core *core, Vec2 pos, Vec2 size, char *texture_path,
                                shader_handle_t world_shader) {

    f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                               0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
    u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    core->go_data.emplace_back(pos, size);
    core->go_render.emplace_back(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                                 sizeof(unit_square_indices), world_shader, texture_path);

    return core->go_data.size();
}

EntityIndex register_ui_entity(Core *core, char *text, TextTransform transform, FontData *font_data,
                               shader_handle_t ui_shader) {
    Widget widget(text, transform, font_data);
    core->ui_widgets.push_back(widget);
    core->ui_render.emplace_back(ui_shader, widget);

    return core->ui_widgets.size();
}

void reset_game(Core *core, PongEntities *entities, PongWorldConfig *config) {

    core->go_data[entities->entity_world_field] =
        GoData(vec2_zero(), vec2_new(((f32)WIDTH / (f32)HEIGHT) * 10, 10));
    core->go_data[entities->entity_world_pad1] =
        GoData(vec2_new(config->distance_from_center, 0.0f), config->pad_size);
    core->go_data[entities->entity_world_pad2] =
        GoData(vec2_new(-(config->distance_from_center), 0.0f), config->pad_size);
    core->go_data[entities->entity_world_ball] = GoData(vec2_zero(), vec2_one() * 0.2f);

    EntityIndex entity_ui_score = entities->entity_ui_score;
    core->ui_widgets[entity_ui_score].set_str(0);
    core->ui_render[entity_ui_score].update(core->ui_widgets[entity_ui_score]);
    core->ui_render[entity_ui_score].draw();
}

// This is gonna be a part of the game module
static void loop(Core *core, PongWorld *world, PongEntities *entities, PongWorldConfig *config, GLFWwindow *window,
                 Input *input, const Renderer &renderer, ParticlePropRegistry *particle_prop_reg, Sfx *sfx,
                 shader_handle_t world_shader) {

    EntityIndex entity_ui_score = entities->entity_ui_score;

    GameState game_state = GameState::Splash;
    f32 game_time = (f32)glfwGetTime();
    f32 dt = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        dt = (f32)glfwGetTime() - game_time;
        game_time = (f32)glfwGetTime();

        input->update(window);

        if (input->just_pressed(KeyCode::Esc)) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (game_state == GameState::Splash) {
            core->ui_render[entities->entity_ui_splash].draw();

            if (input->just_pressed(KeyCode::Enter)) {
                world_init(world);
                sfx_play(sfx, SfxId::SfxStart);

                game_state = GameState::Game;
            }
        } else if (game_state == GameState::Game) {
            PongWorldUpdateResult result =
                world_update(dt, world, core, config, entities, input, sfx, particle_prop_reg, renderer);

            if (result.is_game_over) {
                sfx_play(sfx, SfxId::SfxGameOver);
                game_state = GameState::GameOver;
            }

            // Game draw
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(world_shader);

            for (usize i = 0; i < core->go_render.size(); i++) {
                core->go_render[i].draw(core->go_data[i].transform);
            }

            // Particle update/draw
            std::vector<EntityIndex> dead_particle_indices;
            for (usize i = 0; i < core->particle_sources.size(); i++) {
                ParticleSource &ps = core->particle_sources[i];
                if (!ps.is_alive) {
                    dead_particle_indices.push_back(i);
                    continue;
                }

                ps.update(dt);
                core->particle_render[i].draw(ps);
            }
            for (EntityIndex i = 0; i < dead_particle_indices.size(); i++) {
                deregister_particle(core, i);
            }

            // UI draw
            if (result.did_score) {

                // Update score view
                core->ui_widgets[entity_ui_score].set_str(world->score);
                core->ui_render[entity_ui_score].update(core->ui_widgets[entity_ui_score]);
            }

            core->ui_render[entity_ui_score].draw();
        } else if (game_state == GameState::GameOver) {
            core->ui_render[entities->entity_ui_intermission].draw();

            if (input->just_pressed(KeyCode::Enter)) {
                reset_game(core, entities, config);
                world_init(world);
                sfx_play(sfx, SfxId::SfxStart);
                game_state = GameState::Game;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

static void main_game() {
    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    // TODO @CLEANUP: Remove the init code for these from default ctors
    // It's weird to have code run on variable declarations
    FontData font_data;
    Sfx sfx;

    const f32 cam_size = 5.0f;

    //
    // Entities
    //

    f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                               0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
    u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    Renderer renderer = renderer_new(WIDTH, HEIGHT, cam_size);

    PongWorldConfig config;
    config.area_extents = vec2_new(cam_size * renderer.aspect, cam_size);
    config.pad_size = vec2_new(0.3f, 2.0f);
    config.ball_speed = 4.0f;
    config.distance_from_center = 4.0f;
    config.pad_move_speed = 10.0f;
    config.game_speed_increase_coeff = 0.05f;

    Core core;

    shader_handle_t world_shader = load_shader("src/world.glsl");
    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_handle_t ui_shader = load_shader("src/ui.glsl");

    register_ui_entity(&core, "TorrPong!\0",
                       TextTransform(vec2_new(-0.8f, 0), 0.5f, TextWidthType::FixedWidth, 1.6f), &font_data,
                       ui_shader);
    register_ui_entity(&core, "0\0", TextTransform(vec2_new(-0.9f, -0.9f), 0.3f, TextWidthType::FreeWidth, 0.1f),
                       &font_data, ui_shader);
    register_ui_entity(&core, "Game Over\0",
                       TextTransform(vec2_new(-0.75f, 0.0f), 0.5f, TextWidthType::FixedWidth, 1.5f), &font_data,
                       ui_shader);

    register_gameobject(&core, vec2_zero(), vec2_new(((f32)WIDTH / (f32)HEIGHT) * 10, 10), "assets/Field.png",
                        world_shader);
    register_gameobject(&core, vec2_new(config.distance_from_center, 0.0f), config.pad_size, "assets/PadBlue.png",
                        world_shader);
    register_gameobject(&core, vec2_new(-config.distance_from_center, 0.0f), config.pad_size,
                        "assets/PadGreen.png", world_shader);

    register_gameobject(&core, vec2_zero(), vec2_one() * 0.2f, "assets/Ball.png", world_shader);

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &renderer.view);
    shader_set_mat4(world_shader, "u_proj", &renderer.proj);

    PongWorld world;
    Input input;

    PongEntities entities;
    loop(&core, &world, &entities, &config, window, &input, renderer, &particle_prop_reg, &sfx, world_shader);

    //
    // Cleanup
    //

    // TODO @CLEANUP: We'll have some sort of batching for these two
    glDeleteProgram(world_shader);
    glDeleteProgram(ui_shader);

    glfwTerminate();
}
