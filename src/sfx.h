// #define SFX_DISABLED // TODO @CLEANUP: Convert this to a cmdline argument

#ifdef SFX_DISABLED
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

typedef ALuint sfx_source_handle_t;
typedef ALuint sfx_buffer_handle_t;

enum class SfxId {
    SfxStart,
    SfxHitPad,
    SfxHitWall,
    SfxGameOver
};

struct WavHeader {
    u8 _RIFF[4];         // RIFF Header Magic header
    u32 chunk_size;      // RIFF Chunk Size
    u8 _WAVE[4];         // WAVE Header
    u8 _FMT[4];          // FMT header
    u32 subchunk1_size;  // Size of the fmt chunk
    u16 audio_format;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    u16 channel_count;   // Number of channels 1=Mono 2=Sterio
    u32 sample_freq;     // Sampling Frequency in Hz
    u32 bytes_freq;      // bytes per second
    u16 block_align;     // 2=16-bit mono, 4=16-bit stereo
    u16 bits_per_sample; // Number of bits per sample
    u8 _subchunk2_id[4]; // "data"  string
    u32 sample_data_len; // Sampled data length
};

struct Sfx {
    ALCdevice *device;
    ALCcontext *context;

    sfx_source_handle_t source_objects;
    sfx_source_handle_t source_startgame;
    sfx_source_handle_t source_gameover;
    u32 _unused_padding; // TODO @CLEANUP: Research why we need this

    // TODO @REFACTOR: These are gonna be an array
    sfx_buffer_handle_t buffer_hitpad;
    sfx_buffer_handle_t buffer_hitwall;
    sfx_buffer_handle_t buffer_startgame;
    sfx_buffer_handle_t buffer_gameover;

    Sfx() {
#ifndef SFX_DISABLED
        const char *default_device_name = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
        device = alcOpenDevice(default_device_name);
        context = alcCreateContext(device, NULL);
        alcMakeContextCurrent(context);

        source_startgame = create_source();
        source_gameover = create_source();
        source_objects = create_source();

        buffer_hitpad = create_buffer_with_file("assets/HitPad.wav");
        buffer_hitwall = create_buffer_with_file("assets/HitWall.wav");
        buffer_gameover = create_buffer_with_file("assets/GameOver.wav");
        buffer_startgame = create_buffer_with_file("assets/Start.wav");
#endif
    }

    ~Sfx() {
#ifndef SFX_DISABLED
        alDeleteSources(1, &(source_startgame));
        alDeleteSources(1, &(source_gameover));
        alDeleteSources(1, &(source_objects));
        alDeleteBuffers(1, &(buffer_startgame));
        alDeleteBuffers(1, &(buffer_gameover));
        alDeleteBuffers(1, &(buffer_hitpad));
        alDeleteBuffers(1, &(buffer_hitwall));

        device = alcGetContextsDevice(context);
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
#endif
    }

    static void check_al_error(const char *msg) {
        ALCenum error = alGetError();
        if (error != AL_NO_ERROR) {
            printf("AL error [%d]: %s\n", error, msg);
        }
    }

    static sfx_buffer_handle_t create_buffer_with_file(const char *file_name) {
        WavHeader wav_header;
        usize wav_header_size = sizeof(WavHeader);
        FILE *wav_file = fopen(file_name, "r");
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

        check_al_error("buffer created with file");

        return buffer_handle;
    }

    static sfx_source_handle_t create_source(void) {
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

    void play(SfxId id) {
#ifndef SFX_DISABLED
        sfx_buffer_handle_t buffer = 0;
        sfx_source_handle_t source = 0;
        switch (id) {
        case SfxId::SfxStart:
            buffer = buffer_startgame;
            source = source_startgame;
            break;
        case SfxId::SfxGameOver:
            buffer = buffer_gameover;
            source = source_gameover;
            break;
        case SfxId::SfxHitPad:
            buffer = buffer_hitpad;
            source = source_objects;
            break;
        case SfxId::SfxHitWall:
            buffer = buffer_hitwall;
            source = source_objects;
            break;
        default:
            printf("Unable to play sfx. Unrecognized id: %d\n", id);
            return;
        }

        if (id == SfxId::SfxStart) {
            printf("start\n");
        }

        alSourcei(source, AL_BUFFER, (ALint)buffer);
        Sfx::check_al_error("source");
        alSourcePlay(source);
        Sfx::check_al_error("source play");
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
};

#ifdef SFX_DISABLED
#pragma warning(pop)
#endif
