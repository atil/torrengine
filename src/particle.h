// start from here:
// - add duration/lifetime to these
// - add spawn position
// - add limit angles

typedef struct
{
    uint32_t index;
    float angle;
} Particle;

typedef struct
{
    Vec2 *positions;
    Particle *particles;
    size_t particle_count;
    float life;
    float lifetime;
    bool isAlive;
    uint8_t _padding[7];
} ParticleSystem;

static void particle_init(ParticleSystem *ps, Vec2 pos, size_t particle_count, float lifetime)
{
    ps->particle_count = particle_count;
    ps->positions = (Vec2 *)malloc(particle_count * sizeof(Vec2));
    ps->particles = (Particle *)malloc(particle_count * sizeof(Particle));
    ps->lifetime = lifetime;
    ps->life = 0;
    ps->isAlive = true;

    for (uint32_t i = 0; i < particle_count; i++)
    {
        ps->particles[i].index = i;
        ps->particles[i].angle = (360.0f / particle_count) * i;

        ps->positions[i] = pos;
    }
}

static void particle_update(ParticleSystem *ps, float dt)
{
    for (uint32_t i = 0; i < ps->particle_count; i++)
    {
        Vec2 dir =
            vec2_new((float)cos(ps->particles[i].angle * DEG2RAD), (float)sin(ps->particles[i].angle * DEG2RAD));
        const float particle_speed = 1.0f;
        ps->positions[i] = vec2_add(ps->positions[i], vec2_scale(dir, particle_speed * dt));
    }

    ps->life += dt;
    ps->isAlive = ps->life < ps->lifetime;
}

static void particle_deinit(ParticleSystem *ps)
{
    free(ps->positions);
    free(ps->particles);
}

