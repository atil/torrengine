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

    explicit ParticleSource(const ParticleProps &props, Vec2 emit_point)
        : props(props), emit_point(emit_point) {
        positions = new Vec2[props.count];
        particles = new Particle[props.count];

        life = 0;

        // TODO @INCOMPLETE: Probably we'll pool particle sources and keep them "dead" until we instantiate
        // one
        is_alive = true;

        for (u32 i = 0; i < props.count; i++) {
            particles[i].index = i;
            particles[i].angle = // Notice the "-1", we want the end angle to be inclusive
                lerp(props.angle_limits.x, props.angle_limits.y, (f32)i / (f32)(props.count - 1));

            particles[i].angle += rand_range(-props.angle_offset, props.angle_offset);
            particles[i].speed_offset = props.speed * rand_range(-props.speed_offset, props.speed_offset);

            positions[i] = emit_point;
        }
    }

    ParticleSource(ParticleSource &&rhs)
        : props(rhs.props), life(rhs.life), emit_point(rhs.emit_point), transparency(rhs.transparency),
          is_alive(rhs.is_alive) {
        positions = rhs.positions;
        particles = rhs.particles;
        rhs.positions = nullptr;
        rhs.particles = nullptr;
    }

    ParticleSource &operator=(ParticleSource &&rhs) {
        positions = rhs.positions;
        particles = rhs.particles;
        rhs.positions = nullptr;
        rhs.particles = nullptr;
        return *this;
    }

    ParticleSource(const ParticleSource &rhs) = delete;
    ParticleSource &operator=(const ParticleSource &rhs) = delete;

    ~ParticleSource() {
        delete positions;
        delete particles;
    }

    void update(f32 dt) {
        for (u32 i = 0; i < props.count; i++) {
            Vec2 dir = Vec2((f32)cos(particles[i].angle * DEG2RAD), (f32)sin(particles[i].angle * DEG2RAD));

            f32 speed = props.speed + particles[i].speed_offset;
            positions[i] = positions[i] + dir * (speed * dt);
        }

        life += dt;
        transparency = (props.lifetime - life) / props.lifetime;
        is_alive = life < props.lifetime;
    }
};

struct ParticleRenderUnit {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t uv_bo;
    buffer_handle_t ibo;
    u32 index_count;
    shader_handle_t shader;
    texture_handle_t texture;
    u32 vert_data_len; // TODO @CLEANUP: Why is this u32?
    f32 *vert_data;

    explicit ParticleRenderUnit(usize particle_count, shader_handle_t shader, const char *texture_file_name)
        : shader(shader) {
        // TODO @CLEANUP: VLAs would simplify this allocation
        f32 single_particle_vert[8] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
        vert_data_len = (u32)particle_count * sizeof(single_particle_vert);
        vert_data = (f32 *)malloc(vert_data_len);

        f32 single_particle_uvs[8] = {0, 0, 1, 0, 1, 1, 0, 1};
        usize uv_data_len = particle_count * sizeof(single_particle_uvs);
        f32 *uv_data = (f32 *)malloc(uv_data_len);

        u32 single_particle_index[6] = {0, 1, 2, 0, 2, 3};
        usize index_data_len = particle_count * sizeof(single_particle_index);
        u32 *index_data = (u32 *)malloc(index_data_len);

        for (u32 i = 0; i < (u32)particle_count; i++) {
            // Learning: '+' operator for pointers doesn't increment by bytes.
            // The increment amount is of the pointer's type. So for this one above, it increments 8 f32s.

            memcpy(vert_data + i * 8, single_particle_vert, sizeof(single_particle_vert));

            u32 particle_index_at_i[6] = {
                single_particle_index[0] + (i * 4), single_particle_index[1] + (i * 4),
                single_particle_index[2] + (i * 4), single_particle_index[3] + (i * 4),
                single_particle_index[4] + (i * 4), single_particle_index[5] + (i * 4),
            };
            memcpy(index_data + i * 6, particle_index_at_i, sizeof(particle_index_at_i));

            memcpy(uv_data + i * 8, single_particle_uvs, sizeof(single_particle_uvs));
        }

        index_count = (u32)index_data_len;

        glGenVertexArrays(1, &(vao));
        glGenBuffers(1, &(vbo));
        glGenBuffers(1, &(ibo));
        glGenBuffers(1, &(uv_bo));

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizei)vert_data_len, vert_data, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);

        glBindBuffer(GL_ARRAY_BUFFER, uv_bo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizei)uv_data_len, uv_data, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)index_data_len, index_data, GL_STATIC_DRAW);

        glGenTextures(1, &(texture));
        glBindTexture(GL_TEXTURE_2D, texture);
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
    }

    ParticleRenderUnit(ParticleRenderUnit &&rhs)
        : vao(rhs.vao), vbo(rhs.vbo), ibo(rhs.ibo), shader(rhs.shader), texture(rhs.texture) {

        rhs.vao = 0;
        rhs.vbo = 0;
        rhs.ibo = 0;
        rhs.shader = 0;
        rhs.texture = 0;

        vert_data = rhs.vert_data;
        rhs.vert_data = nullptr;
    }

    ParticleRenderUnit &operator=(ParticleRenderUnit &&rhs) {
        vert_data = rhs.vert_data;
        rhs.vert_data = nullptr;
        return *this;
    }

    ParticleRenderUnit(const ParticleRenderUnit &rhs) = delete;
    ParticleRenderUnit &operator=(const ParticleRenderUnit &rhs) = delete;

    ~ParticleRenderUnit() {
        glDeleteVertexArrays(1, &(vao));
        glDeleteBuffers(1, &(vbo));
        glDeleteBuffers(1, &(uv_bo));
        glDeleteBuffers(1, &(ibo));

        glDeleteProgram(shader); // This shader is created for this renderUnit specifically
        glDeleteTextures(1, &texture);

        free(vert_data);
    }

    void draw(const ParticleSource &ps) {
        glUseProgram(shader);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        f32 half_particle_size = ps.props.size * 0.5f;
        for (u32 i = 0; i < ps.props.count; i++) {
            Vec2 particle_pos = ps.positions[i];
            vert_data[(i * 8) + 0] = particle_pos.x - half_particle_size;
            vert_data[(i * 8) + 1] = particle_pos.y - half_particle_size;
            vert_data[(i * 8) + 2] = particle_pos.x + half_particle_size;
            vert_data[(i * 8) + 3] = particle_pos.y - half_particle_size;
            vert_data[(i * 8) + 4] = particle_pos.x + half_particle_size;
            vert_data[(i * 8) + 5] = particle_pos.y + half_particle_size;
            vert_data[(i * 8) + 6] = particle_pos.x - half_particle_size;
            vert_data[(i * 8) + 7] = particle_pos.y + half_particle_size;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, vert_data_len, vert_data);

        glBindTexture(GL_TEXTURE_2D, texture);
        shader_set_f32(shader, "u_alpha", ps.transparency);
        glDrawElements(GL_TRIANGLES, (GLsizeiptr)index_count, GL_UNSIGNED_INT, 0);
    }
};

struct ParticlePropRegistry {
    ParticleProps pad_hit_right;
    ParticleProps pad_hit_left;
};

static ParticlePropRegistry particle_prop_registry_create(void) {
    ParticlePropRegistry reg;
    reg.pad_hit_right.angle_limits = Vec2(90, 270);
    reg.pad_hit_right.count = 5;
    reg.pad_hit_right.lifetime = 1;
    reg.pad_hit_right.speed = 1;
    reg.pad_hit_right.angle_offset = 10;
    reg.pad_hit_right.speed_offset = 0.1f;
    reg.pad_hit_right.size = 0.3f;

    reg.pad_hit_left.angle_limits = Vec2(-90, 90);
    reg.pad_hit_left.count = 5;
    reg.pad_hit_left.lifetime = 1;
    reg.pad_hit_left.speed = 1;
    reg.pad_hit_left.angle_offset = 10;
    reg.pad_hit_left.speed_offset = 0.1f;
    reg.pad_hit_left.size = 0.3f;

    return reg;
}
