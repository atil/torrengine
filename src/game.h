struct Entity {
    std::string tag;

    explicit Entity(const std::string &tag) : tag(tag) {
    }
};

struct GameObject : public Entity {
    GoData data;
    GoRenderUnit ru;

    explicit GameObject(const std::string &tag, GoData data_, GoRenderUnit ru_)
        : Entity(tag), data(std::move(data_)), ru(std::move(ru_)) {
    }
};

struct ParticleSystem : public Entity {
    ParticleSource ps;
    ParticleRenderUnit ru;

    explicit ParticleSystem(const std::string &tag, ParticleSource ps_, ParticleRenderUnit ru_)
        : Entity(tag), ps(std::move(ps_)), ru(std::move(ru_)) {
    }
};

struct Widget : public Entity {
    WidgetData data;
    WidgetRenderUnit ru;

    explicit Widget(const std::string &tag, WidgetData data_, WidgetRenderUnit ru_)
        : Entity(tag), data(std::move(data_)), ru(std::move(ru_)) {
    }
};

struct Engine;

struct TorState {
    std::string name;
    std::function<std::optional<std::string>(f32, Engine &)> update_func;

    std::vector<EntityIndex> state_gos;
    std::vector<EntityIndex> state_ui;
    std::vector<EntityIndex> state_particles;

    explicit TorState(const std::string &name,
                      std::function<std::optional<std::string>(f32, Engine &)> update)
        : name(name), update_func(update) {
    }
};

struct Engine {
    std::vector<GameObject> game_objects;
    std::vector<ParticleSystem> particles;
    std::vector<Widget> ui;
    std::vector<TorState> all_states;

    Input input;
    u8 _padding1[4];
    Sfx sfx;
    ParticlePropRegistry particle_prop_reg;
    RenderInfo render_info;
    u8 _padding2[4];
    FontData font_data;

    // TODO @CLEANUP: Do these really belong here? Re-evaluate after having a good batching idea
    shader_handle_t world_shader;
    shader_handle_t ui_shader;

    explicit Engine(RenderInfo render_info, shader_handle_t world_shader, shader_handle_t ui_shader)
        : input(), sfx(), particle_prop_reg(particle_prop_registry_create()), render_info(render_info),
          font_data("assets/Consolas.ttf"), world_shader(world_shader), ui_shader(ui_shader) {
        game_objects.reserve(10);
        particles.reserve(10);
        ui.reserve(10);
        // TODO @ROBUSTNESS: These reserves are needed since reallocation-on-expand calls the destructor of
        // these (valid, not-moved-out-of) objects, and render units' destructors must not be called in this
        // case
    }

    GameObject &get_go(const std::string &tag) {
        for (GameObject &go : game_objects) {
            if (go.tag == tag) {
                return go;
            }
        }
        assert(false); // TODO @ROBUSTNESS: This won't be the case for a while
        exit(1);
    }

    Widget &get_widget(const std::string &tag) {
        for (Widget &widget : ui) {
            if (widget.tag == tag) {
                return widget;
            }
        }
        assert(false);
        exit(1);
    }

    TorState &get_state(const std::string &name) {
        for (TorState &state : all_states) {
            if (state.name == name) {
                return state;
            }
        }

        assert(false);
        exit(1);
    }

    void register_particle(const std::string &state_name, const ParticleProps &props, Vec2 emit_point) {
        shader_handle_t particle_shader = load_shader("src/world.glsl"); // Using world shader for now

        glUseProgram(particle_shader);
        Mat4 mat_identity = Mat4::identity();
        shader_set_mat4(particle_shader, "u_model", &mat_identity);
        shader_set_mat4(particle_shader, "u_view", &(render_info.view));
        shader_set_mat4(particle_shader, "u_proj", &(render_info.proj));

        particles.emplace_back("particle", ParticleSource(props, emit_point),
                               ParticleRenderUnit(props.count, particle_shader, "assets/Ball.png"));

        get_state(state_name).state_particles.push_back(particles.size() - 1);
    }

    // We might want to have a deregister_particle(ParticleSystem&) overload here in the future.
    // That will require the == operator's overload
    void deregister_particle(usize index) {
        particles.erase(particles.begin() + (i64)index);
    }

    void register_gameobject(const std::string &tag, const std::string &state_name, Vec2 pos, Vec2 size,
                             char *texture_path) {

        f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                                   0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
        u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

        game_objects.emplace_back(tag, GoData(pos, size),
                                  GoRenderUnit(unit_square_verts, sizeof(unit_square_verts),
                                               unit_square_indices, sizeof(unit_square_indices), world_shader,
                                               texture_path));

        for (TorState &state : all_states) {
            if (state.name == state_name) {
                state.state_gos.push_back(game_objects.size() - 1);
                break;
            }
        }
    }

    void register_ui_entity(const std::string &tag, const std::string &state_name, const std::string &text,
                            TextTransform transform) {
        WidgetData widget(text, transform, font_data); // It's fine if this is destroyed at the scope end

        ui.emplace_back(tag, widget, WidgetRenderUnit(ui_shader, widget));

        for (TorState &state : all_states) {
            if (state.name == state_name) {
                state.state_ui.push_back(ui.size() - 1);
                break;
            }
        }
    }

    void register_state(const std::string &name,
                        std::function<std::optional<std::string>(f32, Engine &)> update) {
        all_states.emplace_back(name, update);
    }
};

struct IGame {
    virtual void init(Engine &) = 0;
    virtual ~IGame() = default;
};

static void loop(IGame &game, Engine &engine, GLFWwindow *window) {

    f32 game_time = (f32)glfwGetTime();
    f32 dt = 0.0f;

    game.init(engine);

    TorState &curr_state = engine.all_states[0];
    while (!glfwWindowShouldClose(window)) {
        dt = (f32)glfwGetTime() - game_time;
        game_time = (f32)glfwGetTime();

        engine.input.update(window);

        if (engine.input.just_pressed(KeyCode::Esc)) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        std::optional<std::string> next_state = curr_state.update_func(dt, engine);
        for (EntityIndex i : curr_state.state_gos) {
            GameObject &go = engine.game_objects[i];
            go.ru.draw(go.data.transform);
        }
        for (EntityIndex i : curr_state.state_ui) {
            Widget &w = engine.ui[i];
            w.ru.draw();
        }

        std::vector<EntityIndex> &alive_particle_indices = curr_state.state_particles;
        std::vector<usize> dead_particle_indices(alive_particle_indices.size());
        for (EntityIndex i : alive_particle_indices) {
            ParticleSource &ps = engine.particles[i].ps;
            if (!ps.is_alive) {
                dead_particle_indices.push_back(i);
                continue;
            }

            ps.update(dt);
            engine.particles[i].ru.draw(ps);
        }
        for (usize i = 0; i < dead_particle_indices.size(); i++) {
            engine.deregister_particle(i);
        }

        if (next_state.has_value()) {
            curr_state = engine.get_state(next_state.value());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

static void main_game(IGame &game) {
    srand((unsigned long)time(0));

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

    shader_handle_t ui_shader = load_shader("src/ui.glsl");
    shader_handle_t world_shader = load_shader("src/world.glsl");
    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &render_info.view);
    shader_set_mat4(world_shader, "u_proj", &render_info.proj);

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();

    Engine engine(render_info, world_shader, ui_shader);

    loop(game, engine, window);

    //
    // Cleanup
    //

    // TODO @CLEANUP: We'll have some sort of batching for these two
    glDeleteProgram(world_shader);
    glDeleteProgram(ui_shader);

    glfwTerminate();
}
