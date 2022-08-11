#pragma once

// #define SFX_DISABLED // TODO @CLEANUP: Convert this to a cmdline argument

#ifdef SFX_DISABLED
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

#include "types.h"
#include <memory>
#include <string>
#include <vector>

enum class SfxId {
    SfxStart,
    SfxHitPad,
    SfxHitWall,
    SfxGameOver
};

struct SfxAsset {
    SfxId id;
    u8 _padding[4];
    std::string file_name;
    SfxAsset(SfxId id, const std::string &file_name) : id(id), file_name(file_name) {
    }
};

class Sfx {
    std::unique_ptr<struct SfxPlayer> player;

  public:
    PREVENT_COPY_MOVE(Sfx);
    Sfx(std::vector<SfxAsset> assets);
    ~Sfx();
    void play(SfxId id);
};

#ifdef SFX_DISABLED
#pragma warning(pop)
#endif
