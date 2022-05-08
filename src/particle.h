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
    ParticleProps props;
    f32 life;
    Vec2 emit_point;
    f32 transparency;
    bool isAlive;
    u8 _padding[7];
};

struct ParticleRenderUnit {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t uv_bo;
    buffer_handle_t ibo;
    u32 index_count;
    shader_handle_t shader;
    texture_handle_t texture;
    u32 vert_data_len;
    f32 *vert_data;
};

struct ParticlePropRegistry {
    ParticleProps pad_hit_right;
    ParticleProps pad_hit_left;
};

static ParticlePropRegistry particle_prop_registry_create(void) {
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

static ParticleSource particle_source_init(ParticleProps *props, Vec2 emit_point) {
    ParticleSource pe;
    pe.positions = (Vec2 *)malloc(props->count * sizeof(Vec2));
    pe.particles = (Particle *)malloc(props->count * sizeof(Particle));
    pe.life = 0;
    pe.isAlive = false;
    pe.props = *props; // TODO @SPEED: It's better to keep a pointer to props?
    pe.emit_point = emit_point;

    for (u32 i = 0; i < pe.props.count; i++) {
        pe.particles[i].index = i;
        pe.particles[i].angle = // Notice the "-1", we want the end angle to be inclusive
            lerp(pe.props.angle_limits.x, pe.props.angle_limits.y, (f32)i / (f32)(pe.props.count - 1));

        pe.particles[i].angle += rand_range(-pe.props.angle_offset, pe.props.angle_offset);
        pe.particles[i].speed_offset = pe.props.speed * rand_range(-pe.props.speed_offset, pe.props.speed_offset);

        pe.positions[i] = pe.emit_point;
    }

    return pe;
}

static void particle_source_update(ParticleSource *ps, f32 dt) {
    for (u32 i = 0; i < ps->props.count; i++) {
        Vec2 dir =
            vec2_new((f32)cos(ps->particles[i].angle * DEG2RAD), (f32)sin(ps->particles[i].angle * DEG2RAD));

        f32 speed = ps->props.speed + ps->particles[i].speed_offset;
        ps->positions[i] = ps->positions[i] + dir * (speed * dt);
    }

    ps->life += dt;
    ps->transparency = (ps->props.lifetime - ps->life) / ps->props.lifetime;
    ps->isAlive = ps->life < ps->props.lifetime;
}

static void particle_source_deinit(ParticleSource *pe) {
    free(pe->positions);
    free(pe->particles);
}

static ParticleRenderUnit render_unit_particle_init(usize particle_count, shader_handle_t shader,
                                                    const char *texture_file_name) {
    ParticleRenderUnit ru;

    // TODO @CLEANUP: VLAs would simplify this allocation
    f32 single_particle_vert[8] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
    usize vert_data_len = particle_count * sizeof(single_particle_vert);
    ru.vert_data = (f32 *)malloc(vert_data_len);

    f32 single_particle_uvs[8] = {0, 0, 1, 0, 1, 1, 0, 1};
    usize uv_data_len = particle_count * sizeof(single_particle_uvs);
    f32 *uv_data = (f32 *)malloc(uv_data_len);

    u32 single_particle_index[6] = {0, 1, 2, 0, 2, 3};
    usize index_data_len = particle_count * sizeof(single_particle_index);
    u32 *index_data = (u32 *)malloc(index_data_len);

    for (u32 i = 0; i < (u32)particle_count; i++) {
        // Learning: '+' operator for pointers doesn't increment by bytes.
        // The increment amount is of the pointer's type. So for this one above, it increments 8 f32s.

        memcpy(ru.vert_data + i * 8, single_particle_vert, sizeof(single_particle_vert));

        u32 particle_index_at_i[6] = {
            single_particle_index[0] + (i * 4), single_particle_index[1] + (i * 4),
            single_particle_index[2] + (i * 4), single_particle_index[3] + (i * 4),
            single_particle_index[4] + (i * 4), single_particle_index[5] + (i * 4),
        };
        memcpy(index_data + i * 6, particle_index_at_i, sizeof(particle_index_at_i));

        memcpy(uv_data + i * 8, single_particle_uvs, sizeof(single_particle_uvs));
    }

    ru.vert_data_len = (u32)vert_data_len;
    ru.index_count = (u32)index_data_len;
    ru.shader = shader;

    glGenVertexArrays(1, &(ru.vao));
    glGenBuffers(1, &(ru.vbo));
    glGenBuffers(1, &(ru.ibo));
    glGenBuffers(1, &(ru.uv_bo));

    glBindVertexArray(ru.vao);

    glBindBuffer(GL_ARRAY_BUFFER, ru.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)vert_data_len, ru.vert_data, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, ru.uv_bo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)uv_data_len, uv_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)index_data_len, index_data, GL_STATIC_DRAW);

    glGenTextures(1, &(ru.texture));
    glBindTexture(GL_TEXTURE_2D, ru.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    i32 width, height, channel_count;
    stbi_set_flip_vertically_on_load(true);
    u8 *data = stbi_load(texture_file_name, &width, &height, &channel_count, 0);
    assert(data != NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    free(index_data);
    free(uv_data);

    return ru;
}

static void render_unit_particle_draw(ParticleRenderUnit *ru, ParticleSource *pe) {
    glUseProgram(ru->shader);
    glBindVertexArray(ru->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);

    f32 half_particle_size = pe->props.size * 0.5f;
    for (u32 i = 0; i < pe->props.count; i++) {
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
    shader_set_f32(ru->shader, "u_alpha", pe->transparency);
    glDrawElements(GL_TRIANGLES, (GLsizeiptr)ru->index_count, GL_UNSIGNED_INT, 0);
}

static void render_unit_particle_deinit(ParticleRenderUnit *ru) {
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->uv_bo));
    glDeleteBuffers(1, &(ru->ibo));

    glDeleteProgram(ru->shader);
    glDeleteTextures(1, &ru->texture);

    free(ru->vert_data);
}
