#pragma warning(disable : 5045) // Spectre thing

enum class ParticleSystemType {
    PadLeft,
    PadRight
};

struct Particle {
    u32 index;
    f32 angle;
    f32 speed_offset; // In percentage
};

struct ParticleProps {
    Vec2 angle_limits;
    usize count;
    f32 lifetime;
    f32 speed;
    f32 angle_offset;
    f32 speed_offset;
    f32 size;
    u8 _padding[4];
};

struct ParticleSource {
    Vec2 *positions;
    Particle *particles;
    const ParticleProps &props;
    f32 life;
    Vec2 emit_point;
    f32 transparency;
    bool is_alive;
    u8 _padding[7];

    explicit ParticleSource(const ParticleProps &props, Vec2 emit_point);

    ParticleSource(ParticleSource &&rhs);
    ParticleSource &operator=(ParticleSource &&rhs) = delete;
    ParticleSource(const ParticleSource &rhs) = delete;
    ParticleSource &operator=(const ParticleSource &rhs) = delete;

    ~ParticleSource();

    void update(f32 dt);
};
