// tasks:
// - core vectors should be: gameobjects, particles, ui.

// NOTE @DOCS: Game origin: up-left

#define GLEW_STATIC                 // Statically linking glew
#define GLFW_DLL                    // Dynamically linking glfw
#define GLFW_INCLUDE_NONE           // Disable including dev environment header
#define STB_TRUETYPE_IMPLEMENTATION // stb requires these
#define STB_IMAGE_IMPLEMENTATION

// Disable a bunch of warnings from system headers
#pragma warning(push, 0)
#pragma warning(disable : 5040) // These require extra attention for some reason
#pragma warning(disable : 5045) // Spectre thing
#include <GL/glew.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_truetype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <string>
#pragma warning(pop)

#define WIDTH 640
#define HEIGHT 480

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#pragma warning(disable : 4505) // Unreferenced functions

// Core module (lib)
#include "types.h"
#include "util.h"
#include "tomath.h"

// Engine module
#include "core.h"
#include "input.h"
#include "shader.h"
#include "ui.h"
#include "render.h"
#include "particle.h"
#include "sfx.h"

// Game module

// TODO @CLEANUP: How to deal with this? Move to its own file?
struct PongEntities {
    EntityIndex entity_ui_splash = 0;
    EntityIndex entity_ui_score = 1;
    EntityIndex entity_ui_intermission = 2;

    EntityIndex entity_world_field = 0;
    EntityIndex entity_world_pad1 = 1;
    EntityIndex entity_world_pad2 = 2;
    EntityIndex entity_world_ball = 3;
};

EntityIndex register_particle(Core *core, const ParticleProps &props, const RenderInfo &renderer, Vec2 emit_point);

#include "world.h"
#include "game.h"
#pragma warning(pop)

#pragma warning(disable : 5045) // Spectre thing

int main(void) {
    main_game();
    return 0;
}
