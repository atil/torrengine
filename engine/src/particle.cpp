#include <cstdlib>
#include <cstring>
#include "common.h"
#include "tomath.h"
#include "particle.h"

ParticleSource::ParticleSource(const ParticleProps &props, Vec2 emit_point)
    : props(props), emit_point(emit_point) {
    positions = new Vec2[props.count];
    particles = new Particle[props.count];

    life = 0;

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

ParticleSource::ParticleSource(ParticleSource &&rhs)
    : props(rhs.props), life(rhs.life), emit_point(rhs.emit_point), transparency(rhs.transparency),
      is_alive(rhs.is_alive) {
    positions = rhs.positions;
    particles = rhs.particles;
    rhs.positions = nullptr;
    rhs.particles = nullptr;
}

ParticleSource::~ParticleSource() {
    delete positions;
    delete particles;
}

void ParticleSource::update(f32 dt) {
    for (u32 i = 0; i < props.count; i++) {
        Vec2 dir = Vec2((f32)cos(particles[i].angle * DEG2RAD), (f32)sin(particles[i].angle * DEG2RAD));

        f32 speed = props.speed + particles[i].speed_offset;
        positions[i] = positions[i] + dir * (speed * dt);
    }

    life += dt;
    transparency = (props.lifetime - life) / props.lifetime;
    is_alive = life < props.lifetime;
}