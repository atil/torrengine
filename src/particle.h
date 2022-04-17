// start from here:
// - add transparency

typedef struct
{
    uint32_t index;
    float angle;
} Particle;

typedef struct
{
    Vec2 emit_point;
    Vec2 angle_limits;
    size_t count;
    float lifetime;
    uint8_t _padding[4];
} ParticleProps;

typedef struct
{
    Vec2 *positions;
    Particle *particles;
    ParticleProps props;
    float life;
    float transparency;
    bool isAlive;
    uint8_t _padding[7];
} ParticleSystem;

static void particle_init(ParticleSystem *ps, ParticleProps props)
{
    ps->positions = (Vec2 *)malloc(props.count * sizeof(Vec2));
    ps->particles = (Particle *)malloc(props.count * sizeof(Particle));
    ps->life = 0;
    ps->isAlive = true;
    ps->props = props;

    for (uint32_t i = 0; i < ps->props.count; i++)
    {
        ps->particles[i].index = i;
        ps->particles[i].angle = lerp(ps->props.angle_limits.x, ps->props.angle_limits.y, (float)i / ps->props.count);
        ps->positions[i] = ps->props.emit_point;
    }
}

static void particle_update(ParticleSystem *ps, float dt)
{
    for (uint32_t i = 0; i < ps->props.count; i++)
    {
        Vec2 dir = vec2_new((float)cos(ps->particles[i].angle * DEG2RAD), (float)sin(ps->particles[i].angle * DEG2RAD));
        const float particle_speed = 1.0f;
        ps->positions[i] = vec2_add(ps->positions[i], vec2_scale(dir, particle_speed * dt));
    }

    ps->life += dt;
    ps->transparency = (ps->props.lifetime - ps->life) / ps->props.lifetime;
    ps->isAlive = ps->life < ps->props.lifetime;
}

static void particle_deinit(ParticleSystem *ps)
{
    free(ps->positions);
    free(ps->particles);
}
