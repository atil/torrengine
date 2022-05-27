// TODO @CLEANUP: Maybe move these functions to render.h? To keep file count small? Maybe not, if they end up being
// totally unrelated?

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256
#define FONT_TEXT_HEIGHT 50 // In pixels

struct TextBufferData {
    f32 *vb_data;
    usize vb_len;
    u32 *ib_data;
    usize ib_len;
};

enum class TextWidthType
{
    FixedWidth,
    FreeWidth,
};

struct TextTransform {
    Vec2 anchor;
    f32 height; // In NDC
    TextWidthType width_type;
    f32 width; // In NDC

    explicit TextTransform(Vec2 anchor, f32 height, TextWidthType width_type, f32 width)
        : anchor(anchor), height(height), width_type(width_type), width(width) {
    }
};

struct FontData {
    u8 *font_bitmap;
    f32 ascent;                                 // In pixels
    f32 descent;                                // In pixels
    stbtt_bakedchar font_char_data[CHAR_COUNT]; // TODO @LEAK: This is not leaked, but research anyway

    explicit FontData(const char *ttf_path) {
        // TODO @ROBUSTNESS: Assert that it's called once
        u8 *font_bytes = (u8 *)read_file(ttf_path);
        assert(font_bytes != nullptr);
        font_bitmap = new u8[FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT];

        stbtt_BakeFontBitmap((u8 *)font_bytes, 0, FONT_TEXT_HEIGHT, font_bitmap, FONT_ATLAS_WIDTH,
                             FONT_ATLAS_HEIGHT, ' ', CHAR_COUNT, font_char_data);

        stbtt_fontinfo font_info;
        stbtt_InitFont(&font_info, font_bytes, 0);

        int ascent_int, descent_int, line_gap;
        stbtt_GetFontVMetrics(&font_info, &ascent_int, &descent_int, &line_gap);

        f32 scale = stbtt_ScaleForPixelHeight(&font_info, FONT_TEXT_HEIGHT);
        ascent = (f32)ascent_int * scale;
        descent = (f32)descent_int * scale;

        free(font_bytes);
    }

    ~FontData() {
        delete font_bitmap;
    }
};

struct Widget {
    std::string text;
    TextTransform transform;
    u8 _padding[4];
    FontData *font_data;

    explicit Widget(const char *chars, TextTransform transform, FontData *font_data)
        : text(chars), transform(transform), font_data(font_data) {
    }

    void set_str(u32 integer) {
        char int_str_buffer[32]; // TODO @ROBUSTNESS: Assert that it's a 32-bit integer
        sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", integer);
        text = int_str_buffer;
    }
};
