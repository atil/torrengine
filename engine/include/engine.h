// No pragma once. Should only be included from the main translation unit

#include <string>
#include <functional>
#include <optional>
#include "godata.h"
#include "input.h"
#include "render.h"
#include "particle.h"
#include "sfx.h"

struct Entity {
    std::string tag;

    explicit Entity(const std::string &tag);
    virtual ~Entity() = default;
};

struct GameObject : public Entity {
    GoData data;
    GoRenderUnit ru;

    PREVENT_COPY_MOVE(GameObject);
    explicit GameObject(const std::string &tag, GoData data, GoRenderUnit ru);
};

struct ParticleSystem : public Entity {
    ParticleSource ps;
    ParticleRenderUnit ru;

    PREVENT_COPY_MOVE(ParticleSystem);
    explicit ParticleSystem(const std::string &tag, ParticleSource ps, ParticleRenderUnit ru);
};

struct Widget : public Entity {
    WidgetData data;
    WidgetRenderUnit ru;

    PREVENT_COPY_MOVE(Widget);
    explicit Widget(const std::string &tag, WidgetData data, WidgetRenderUnit ru);
};

class Engine;

struct Scene {
    std::string name;
    std::function<std::optional<std::string>(f32, Engine &)> update_func;

    std::vector<std::weak_ptr<GameObject>> state_gos;
    std::vector<std::weak_ptr<Widget>> state_ui;
    std::vector<std::weak_ptr<ParticleSystem>> state_particles;

    explicit Scene(const std::string &name, std::function<std::optional<std::string>(f32, Engine &)> update);
};

class Engine {
    friend struct Application;

    std::vector<std::shared_ptr<GameObject>> game_objects;
    std::vector<std::shared_ptr<ParticleSystem>> particles;
    std::vector<std::shared_ptr<Widget>> ui;
    std::vector<Scene> all_scenes;

    Input input;
    Sfx sfx;
    std::unordered_map<ParticleSystemType, ParticleProps> particle_props;

    Renderer renderer;
    FontData font_data;

  public:
    PREVENT_COPY_MOVE(Engine);
    explicit Engine(u32 screen_width, u32 screen_height, f32 cam_size, std::vector<SfxAsset> sfx_assets);

    GameObject &get_go(const std::string &tag) const;
    Widget &get_widget(const std::string &tag) const;
    Scene &get_scene(const std::string &name);
    const RenderInfo &get_render_info() const;

    void register_particle_prop(ParticleSystemType type, const ParticleProps &props);
    void register_particle(const std::string &state_name, ParticleSystemType type, Vec2 emit_point);
    void deregister_particle(usize index);

    void register_gameobject(const std::string &tag, const std::string &state_name, Vec2 pos, Vec2 size,
                             char *texture_path);

    void register_ui_entity(const std::string &tag, const std::string &state_name, const std::string &text,
                            TextTransform transform);

    void register_state(const std::string &name,
                        std::function<std::optional<std::string>(f32, Engine &)> update);

    void sfx_play(SfxId id);
    void particle_play(const std::string &state_name, ParticleSystemType type, Vec2 collision_point);
    bool input_just_pressed(KeyCode key_code) const;
    bool input_is_down(KeyCode key_code) const;
};