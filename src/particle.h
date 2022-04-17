// start from here:
// - randomness per particle

typedef struct
{
    uint32_t index;
    float angle;
    float speed_offset;
} Particle;

typedef struct
{
    Vec2 emit_point;
    Vec2 angle_limits;
    size_t count;
    float lifetime;
    float speed;
    float angle_offset;
    float speed_offset;
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

static ParticleSystem *particle_system_init(ParticleProps props)
{
    ParticleSystem *ps = (ParticleSystem *)malloc(sizeof(ParticleSystem));
    ps->positions = (Vec2 *)malloc(props.count * sizeof(Vec2));
    ps->particles = (Particle *)malloc(props.count * sizeof(Particle));
    ps->life = 0;
    ps->isAlive = false;
    ps->props = props;

    for (uint32_t i = 0; i < ps->props.count; i++)
    {
        ps->particles[i].index = i;
        ps->particles[i].angle = // Notice the "-1", we want the end angle to be inclusive
            lerp(ps->props.angle_limits.x, ps->props.angle_limits.y, (float)i / (ps->props.count - 1));

        ps->particles[i].angle += rand_range(-ps->props.angle_offset, ps->props.angle_offset);
        ps->particles[i].speed_offset =
            ps->props.speed * rand_range(-ps->props.speed_offset, ps->props.speed_offset);

        ps->positions[i] = ps->props.emit_point;
    }

    return ps;
}

static void particle_system_update(ParticleSystem *ps, float dt)
{
    for (uint32_t i = 0; i < ps->props.count; i++)
    {
        Vec2 dir =
            vec2_new((float)cos(ps->particles[i].angle * DEG2RAD), (float)sin(ps->particles[i].angle * DEG2RAD));

        float speed = ps->props.speed + ps->particles[i].speed_offset;
        ps->positions[i] = vec2_add(ps->positions[i], vec2_scale(dir, speed * dt));
    }

    ps->life += dt;
    ps->transparency = (ps->props.lifetime - ps->life) / ps->props.lifetime;
    ps->isAlive = ps->life < ps->props.lifetime;
}

static void particle_system_deinit(ParticleSystem *ps)
{
    free(ps->positions);
    free(ps->particles);
    free(ps);
}
