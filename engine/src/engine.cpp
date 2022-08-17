#include <cassert>
#include "engine.h"

Entity::Entity(const std::string &tag) : tag(tag) {
}

GameObject::GameObject(const std::string &tag, GoData data_, GoRenderUnit ru_)
    : Entity(tag), data(std::move(data_)), ru(std::move(ru_)) {
}

ParticleSystem::ParticleSystem(const std::string &tag, ParticleSource ps_, ParticleRenderUnit ru_)
    : Entity(tag), ps(std::move(ps_)), ru(std::move(ru_)) {
}

Widget::Widget(const std::string &tag, WidgetData data_, WidgetRenderUnit ru_)
    : Entity(tag), data(std::move(data_)), ru(std::move(ru_)) {
}

Scene::Scene(const std::string &name, std::function<std::optional<std::string>(f32, Engine &)> update)
    : name(name), update_func(update) {
}

Engine::Engine(u32 screen_width, u32 screen_height, f32 cam_size, std::vector<SfxAsset> sfx_assets)
    : input(), sfx(sfx_assets), renderer(screen_width, screen_height, cam_size),
      font_data("assets/Consolas.ttf") {
}

GameObject &Engine::get_go(const std::string &tag) {
    for (auto go : game_objects) {
        if (go->tag == tag) {
            return *go;
        }
    }
    UNREACHABLE("Gameobject not found");
}

Widget &Engine::get_widget(const std::string &tag) {
    for (auto widget : ui) {
        if (widget->tag == tag) {
            return *widget;
        }
    }
    UNREACHABLE("Widget not found");
}

Scene &Engine::get_scene(const std::string &name) {
    for (Scene &state : all_scenes) {
        if (state.name == name) {
            return state;
        }
    }

    UNREACHABLE("Scene not found");
}

void Engine::register_particle_prop(ParticleSystemType type, const ParticleProps &props) {
    if (particle_props.find(type) != particle_props.end()) {
        printf("Trying to re-register the particle type: %d\n", type);
        return;
    }

    particle_props.insert(std::make_pair(type, props));
}

void Engine::register_particle(const std::string &state_name, ParticleSystemType type, Vec2 emit_point) {
    const ParticleProps &props = particle_props[type];

    std::shared_ptr<ParticleSystem> ps = std::make_shared<ParticleSystem>(
        "particle", ParticleSource(props, emit_point),
        ParticleRenderUnit(props.count, renderer.render_info, "assets/Ball.png"));

    particles.push_back(ps);
    get_scene(state_name).state_particles.push_back(ps);
}

// We might want to have a deregister_particle(ParticleSystem&) overload here in the future.
// That will require the == operator's overload
void Engine::deregister_particle(usize index) {
    particles.erase(particles.begin() + (i64)index);
}

void Engine::register_gameobject(const std::string &tag, const std::string &state_name, Vec2 pos, Vec2 size,
                                 char *texture_path) {

    f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                               0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
    u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    std::shared_ptr<GameObject> go_ptr = std::make_shared<GameObject>(
        tag, GoData(pos, size),
        GoRenderUnit(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), renderer.world_shader, texture_path));
    game_objects.push_back(go_ptr);

    for (Scene &state : all_scenes) {
        if (state.name == state_name) {
            state.state_gos.push_back(go_ptr);
            break;
        }
    }
}

void Engine::register_ui_entity(const std::string &tag, const std::string &state_name,
                                const std::string &text, TextTransform transform) {
    WidgetData widget(text, transform, font_data); // It's fine if this is destroyed at the scope end

    std::shared_ptr<Widget> widget_ptr =
        std::make_shared<Widget>(tag, widget, WidgetRenderUnit(renderer.ui_shader, widget));

    ui.push_back(widget_ptr);

    for (Scene &state : all_scenes) {
        if (state.name == state_name) {
            state.state_ui.push_back(widget_ptr);
            break;
        }
    }
}

void Engine::register_state(const std::string &name,
                            std::function<std::optional<std::string>(f32, Engine &)> update) {
    all_scenes.emplace_back(name, update);
}

void Engine::sfx_play(SfxId id) {
    sfx.play(id);
}

void Engine::particle_play(const std::string &state_name, ParticleSystemType type, Vec2 collision_point) {
    register_particle(state_name, type, collision_point);
}