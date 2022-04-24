typedef struct
{
    uint32_t index;
    float angle;
    float speed_offset; // In percentage
} Particle;

typedef struct
{
    Vec2 angle_limits;
    size_t count;
    float lifetime;
    float speed;
    float angle_offset;
    float speed_offset;
    float size;
    uint8_t _padding[4];
} ParticleProps;

typedef struct
{
    Vec2 *positions;
    Particle *particles;
    ParticleProps props;
    float life;
    Vec2 emit_point;
    float transparency;
    bool isAlive;
    uint8_t _padding[7];
} ParticleEmitter; // TODO @CLEANUP: Rename to ParticleSource?

typedef struct
{
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t uv_bo;
    buffer_handle_t ibo;
    uint32_t index_count;
    shader_handle_t shader;
    texture_handle_t texture;
    uint32_t vert_data_len;
    float *vert_data;
} ParticleRenderUnit;

typedef struct
{
    ParticleEmitter *emitter;
    ParticleRenderUnit *render_unit;
} ParticleSystem;

typedef struct
{
    ParticleSystem *array_ptr;
    size_t system_count;
    size_t system_capacity;
} ParticleSystemRegistry;

typedef struct
{
    ParticleProps pad_hit_right;
    ParticleProps pad_hit_left;
} ParticlePropRegistry;

static ParticlePropRegistry particle_prop_registry_create(void)
{
    ParticlePropRegistry reg;
    reg.pad_hit_right.angle_limits = vec2_new(90, 270);
    reg.pad_hit_right.count = 5;
    reg.pad_hit_right.lifetime = 1;
    reg.pad_hit_right.speed = 1;
    reg.pad_hit_right.angle_offset = 10;
    reg.pad_hit_right.speed_offset = 0.1f;
    reg.pad_hit_right.size = 0.3f;

    reg.pad_hit_left.angle_limits = vec2_new(-90, 90);
    reg.pad_hit_left.count = 5;
    reg.pad_hit_left.lifetime = 1;
    reg.pad_hit_left.speed = 1;
    reg.pad_hit_left.angle_offset = 10;
    reg.pad_hit_left.speed_offset = 0.1f;
    reg.pad_hit_left.size = 0.3f;

    return reg;
}

static ParticleEmitter *particle_emitter_init(ParticleProps props, Vec2 emit_point)
{
    ParticleEmitter *pe = (ParticleEmitter *)malloc(sizeof(ParticleEmitter));
    pe->positions = (Vec2 *)malloc(props.count * sizeof(Vec2));
    pe->particles = (Particle *)malloc(props.count * sizeof(Particle));
    pe->life = 0;
    pe->isAlive = false;
    pe->props = props;
    pe->emit_point = emit_point;

    for (uint32_t i = 0; i < pe->props.count; i++)
    {
        pe->particles[i].index = i;
        pe->particles[i].angle = // Notice the "-1", we want the end angle to be inclusive
            lerp(pe->props.angle_limits.x, pe->props.angle_limits.y, (float)i / (float)(pe->props.count - 1));

        pe->particles[i].angle += rand_range(-pe->props.angle_offset, pe->props.angle_offset);
        pe->particles[i].speed_offset = pe->props.speed * rand_range(-pe->props.speed_offset, pe->props.speed_offset);

        pe->positions[i] = pe->emit_point;
    }

    return pe;
}

static void particle_emitter_update(ParticleEmitter *ps, float dt)
{
    for (uint32_t i = 0; i < ps->props.count; i++)
    {
        Vec2 dir = vec2_new((float)cos(ps->particles[i].angle * DEG2RAD), (float)sin(ps->particles[i].angle * DEG2RAD));

        float speed = ps->props.speed + ps->particles[i].speed_offset;
        ps->positions[i] = vec2_add(ps->positions[i], vec2_scale(dir, speed * dt));
    }

    ps->life += dt;
    ps->transparency = (ps->props.lifetime - ps->life) / ps->props.lifetime;
    ps->isAlive = ps->life < ps->props.lifetime;
}

static void particle_emitter_deinit(ParticleEmitter *ps)
{
    free(ps->positions);
    free(ps->particles);
    free(ps);
}

static ParticleRenderUnit *render_unit_particle_init(size_t particle_count, shader_handle_t shader,
                                                     const char *texture_file_name)
{
    ParticleRenderUnit *ru = (ParticleRenderUnit *)malloc(sizeof(ParticleRenderUnit));

    // TODO @CLEANUP: VLAs would simplify this allocation
    float single_particle_vert[8] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
    size_t vert_data_len = particle_count * sizeof(single_particle_vert);
    ru->vert_data = (float *)malloc(vert_data_len);

    float single_particle_uvs[8] = {0, 0, 1, 0, 1, 1, 0, 1};
    size_t uv_data_len = particle_count * sizeof(single_particle_uvs);
    float *uv_data = (float *)malloc(uv_data_len);

    uint32_t single_particle_index[6] = {0, 1, 2, 0, 2, 3};
    size_t index_data_len = particle_count * sizeof(single_particle_index);
    uint32_t *index_data = (uint32_t *)malloc(index_data_len);

    for (uint32_t i = 0; i < (uint32_t)particle_count; i++)
    {
        // Learning: '+' operator for pointers doesn't increment by bytes.
        // The increment amount is of the pointer's type. So for this one above, it increments 8 floats.

        memcpy(ru->vert_data + i * 8, single_particle_vert, sizeof(single_particle_vert));

        uint32_t particle_index_at_i[6] = {
            single_particle_index[0] + (i * 4), single_particle_index[1] + (i * 4), single_particle_index[2] + (i * 4),
            single_particle_index[3] + (i * 4), single_particle_index[4] + (i * 4), single_particle_index[5] + (i * 4),
        };
        memcpy(index_data + i * 6, particle_index_at_i, sizeof(particle_index_at_i));

        memcpy(uv_data + i * 8, single_particle_uvs, sizeof(single_particle_uvs));
    }

    ru->vert_data_len = (uint32_t)vert_data_len;
    ru->index_count = (uint32_t)index_data_len;
    ru->shader = shader;

    glGenVertexArrays(1, &(ru->vao));
    glGenBuffers(1, &(ru->vbo));
    glGenBuffers(1, &(ru->ibo));
    glGenBuffers(1, &(ru->uv_bo));

    glBindVertexArray(ru->vao);

    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)vert_data_len, ru->vert_data, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, ru->uv_bo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)uv_data_len, uv_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)index_data_len, index_data, GL_STATIC_DRAW);

    glGenTextures(1, &(ru->texture));
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, channel_count;
    stbi_set_flip_vertically_on_load(true);
    uint8_t *data = stbi_load(texture_file_name, &width, &height, &channel_count, 0);
    assert(data != NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    free(index_data);
    free(uv_data);

    return ru;
}

static void render_unit_particle_draw(ParticleRenderUnit *ru, ParticleEmitter *pe)
{
    glUseProgram(ru->shader);
    glBindVertexArray(ru->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);

    float half_particle_size = pe->props.size * 0.5f;
    for (uint32_t i = 0; i < pe->props.count; i++)
    {
        Vec2 particle_pos = pe->positions[i];
        ru->vert_data[(i * 8) + 0] = particle_pos.x - half_particle_size;
        ru->vert_data[(i * 8) + 1] = particle_pos.y - half_particle_size;
        ru->vert_data[(i * 8) + 2] = particle_pos.x + half_particle_size;
        ru->vert_data[(i * 8) + 3] = particle_pos.y - half_particle_size;
        ru->vert_data[(i * 8) + 4] = particle_pos.x + half_particle_size;
        ru->vert_data[(i * 8) + 5] = particle_pos.y + half_particle_size;
        ru->vert_data[(i * 8) + 6] = particle_pos.x - half_particle_size;
        ru->vert_data[(i * 8) + 7] = particle_pos.y + half_particle_size;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, ru->vert_data_len, ru->vert_data);

    glBindTexture(GL_TEXTURE_2D, ru->texture);
    shader_set_float(ru->shader, "u_alpha", pe->transparency);
    glDrawElements(GL_TRIANGLES, (GLsizeiptr)ru->index_count, GL_UNSIGNED_INT, 0);
}

static void render_unit_particle_deinit(ParticleRenderUnit *ru)
{
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->uv_bo));
    glDeleteBuffers(1, &(ru->ibo));

    glDeleteProgram(ru->shader);
    glDeleteTextures(1, &ru->texture);

    free(ru->vert_data);
    free(ru);
}

static ParticleSystem particle_system_create(ParticleProps *props, Renderer *renderer, Vec2 emit_point)
{
    ParticleSystem ps;
    shader_handle_t particle_shader = load_shader("src/world.glsl"); // Using world shader for now
    ps.emitter = particle_emitter_init(*props, emit_point);
    ps.render_unit = render_unit_particle_init(props->count, particle_shader, "assets/Ball.png");
    ps.emitter->isAlive = true; // TODO @INCOMPLETE: We might want make this alive later

    shader_handle_t curr_particle_shader = ps.render_unit->shader;
    glUseProgram(curr_particle_shader);
    Mat4 mat_identity = mat4_identity();
    shader_set_mat4(curr_particle_shader, "u_model", &mat_identity);
    shader_set_mat4(curr_particle_shader, "u_view", &(renderer->view));
    shader_set_mat4(curr_particle_shader, "u_proj", &(renderer->proj));

    return ps;
}

static ParticleSystemRegistry particle_system_registry_create(void)
{
    ParticleSystemRegistry reg;
    reg.system_capacity = 10;
    reg.array_ptr = (ParticleSystem *)calloc(reg.system_capacity, sizeof(ParticleSystem));
    reg.system_count = 0;
    return reg;
}

static void particle_system_registry_add(ParticleSystemRegistry *reg, ParticleSystem ps)
{
    if (reg->system_count == reg->system_capacity)
    {
        printf("too many particle systems\n");
        return;
    }

    reg->array_ptr[reg->system_count] = ps;
    reg->system_count++;
}

static bool particle_system_is_equal(ParticleSystem a, ParticleSystem b)
{
    return a.emitter == b.emitter && a.render_unit == b.render_unit;
}

static ParticleSystem particle_system_registry_get(ParticleSystemRegistry *reg, size_t index)
{
    return reg->array_ptr[index];
}

static void particle_system_registry_remove(ParticleSystemRegistry *reg, ParticleSystem ps)
{
    for (size_t i = 0; i < reg->system_count; i++)
    {
        if (particle_system_is_equal(reg->array_ptr[i], ps))
        {
            for (size_t j = i; j < reg->system_count - 1; j++) // Shift the rest of the array
            {
                reg->array_ptr[j] = reg->array_ptr[j + 1];
            }

            reg->system_count--;
            return;
        }
    }
}

static void particle_system_registry_deinit(ParticleSystemRegistry *reg)
{
    for (size_t i = 0; i < reg->system_count; i++)
    {
        particle_emitter_deinit(reg->array_ptr[i].emitter);
        render_unit_particle_deinit(reg->array_ptr[i].render_unit);
    }

    free(reg);
}
