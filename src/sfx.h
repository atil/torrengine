typedef ALuint sfx_source_handle_t;
typedef ALuint sfx_buffer_handle_t;

typedef enum
{
    SfxStart,
    SfxHitPad,
    SfxHitWall,
    SfxGameOver
} SfxId;

typedef struct
{
    ALCdevice *device;
    ALCcontext *context;

    sfx_source_handle_t source_objects;
    sfx_source_handle_t source_game;

    sfx_buffer_handle_t buffer_hitpad;
    sfx_buffer_handle_t buffer_hitwall;
    sfx_buffer_handle_t buffer_start;
    sfx_buffer_handle_t buffer_gameover;
} Sfx;

typedef struct
{
    uint8_t _RIFF[4];         // RIFF Header Magic header
    uint32_t chunk_size;      // RIFF Chunk Size
    uint8_t _WAVE[4];         // WAVE Header
    uint8_t _FMT[4];          // FMT header
    uint32_t subchunk1_size;  // Size of the fmt chunk
    uint16_t audio_format;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t channel_count;   // Number of channels 1=Mono 2=Sterio
    uint32_t sample_freq;     // Sampling Frequency in Hz
    uint32_t bytes_freq;      // bytes per second
    uint16_t block_align;     // 2=16-bit mono, 4=16-bit stereo
    uint16_t bits_per_sample; // Number of bits per sample
    uint8_t _subchunk2_id[4]; // "data"  string
    uint32_t sample_data_len; // Sampled data length
} WavHeader;

static void check_al_error(const char *msg)
{
    ALCenum error = alGetError();
    if (error != AL_NO_ERROR)
    {
        printf("AL error: %s\n", msg);
    }
}

static sfx_buffer_handle_t create_buffer_with_file(const char *file_name)
{
    WavHeader wav_header;
    size_t wav_header_size = sizeof(WavHeader);
    FILE *wav_file = fopen(file_name, "r");
    assert(wav_file);
    fread(&wav_header, 1, wav_header_size, wav_file);

    uint8_t *wav_buffer = (uint8_t *)malloc(wav_header.sample_data_len);
    fseek(wav_file, 44, SEEK_SET);
    fread(wav_buffer, wav_header.sample_data_len, 1, wav_file);

    sfx_buffer_handle_t buffer_handle;
    alGenBuffers(1, &buffer_handle);
    alBufferData(buffer_handle, AL_FORMAT_MONO16, wav_buffer, wav_header.sample_data_len, wav_header.sample_freq);

    free(wav_buffer);
    fclose(wav_file);

    return buffer_handle;
}

static sfx_source_handle_t create_source(void)
{
    sfx_source_handle_t source_handle;
    alGenSources((ALuint)1, &source_handle);
    alSourcef(source_handle, AL_PITCH, 1);
    alSourcef(source_handle, AL_GAIN, 0.2f);
    alSource3f(source_handle, AL_POSITION, 0, 0, 0);
    alSource3f(source_handle, AL_VELOCITY, 0, 0, 0);
    alSourcei(source_handle, AL_LOOPING, AL_FALSE);
    check_al_error("source set");

    return source_handle;
}

static void sfx_init(Sfx *sfx)
{
    const char *default_device_name = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    sfx->device = alcOpenDevice(default_device_name);
    sfx->context = alcCreateContext(sfx->device, NULL);
    alcMakeContextCurrent(sfx->context);

    sfx->source_game = create_source();
    sfx->source_objects = create_source();

    sfx->buffer_hitpad = create_buffer_with_file("assets/HitPad.wav");
    sfx->buffer_hitwall = create_buffer_with_file("assets/HitWall.wav");
    sfx->buffer_gameover = create_buffer_with_file("assets/GameOver.wav");
    sfx->buffer_start = create_buffer_with_file("assets/Start.wav");
}

static void sfx_play(Sfx *sfx, SfxId id)
{
    sfx_buffer_handle_t buffer = 0;
    sfx_source_handle_t source = 0;
    switch (id)
    {
    case SfxStart:
        buffer = sfx->buffer_start;
        source = sfx->source_game;
        break;
    case SfxGameOver:
        buffer = sfx->buffer_gameover;
        source = sfx->source_game;
        break;
    case SfxHitPad:
        buffer = sfx->buffer_hitpad;
        source = sfx->source_objects;
        break;
    case SfxHitWall:
        buffer = sfx->buffer_hitwall;
        source = sfx->source_objects;
        break;
    default:
        printf("Unable to play sfx. Unrecognized id: %d\n", id);
        return;
    }
    alSourcei(source, AL_BUFFER, buffer);
    alSourcePlay(source);

    // Checking if the source is still playing:
    // ALint source_state;
    // alGetSourcei(sfx->source, AL_SOURCE_STATE, &source_state);
    // while (source_state == AL_PLAYING)
    // {
    //     alGetSourcei(al_source, AL_SOURCE_STATE, &source_state);
    //     check_al_error("source get 2");
    // }
}

static void sfx_deinit(Sfx *sfx)
{
    alDeleteSources(1, &(sfx->source_game));
    alDeleteSources(1, &(sfx->source_objects));
    alDeleteBuffers(1, &(sfx->buffer_start));
    alDeleteBuffers(1, &(sfx->buffer_gameover));
    alDeleteBuffers(1, &(sfx->buffer_hitpad));
    alDeleteBuffers(1, &(sfx->buffer_hitwall));

    sfx->device = alcGetContextsDevice(sfx->context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(sfx->context);
    alcCloseDevice(sfx->device);
}
