#include <AL/al.h>
#include <AL/alc.h>
#include <string>
#include "sfx.h"
#include <unordered_map>

typedef ALuint sfx_source_handle;
typedef ALuint sfx_buffer_handle;

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

struct SfxPlayer {

    ALCdevice *device;
    ALCcontext *context;

    sfx_source_handle source_objects;
    sfx_source_handle source_startgame;
    sfx_source_handle source_gameover;

    std::unordered_map<SfxId, sfx_buffer_handle> buffers;

    SfxPlayer(std::vector<SfxAsset> assets);
    ~SfxPlayer();

    void play(SfxId id);
    static sfx_buffer_handle create_buffer_with_file(const std::string &file_name);
    static sfx_source_handle create_source(void);
    static void check_al_error(const std::string &msg);
};
