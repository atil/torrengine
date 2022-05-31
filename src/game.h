struct Entity {
    std::string tag;

    explicit Entity(const std::string &tag) : tag(tag) {
    }
};

struct GameObject : public Entity {
    GoData data;
    GoRenderUnit ru;

    explicit GameObject(const std::string &tag, GoData data, GoRenderUnit ru)
        : Entity(tag), data(std::move(data)), ru(std::move(ru)) {
    }
};

struct ParticleSystem : public Entity {
    ParticleSource ps;
    ParticleRenderUnit ru;

    explicit ParticleSystem(const std::string &tag, ParticleSource ps, ParticleRenderUnit ru)
        : Entity(tag), ps(std::move(ps)), ru(std::move(ru)) {
    }
};

struct Widget : public Entity {
    WidgetData data;
    WidgetRenderUnit ru;

    explicit Widget(const std::string &tag, WidgetData data, WidgetRenderUnit ru)
        : Entity(tag), data(std::move(data)), ru(std::move(ru)) {
    }
};

struct Core {
    std::vector<GameObject> game_objects;
    std::vector<ParticleSystem> particles;
    std::vector<Widget> ui;

    GameObject &get_go(const std::string &tag) {
        for (GameObject &go : game_objects) {
            if (go.tag == tag) {
                return go;
            }
        }
        exit(1); // TODO @ROBUSTNESS: This won't be the case for a while
    }

    Widget &get_widget(const std::string &tag) {
        for (Widget &widget : ui) {
            if (widget.tag == tag) {
                return widget;
            }
        }
        exit(1);
    }

    void register_particle(const ParticleProps &props, const RenderInfo &render_info, Vec2 emit_point) {
        shader_handle_t particle_shader = load_shader("src/world.glsl"); // Using world shader for now

        glUseProgram(particle_shader);
        Mat4 mat_identity = Mat4::identity();
        shader_set_mat4(particle_shader, "u_model", &mat_identity);
        shader_set_mat4(particle_shader, "u_view", &(render_info.view));
        shader_set_mat4(particle_shader, "u_proj", &(render_info.proj));

        particles.emplace_back("particle", ParticleSource(props, emit_point),
                               ParticleRenderUnit(props.count, particle_shader, "assets/Ball.png"));
    }

    void deregister_particle(ParticleSystem &to_delete) {
        particles.erase(std::remove(particles.begin(), particles.end(), to_delete), particles.end());
    }

    void deregister_particle(usize index) {
        particles.erase(particles.begin() + index);
    }

    void register_gameobject(const std::string &tag, Vec2 pos, Vec2 size, char *texture_path,
                             shader_handle_t world_shader) {

        f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                                   0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
        u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

        game_objects.emplace_back(tag, GoData(pos, size),
                                  GoRenderUnit(unit_square_verts, sizeof(unit_square_verts),
                                               unit_square_indices, sizeof(unit_square_indices), world_shader,
                                               texture_path));
    }

    void register_ui_entity(const std::string &tag, const std::string &text, TextTransform transform,
                            const FontData &font_data, shader_handle_t ui_shader) {
        WidgetData widget(text, transform, font_data); // It's fine if this is destroyed at the scope end

        ui.emplace_back(tag, widget, WidgetRenderUnit(ui_shader, widget));
    }
};

struct Engine { // This is gonna own everything.
    Core core;  // Would it make more sense to replace the vectors with this?
    Input input;
    Sfx sfx;
    ParticlePropRegistry particle_prop_reg;
    RenderInfo render_info;
    FontData font_data;

    // TODO @CLEANUP: Do these really belong here? Re-evaluate after having a good batching idea
    shader_handle_t world_shader;
    shader_handle_t ui_shader;

    explicit Engine(RenderInfo render_info, shader_handle_t world_shader, shader_handle_t ui_shader)
        : core(), input(), sfx(), particle_prop_reg(particle_prop_registry_create()),
          font_data("assets/Consolas.ttf"), world_shader(world_shader), ui_shader(ui_shader) {
    }
};

struct TrState {
    std::string name;
    std::function<void(f32, Engine &)> update;

    explicit TrState(const std::string &name, std::function<void(f32, Engine &)> update)
        : name(name), update(update) {
    }
};

void reset_game() {

    //
    // TODO @NOCHECKIN: Fix this the last. Needs to be part of pong.h

    // engine.core.go_data[entities->entity_world_field] =
    //     GoData(Vec2::zero(), Vec2(((f32)WIDTH / (f32)HEIGHT) * 10, 10));
    // engine.core.go_data[entities->entity_world_pad1] =
    //     GoData(Vec2(config->distance_from_center, 0.0f), config->pad_size);
    // engine.core.go_data[entities->entity_world_pad2] =
    //     GoData(Vec2(-(config->distance_from_center), 0.0f), config->pad_size);
    // engine.core.go_data[entities->entity_world_ball] = GoData(Vec2::zero(), Vec2::one() * 0.2f);

    // EntityIndex entity_ui_score = entities->entity_ui_score;
    // engine.core.ui_widgets[entity_ui_score].set_str(0);
    // engine.core.ui_render[entity_ui_score].update(engine.core.ui_widgets[entity_ui_score]);
    // engine.core.ui_render[entity_ui_score].draw();
}

static void loop(Engine &engine, GLFWwindow *window) {

    f32 game_time = (f32)glfwGetTime();
    f32 dt = 0.0f;

    PongGame pong; // We don't know this type yet. How does this work?
    pong.init(engine);

    // This is gonna probably change when we move this stuff to lua
    TrState splash_state("splash", std::bind(&PongGame::update_splash_state, pong, std::placeholders::_1,
                                             std::placeholders::_2));
    TrState game_state(
        "game", std::bind(&PongGame::update_game_state, pong, std::placeholders::_1, std::placeholders::_2));
    TrState intermission_state("intermission", std::bind(&PongGame::update_intermission_state, pong,
                                                         std::placeholders::_1, std::placeholders::_2));

    while (!glfwWindowShouldClose(window)) {
        dt = (f32)glfwGetTime() - game_time;
        game_time = (f32)glfwGetTime();

        engine.input.update(window);

        if (engine.input.just_pressed(KeyCode::Esc)) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (game_state == GameState::Splash) {
            engine.core.get_widget("splash").ru.draw();

            if (engine.input.just_pressed(KeyCode::Enter)) {

                world_init(world);

                sfx_play(&engine.sfx, SfxId::SfxStart);

                game_state = GameState::Game;
            }
        } else if (game_state == GameState::Game) {

            world_update(dt, world, core, config, entities, input, sfx, particle_prop_reg, render_info);

            if (TODO.isgameover) {
                sfx_play(sfx, SfxId::SfxGameOver);
                game_state = GameState::GameOver;
            }

            // Game draw
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(engine.world_shader);

            for (GameObject &go : engine.core.game_objects) {
                go.ru.draw(go.data.transform);
            }

            // Particle update/draw
            std::vector<usize> dead_particle_indices(engine.core.particles.size());
            for (usize i = 0; i < engine.core.particles.size(); i++) {
                ParticleSource &ps = engine.core.particles[i].ps;
                if (!ps.is_alive) {
                    dead_particle_indices.push_back(i);
                    continue;
                }

                ps.update(dt);
                engine.core.particles[i].ru.draw(ps);
            }
            for (usize i = 0; i < dead_particle_indices.size(); i++) {
                engine.core.deregister_particle(i);
            }

            // UI draw
            Widget &score_widget = engine.core.get_widget("score");
            if (TODO.didscore) {

                // Update score view
                score_widget.data.set_str(0); // TODO @NOCHECKIN
                score_widget.ru.update(score_widget.data);
            }
            score_widget.ru.draw();

        } else if (game_state == GameState::GameOver) {
            engine.core.get_widget("intermission").ru.draw();

            if (engine.input.just_pressed(KeyCode::Enter)) {
                reset_game();

                // world_init()

                sfx_play(&engine.sfx, SfxId::SfxStart);
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
    FontData font_data("assets/Consolas.ttf");
    Sfx sfx;

    const f32 cam_size = 5.0f;
    RenderInfo render_info = render_info_new(WIDTH, HEIGHT, cam_size);

    Core core;

    shader_handle_t ui_shader = load_shader("src/ui.glsl");
    shader_handle_t world_shader = load_shader("src/world.glsl");
    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &render_info.view);
    shader_set_mat4(world_shader, "u_proj", &render_info.proj);

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();

    Engine engine(render_info, world_shader, ui_shader);

    loop(engine, window);

    //
    // Cleanup
    //

    // TODO @CLEANUP: We'll have some sort of batching for these two
    glDeleteProgram(world_shader);
    glDeleteProgram(ui_shader);

    glfwTerminate();
}
