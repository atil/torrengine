// TODO @CLEANUP: Maybe move these functions to render.h? To keep file count small? Maybe not, if they end up being
// totally unrelated?

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256
#define FONT_TEXT_HEIGHT 50 // In pixels

typedef struct
{
    float *vb_data;
    size_t vb_len;
    uint32_t *ib_data;
    size_t ib_len;
} TextBufferData;

typedef enum
{
    FixedWidth,
    FreeWidth,
} TextWidthType;

typedef struct
{
    Vec2 anchor;
    float height; // In NDC
    TextWidthType width_type;
    float width; // In NDC
} TextTransform;

typedef struct
{
    uint8_t *font_bitmap;
    float ascent;                               // In pixels
    float descent;                              // In pixels
    stbtt_bakedchar font_char_data[CHAR_COUNT]; // TODO @LEAK: This is not leaked, but research anyway
} FontData;

static TextTransform texttransform_new(Vec2 anchor, float height, TextWidthType width_type, float width)
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
    uint8_t *font_bytes = (uint8_t *)read_file("assets/Consolas.ttf");
    font_data->font_bitmap = (uint8_t *)malloc(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * sizeof(uint8_t));

    stbtt_BakeFontBitmap((uint8_t *)font_bytes, 0, FONT_TEXT_HEIGHT, font_data->font_bitmap, FONT_ATLAS_WIDTH,
                         FONT_ATLAS_HEIGHT, ' ', CHAR_COUNT, font_data->font_char_data);

    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, font_bytes, 0);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

    float scale = stbtt_ScaleForPixelHeight(&font_info, FONT_TEXT_HEIGHT);
    font_data->ascent = ascent * scale;
    font_data->descent = descent * scale;

    free(font_bytes);
}

static void text_deinit(FontData *font_data)
{
    free(font_data->font_bitmap);
}
