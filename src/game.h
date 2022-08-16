#include <ctime>
#include <GLFW/glfw3.h>
#include "engine.h"
#include "input.h"

#define WIDTH 640
#define HEIGHT 480

// TODO @REFACTOR: This would look better as "engine.RunGame(game, window)"
static void loop(IGame &game, Engine &engine, GLFWwindow *window) {

    f32 game_time = (f32)glfwGetTime();
    f32 dt = 0.0f;

    game.init(engine);

    Scene &curr_state = engine.all_scenes[0];
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
            curr_state = engine.get_scene(next_state.value());
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

    std::vector<SfxAsset> sfx_assets;
    sfx_assets.emplace_back(SfxId::SfxStart, "assets/Start.Wav");
    sfx_assets.emplace_back(SfxId::SfxHitPad, "assets/HitPad.Wav");
    sfx_assets.emplace_back(SfxId::SfxHitWall, "assets/HitWall.Wav");
    sfx_assets.emplace_back(SfxId::SfxGameOver, "assets/GameOver.Wav");

    Engine engine(WIDTH, HEIGHT, sfx_assets);

    loop(game, engine, window);

    glfwTerminate();
}
