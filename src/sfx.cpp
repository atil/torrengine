#include <cstdio>
#include <string>
#include <cassert>
#include <utility>
#include "sfx_p.h"

Sfx::Sfx(std::vector<SfxAsset> assets) {
    player = std::make_unique<SfxPlayer>(assets);
}

Sfx::~Sfx() {
    // Needed because we can't declare a unique_ptr with an incomplete type. It's destructor becomes
    // problematic. We need to implement a destructor where the type is complete
    // See: https://stackoverflow.com/a/9954553/4894526
}

void Sfx::play(SfxId id) {
    player->play(id);
}

void SfxPlayer::check_al_error(const std::string &msg) {
    ALCenum error = alGetError();
    if (error != AL_NO_ERROR) {
        printf("AL error [%d]: %s\n", error, msg.c_str());
    }
}

SfxPlayer::SfxPlayer(std::vector<SfxAsset> assets) {
#ifndef SFX_DISABLED
    const char *default_device_name = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    device = alcOpenDevice(default_device_name);
    context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    source_startgame = create_source();
    source_gameover = create_source();
    source_objects = create_source();

    buffers.reserve(assets.size());
    for (const SfxAsset &asset : assets) {
        sfx_buffer_handle_t handle = create_buffer_with_file(asset.file_name);
        auto pair = std::make_pair(asset.id, handle);
        buffers.insert(pair);
    }

    // start from here:
    // - use "buffers" instead of these below
    // - noncopyable macro
    // - particles to its own module
#endif
}

SfxPlayer::~SfxPlayer() {
#ifndef SFX_DISABLED
    alDeleteSources(1, &(source_startgame));
    alDeleteSources(1, &(source_gameover));
    alDeleteSources(1, &(source_objects));

    for (const auto &pair : buffers) {
        alDeleteBuffers(1, &pair.second);
    }

    device = alcGetContextsDevice(context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
#endif
}

sfx_buffer_handle_t SfxPlayer::create_buffer_with_file(const std::string &file_name) {
    WavHeader wav_header;
    usize wav_header_size = sizeof(WavHeader);
    FILE *wav_file = fopen(file_name.c_str(), "r");
    assert(wav_file);
    fread(&wav_header, 1, wav_header_size, wav_file);

    u8 *wav_buffer = (u8 *)malloc(wav_header.sample_data_len);
    fseek(wav_file, 44, SEEK_SET);
    fread(wav_buffer, wav_header.sample_data_len, 1, wav_file);

    sfx_buffer_handle_t buffer_handle;
    alGenBuffers(1, &buffer_handle);
    alBufferData(buffer_handle, AL_FORMAT_MONO16, wav_buffer, (ALsizei)wav_header.sample_data_len,
                 (ALsizei)wav_header.sample_freq);

    free(wav_buffer);
    fclose(wav_file);

    SfxPlayer::check_al_error("buffer created with file");

    return buffer_handle;
}

sfx_source_handle_t SfxPlayer::create_source() {
    sfx_source_handle_t source_handle;
    alGenSources((ALuint)1, &source_handle);
    alSourcef(source_handle, AL_PITCH, 1);
    alSourcef(source_handle, AL_GAIN, 0.2f);
    alSource3f(source_handle, AL_POSITION, 0, 0, 0);
    alSource3f(source_handle, AL_VELOCITY, 0, 0, 0);
    alSourcei(source_handle, AL_LOOPING, AL_FALSE);
    SfxPlayer::check_al_error("source set");

    return source_handle;
}

void SfxPlayer::play(SfxId id) {

#ifndef SFX_DISABLED
    if (buffers.find(id) == buffers.end()) {
        printf("Can't play sound. Unrecognized id: %d\n", id);
        return;
    }

    sfx_buffer_handle_t buffer = buffers[id];

    sfx_source_handle_t source = 0;
    switch (id) {
    case SfxId::SfxStart:
        source = source_startgame;
        break;
    case SfxId::SfxGameOver:
        source = source_gameover;
        break;
    case SfxId::SfxHitPad:
    case SfxId::SfxHitWall:
        source = source_objects;
        break;
    default:
        printf("Unable to play sfx. Unrecognized id: %d\n", id);
        return;
    }

    alSourcei(source, AL_BUFFER, (ALint)buffer);
    SfxPlayer::check_al_error("source");
    alSourcePlay(source);
    SfxPlayer::check_al_error("source play");
#endif

    // Checking if the source is still playing:
    // ALint source_state;
    // alGetSourcei(sfx->source, AL_SOURCE_STATE, &source_state);
    // while (source_state == AL_PLAYING)
    // {
    //     alGetSourcei(al_source, AL_SOURCE_STATE, &source_state);
    //     check_al_error("source get 2");
    // }
}