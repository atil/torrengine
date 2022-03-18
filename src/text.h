// TODO @CLEANUP: Maybe move these functions to render.h? To keep file count small? Maybe not, if they end up being
// totally unrelated?

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256

typedef struct
{
    float *vb_data;
    size_t vb_len;
    uint32_t *ib_data;
    size_t ib_len;
} TextBufferData;

typedef struct
{
    Vec2 anchor;
    Vec2 scale;
} TextTransform;

typedef struct
{
    uint8_t *font_bitmap;
    stbtt_bakedchar font_char_data[CHAR_COUNT]; // TODO @LEAK: Is this leaked? Research.
} FontData;

static void text_init(FontData *font_data)
{
    // TODO @ROBUSTNESS: Assert that it's called once
    uint8_t *font_bytes = (uint8_t *)read_file("assets/Consolas.ttf");
    font_data->font_bitmap = (uint8_t *)malloc(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * sizeof(uint8_t));

    const float pixel_height = 50;

    stbtt_BakeFontBitmap((uint8_t *)font_bytes, 0, pixel_height, font_data->font_bitmap, FONT_ATLAS_WIDTH,
                         FONT_ATLAS_HEIGHT, ' ', CHAR_COUNT, font_data->font_char_data);

    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, font_bytes, 0);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

    float scale = stbtt_ScaleForPixelHeight(&font_info, pixel_height);
    float ascent_f = (float)ascent * scale;
    float descent_f = (float)descent * scale;
    float line_gap_f = (float)line_gap * scale;

    printf("ascent %f, descent %f, line gap %f scale %f\n", ascent_f, descent_f, line_gap_f, scale);

    // ascent + descent == pixel_height
    // these below are in the same coord system with stbtt_bakedchar.x0/y0
    // they're both in bitmap pixels, local to the glyph
    // -40 means it's 40 pixels above the baseline
    // start from here: from ^this, we can deduct where the baseline should be, in UVs
    // 40 above, 10 below.
    // instead of using 0 as the textvb's bottom coord, incorporate that -(10/50)
    // don't know how exactly thought. it's late now

    int c_x1, c_y1, c_x2, c_y2;
    stbtt_GetCodepointBitmapBox(&font_info, '_', scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
    printf("(%d, %d) - (%d, %d)\n", c_x1, c_y1, c_x2, c_y2);
    stbtt_GetCodepointBitmapBox(&font_info, '|', scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
    printf("(%d, %d) - (%d, %d)\n", c_x1, c_y1, c_x2, c_y2);

    free(font_bytes);
}

static void text_deinit(FontData *font_data)
{
    free(font_data->font_bitmap);
}
