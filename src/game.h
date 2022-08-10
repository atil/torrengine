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

    // TODO @CLEANUP: Convert this to a macro
    GameObject(const GameObject &) = delete;
    GameObject(GameObject &&) = delete;
    GameObject &operator=(const GameObject &) = delete;
    GameObject &operator=(GameObject &&) = delete;
};

struct ParticleSystem : public Entity {
    ParticleSource ps;
    ParticleRenderUnit ru;

    explicit ParticleSystem(const std::string &tag, ParticleSource ps_, ParticleRenderUnit ru_)
        : Entity(tag), ps(std::move(ps_)), ru(std::move(ru_)) {
    }

    ParticleSystem(const ParticleSystem &) = delete;
    ParticleSystem(ParticleSystem &&) = delete;
    ParticleSystem &operator=(const ParticleSystem &) = delete;
    ParticleSystem &operator=(ParticleSystem &&) = delete;
};

struct Widget : public Entity {
    WidgetData data;
    WidgetRenderUnit ru;

    explicit Widget(const std::string &tag, WidgetData data_, WidgetRenderUnit ru_)
        : Entity(tag), data(std::move(data_)), ru(std::move(ru_)) {
    }

    Widget(const Widget &) = delete;
    Widget(Widget &&) = delete;
    Widget &operator=(const Widget &) = delete;
    Widget &operator=(Widget &&) = delete;
};

struct Engine;

struct TorState {
    std::string name;
    std::function<std::optional<std::string>(f32, Engine &)> update_func;

    std::vector<std::weak_ptr<GameObject>> state_gos;
    std::vector<std::weak_ptr<Widget>> state_ui;
    std::vector<std::weak_ptr<ParticleSystem>> state_particles;

    explicit TorState(const std::string &name,
                      std::function<std::optional<std::string>(f32, Engine &)> update)
        : name(name), update_func(update) {
    }
};

struct Engine {
    std::vector<std::shared_ptr<GameObject>> game_objects;
    std::vector<std::shared_ptr<ParticleSystem>> particles;
    std::vector<std::shared_ptr<Widget>> ui;
    std::vector<TorState> all_states;

    Input input;
    u8 _padding1[8];
    Sfx sfx;
    ParticlePropRegistry particle_prop_reg;
    RenderInfo render_info;
    u8 _padding2[4];
    FontData font_data;

    // TODO @CLEANUP: Do these really belong here? Re-evaluate after having a good batching idea
    shader_handle_t world_shader;
    shader_handle_t ui_shader;

    explicit Engine(RenderInfo render_info, std::vector<SfxAsset> sfx_assets, shader_handle_t world_shader_,
                    shader_handle_t ui_shader_)
        : input(), sfx(sfx_assets), particle_prop_reg(particle_prop_registry_create()),
          render_info(render_info), font_data("assets/Consolas.ttf"), world_shader(world_shader_),
          ui_shader(ui_shader_) {
        game_objects.reserve(10);
        particles.reserve(10);
        ui.reserve(10);
        // TODO @ROBUSTNESS: These reserves are needed since reallocation-on-expand calls the destructor of
        // these (valid, not-moved-out-of) objects, and render units' destructors must not be called in this
        // case
    }

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine &operator=(Engine &&) = delete;

    GameObject &get_go(const std::string &tag) {
        for (auto go : game_objects) {
            if (go->tag == tag) {
                return *go;
            }
        }
        assert(false); // TODO @ROBUSTNESS: This won't be the case for a while
        exit(1);
    }

    Widget &get_widget(const std::string &tag) {
        for (auto widget : ui) {
            if (widget->tag == tag) {
                return *widget;
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
        shader_handle_t particle_shader = Shader::load("src/shader/world.glsl"); // Using world shader for now

        glUseProgram(particle_shader);
        Mat4 mat_identity = Mat4::identity();
        Shader::set_mat4(particle_shader, "u_model", &mat_identity);
        Shader::set_mat4(particle_shader, "u_view", &(render_info.view));
        Shader::set_mat4(particle_shader, "u_proj", &(render_info.proj));

        std::shared_ptr<ParticleSystem> ps = std::make_shared<ParticleSystem>(
            "particle", ParticleSource(props, emit_point),
            ParticleRenderUnit(props.count, particle_shader, "assets/Ball.png"));

        particles.push_back(ps);
        get_state(state_name).state_particles.push_back(ps);
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

        std::shared_ptr<GameObject> go_ptr = std::make_shared<GameObject>(
            tag, GoData(pos, size),
            GoRenderUnit(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                         sizeof(unit_square_indices), world_shader, texture_path));
        game_objects.push_back(go_ptr);

        for (TorState &state : all_states) {
            if (state.name == state_name) {
                state.state_gos.push_back(go_ptr);
                break;
            }
        }
    }

    void register_ui_entity(const std::string &tag, const std::string &state_name, const std::string &text,
                            TextTransform transform) {
        WidgetData widget(text, transform, font_data); // It's fine if this is destroyed at the scope end

        std::shared_ptr<Widget> widget_ptr =
            std::make_shared<Widget>(tag, widget, WidgetRenderUnit(ui_shader, widget));

        ui.push_back(widget_ptr);

        for (TorState &state : all_states) {
            if (state.name == state_name) {
                state.state_ui.push_back(widget_ptr);
                break;
            }
        }
    }

    void register_state(const std::string &name,
                        std::function<std::optional<std::string>(f32, Engine &)> update) {
        all_states.emplace_back(name, update);
    }

    void sfx_play(SfxId id) {
        sfx.play(id);
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
        for (auto go_weak : curr_state.state_gos) {
            std::shared_ptr<GameObject> go_shared = go_weak.lock();
            go_shared->ru.draw(go_shared->data.transform);
        }
        for (auto widget_weak : curr_state.state_ui) {
            std::shared_ptr<Widget> widget_shared = widget_weak.lock();
            widget_shared->ru.draw();
        }

        std::vector<usize> dead_particle_indices;
        dead_particle_indices.reserve(curr_state.state_particles.size());
        for (usize i = 0; i < curr_state.state_particles.size(); i++) {
            std::shared_ptr<ParticleSystem> particle_shared = curr_state.state_particles[i].lock();
            if (!particle_shared->ps.is_alive) {
                dead_particle_indices.push_back(i);
                continue;
            }
            particle_shared->ps.update(dt);
            particle_shared->ru.draw(particle_shared->ps);
        }
        for (usize i = 0; i < dead_particle_indices.size(); i++) {
            engine.deregister_particle(i);
            curr_state.state_particles.erase(curr_state.state_particles.begin() + (i64)i);
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

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true); // To enable debug output

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    const f32 cam_size = 5.0f;
    RenderInfo render_info = render_info_new(WIDTH, HEIGHT, cam_size);

    shader_handle_t ui_shader = Shader::load("src/shader/ui.glsl");
    shader_handle_t world_shader = Shader::load("src/shader/world.glsl");
    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // OpenGL debug output
    // TODO @CLEANUP: Bind this to a switch or something
    // glEnable(GL_DEBUG_OUTPUT);
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // glDebugMessageCallback(glDebugOutput, nullptr);
    // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    glUseProgram(world_shader);
    Shader::set_mat4(world_shader, "u_view", &render_info.view);
    Shader::set_mat4(world_shader, "u_proj", &render_info.proj);

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();

    std::vector<SfxAsset> sfx_assets;
    sfx_assets.emplace_back(SfxId::SfxStart, "assets/Start.Wav");
    sfx_assets.emplace_back(SfxId::SfxHitPad, "assets/HitPad.Wav");
    sfx_assets.emplace_back(SfxId::SfxHitWall, "assets/HitWall.Wav");
    sfx_assets.emplace_back(SfxId::SfxGameOver, "assets/GameOver.Wav");

    Engine engine(render_info, sfx_assets, world_shader, ui_shader);

    loop(game, engine, window);

    //
    // Cleanup
    //

    // TODO @CLEANUP: We'll have some sort of batching for these two
    glDeleteProgram(world_shader);
    glDeleteProgram(ui_shader);

    glfwTerminate();
}
