// TODO @CLEANUP: Maybe move these functions to render.h? To keep file count small? Maybe not, if they end up being
// totally unrelated?

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256
#define FONT_TEXT_HEIGHT 50 // In pixels

struct TextBufferData
{
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

struct TextTransform
{
    Vec2 anchor;
    f32 height; // In NDC
    TextWidthType width_type;
    f32 width; // In NDC
};

struct FontData
{
    u8 *font_bitmap;
    f32 ascent;                                 // In pixels
    f32 descent;                                // In pixels
    stbtt_bakedchar font_char_data[CHAR_COUNT]; // TODO @LEAK: This is not leaked, but research anyway
};

static TextTransform texttransform_new(Vec2 anchor, f32 height, TextWidthType width_type, f32 width)
{
    TextTransform tx;
    tx.anchor = anchor;
    tx.height = height;
    tx.width_type = width_type;
    tx.width = width;
    return tx;
}

static void text_init(FontData *font_data)
{
    // TODO @ROBUSTNESS: Assert that it's called once
    u8 *font_bytes = (u8 *)read_file("assets/Consolas.ttf");
    assert(font_bytes != NULL);
    font_data->font_bitmap = (u8 *)malloc(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * sizeof(u8));

    stbtt_BakeFontBitmap((u8 *)font_bytes, 0, FONT_TEXT_HEIGHT, font_data->font_bitmap, FONT_ATLAS_WIDTH,
                         FONT_ATLAS_HEIGHT, ' ', CHAR_COUNT, font_data->font_char_data);

    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, font_bytes, 0);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

    f32 scale = stbtt_ScaleForPixelHeight(&font_info, FONT_TEXT_HEIGHT);
    font_data->ascent = (f32)ascent * scale;
    font_data->descent = (f32)descent * scale;

    free(font_bytes);
}

static void text_deinit(FontData *font_data)
{
    free(font_data->font_bitmap);
}
