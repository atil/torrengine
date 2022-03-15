typedef struct
{
    ALCdevice *device;
    ALCcontext *context;
    ALuint source;
    ALuint buffer;
} Sfx;

typedef struct WAV_HEADER
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

static void sfx_init(Sfx *sfx)
{
    WavHeader wav_header;
    size_t wav_header_size = sizeof(WavHeader);
    FILE *wav_file = fopen("assets/test.wav", "r");
    assert(wav_file);
    fread(&wav_header, 1, wav_header_size, wav_file);

    uint8_t *wav_buffer = (uint8_t *)malloc(wav_header.sample_data_len);
    fseek(wav_file, 44, SEEK_SET);
    fread(wav_buffer, wav_header.sample_data_len, 1, wav_file);

    const char *default_device_name = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *al_device = alcOpenDevice(default_device_name);
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
    alBufferData(al_buffer, AL_FORMAT_MONO16, wav_buffer, wav_header.sample_data_len, wav_header.sample_freq);
    check_al_error("buffer data");
    alSourcei(al_source, AL_BUFFER, al_buffer);
    check_al_error("buffer bind");

    sfx->device = al_device;
    sfx->context = al_context;
    sfx->source = al_source;
    sfx->buffer = al_buffer;

    free(wav_buffer);
    fclose(wav_file);
}

static void sfx_play(Sfx *sfx)
{
    alSourcePlay(sfx->source);
    check_al_error("play");

    // Checking if the source is still playing
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
    alDeleteSources(1, &(sfx->source));
    alDeleteBuffers(1, &(sfx->buffer));
    sfx->device = alcGetContextsDevice(sfx->context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(sfx->context);
    alcCloseDevice(sfx->device);
}
