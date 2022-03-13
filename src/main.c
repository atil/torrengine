#define GLEW_STATIC                    // Statically linking glew
#define GLFW_DLL                       // Dynamically linking glfw
#define GLFW_INCLUDE_NONE              // Disable including dev environment header
#define STB_TRUETYPE_IMPLEMENTATION    // stb requires these
#define STB_IMAGE_WRITE_IMPLEMENTATION // TODO @CLEANUP: Only needed for font debug
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
#include <stb_image_write.h> // TODO @CLEANUP: Only needed for font debug
#include <stb_truetype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#include "util.h"
#include "tomath.h"
#include "text.h"
#include "render.h"
#include "game.h"
#pragma warning(pop)

#define WIDTH 640
#define HEIGHT 480

typedef struct WAV_HEADER
{
    /* RIFF Chunk Descriptor */
    uint8_t RIFF[4];    // RIFF Header Magic header
    uint32_t ChunkSize; // RIFF Chunk Size
    uint8_t WAVE[4];    // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t fmt[4];         // FMT header
    uint32_t Subchunk1Size; // Size of the fmt chunk
    uint16_t AudioFormat;   // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t NumOfChan;     // Number of channels 1=Mono 2=Sterio
    uint32_t SamplesPerSec; // Sampling Frequency in Hz
    uint32_t bytesPerSec;   // bytes per second
    uint16_t blockAlign;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample; // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t Subchunk2ID[4]; // "data"  string
    uint32_t Subchunk2Size; // Sampled data length
} wav_hdr_t;

int getFileSize(FILE *inFile)
{
    int fileSize = 0;
    fseek(inFile, 0, SEEK_END);

    fileSize = ftell(inFile);

    fseek(inFile, 0, SEEK_SET);
    return fileSize;
}

void check_al_error(char *msg)
{
    ALCenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        printf("AL error: %s\n", msg);
    }
}

int main(void)
{
    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    FontData font_data;
    text_init(&font_data);

    //
    // Sound schenanigans
    //
    wav_hdr_t wavHeader;
    size_t headerSize = sizeof(wav_hdr_t), filelength = 0;
    FILE *wavFile = fopen("assets/test.wav", "r");
    size_t header_bytes_read = fread(&wavHeader, 1, headerSize, wavFile);
    uint16_t bytesPerSample = wavHeader.bitsPerSample / 8;
    uint64_t numSamples = wavHeader.ChunkSize / bytesPerSample; // How many samples are in the wav file?

    uint8_t *wav_buffer = (uint8_t *)malloc(wavHeader.Subchunk2Size);
    fseek(wavFile, 44, SEEK_SET);
    size_t bytesRead = fread(wav_buffer, wavHeader.Subchunk2Size, 1, wavFile);
    printf("error? :%d \n", feof(wavFile));
    // stuff
    printf("read %zd bytes\n", bytesRead);

    const char *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *al_device = alcOpenDevice(defaultDeviceName);
    ALCcontext *al_context = alcCreateContext(al_device, NULL);
    alcMakeContextCurrent(al_context);
    ALuint al_source;
    alGenSources((ALuint)1, &al_source);
    alSourcef(al_source, AL_PITCH, 1);
    alSourcef(al_source, AL_GAIN, 1);
    alSource3f(al_source, AL_POSITION, 0, 0, 0);
    alSource3f(al_source, AL_VELOCITY, 0, 0, 0);
    alSourcei(al_source, AL_LOOPING, AL_FALSE);
    check_al_error("source set");

    ALuint al_buffer;
    alGenBuffers(1, &al_buffer);
    alBufferData(al_buffer, AL_FORMAT_MONO16, wav_buffer, wavHeader.Subchunk2Size, wavHeader.SamplesPerSec);
    check_al_error("buffer data");
    alSourcei(al_source, AL_BUFFER, al_buffer);
    check_al_error("buffer bind");

    alSourcePlay(al_source);
    check_al_error("play");

    ALint source_state;
    alGetSourcei(al_source, AL_SOURCE_STATE, &source_state);
    check_al_error("source get 1");
    while (source_state == AL_PLAYING)
    {
        alGetSourcei(al_source, AL_SOURCE_STATE, &source_state);
        check_al_error("source get 2");
    }

    free(wav_buffer);
    filelength = getFileSize(wavFile);
    fclose(wavFile);

    alDeleteSources(1, &al_source);
    alDeleteBuffers(1, &al_buffer);
    al_device = alcGetContextsDevice(al_context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(al_context);
    alcCloseDevice(al_device);

    //
    // GameObject rendering
    //

    RenderUnit pad1_ru;
    RenderUnit pad2_ru;
    RenderUnit ball_ru;

    shader_handle_t world_shader = load_shader("src/world.glsl");

    // Holds position (vec3) and UV (vec2)
    float unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                                 0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f};
    uint32_t unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    render_unit_init(&pad1_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader);
    render_unit_init(&pad2_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader);
    render_unit_init(&ball_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader);

    //
    // UI
    //

    shader_handle_t ui_shader = load_shader("src/ui.glsl");

    UiRenderUnit ui_ru_score;
    render_unit_ui_alloc(&ui_ru_score, ui_shader, &font_data);
    TextTransform text_transform_score;
    text_transform_score.anchor = vec2_new(-0.9f, -0.9f);
    text_transform_score.scale = vec2_new(0.1f, 0.5f);
    render_unit_ui_update(&ui_ru_score, &font_data, "0", text_transform_score);

    UiRenderUnit ui_ru_intermission;
    render_unit_ui_alloc(&ui_ru_intermission, ui_shader, &font_data);
    TextTransform text_transform_intermission;
    text_transform_intermission.anchor = vec2_new(-0.5f, 0.0f);
    text_transform_intermission.scale = vec2_new(1.0f, 0.5f);
    render_unit_ui_update(&ui_ru_intermission, &font_data, "Game Over", text_transform_intermission);

    //
    // View-projection matrices
    //

    // We translate this matrix by the cam position
    Mat4 view = mat4_identity();
    const float cam_size = 5.0f;
    const float aspect = (float)WIDTH / (float)HEIGHT;
    Mat4 proj = mat4_ortho(-aspect * cam_size, aspect * cam_size, -cam_size, cam_size, -0.001f, 100.0f);

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &view);
    shader_set_mat4(world_shader, "u_proj", &proj);

    //
    // Game objects
    //

    PongGameConfig config;
    config.area_extents = vec2_new(cam_size * aspect, cam_size);
    config.pad_size = vec2_new(0.1f, 2.0f);
    config.ball_speed = 4.0f;
    config.distance_from_center = 4.0f;
    config.pad_move_speed = 10.0f;
    config.game_speed_increase_coeff = 0.05f;

    PongGame game;
    game_init(&game, &config);

    float game_time = (float)glfwGetTime();
    float dt = 0.0f;
    bool is_game_running = true;
    while (!glfwWindowShouldClose(window))
    {
        dt = (float)glfwGetTime() - game_time;
        game_time = (float)glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // TODO @CLEANUP: This looks bad
        PongGameUpdateResult result;
        result.is_game_over = false;
        result.did_score = false;

        if (is_game_running)
        {
            result = game_update(dt, &game, &config, window);
            if (result.is_game_over)
            {
                is_game_running = false;
            }
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Game draw
        glUseProgram(world_shader);
        glBindVertexArray(pad1_ru.vao);
        shader_set_mat4(world_shader, "u_model", &game.pad1_go.transform);
        shader_set_float3(world_shader, "u_rectcolor", 1, 1, 0);
        glDrawElements(GL_TRIANGLES, pad1_ru.index_count, GL_UNSIGNED_INT, 0);

        glBindVertexArray(pad1_ru.vao);
        shader_set_mat4(world_shader, "u_model", &game.pad2_go.transform);
        shader_set_float3(world_shader, "u_rectcolor", 0, 1, 1);
        glDrawElements(GL_TRIANGLES, pad2_ru.index_count, GL_UNSIGNED_INT, 0);

        glBindVertexArray(ball_ru.vao);
        shader_set_mat4(world_shader, "u_model", &game.ball_go.transform);
        shader_set_float3(world_shader, "u_rectcolor", 1, 0, 1);
        // TODO @DOCS: How can that last parameter be zero
        glDrawElements(GL_TRIANGLES, ball_ru.index_count, GL_UNSIGNED_INT, 0);

        // UI draw
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ui_ru_score.texture);
        glUseProgram(ui_ru_score.shader);
        glBindVertexArray(ui_ru_score.vao);
        if (result.did_score)
        {
            char int_str_buffer[sizeof(uint32_t)]; // TODO @ROBUSTNESS: Assert that it's a 32-bit integer
            sprintf_s(int_str_buffer, sizeof(char) * sizeof(uint32_t), "%d", game.score);

            render_unit_ui_update(&ui_ru_score, &font_data, int_str_buffer, text_transform_score);
        }
        glDrawElements(GL_TRIANGLES, ui_ru_score.index_count, GL_UNSIGNED_INT, 0);

        if (!is_game_running) // Draw game over
        {
            glBindVertexArray(ui_ru_intermission.vao);
            glDrawElements(GL_TRIANGLES, ui_ru_intermission.index_count, GL_UNSIGNED_INT, 0);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
                game_init(&game, &config);
                is_game_running = true;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    text_deinit(&font_data);

    render_unit_deinit(&pad1_ru);
    render_unit_deinit(&pad2_ru);
    render_unit_deinit(&ball_ru);
    glDeleteProgram(world_shader); // TODO @CLEANUP: We'll have some sort of batching probably

    render_unit_ui_deinit(&ui_ru_score);
    render_unit_ui_deinit(&ui_ru_intermission);
    glDeleteProgram(ui_shader);

    glfwTerminate();
    return 0;
}
