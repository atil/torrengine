// TODO @CLEANUP: Maybe move these functions to render.h? To keep file count small? Maybe not, if they end up being
// totally unrelated?

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256

// TODO @CLEANUP: Can we get around this?
// It would be better to have only functions (and data types) decleared in the header files, so that we can treat them
// as libraries
static uint8_t *font_bitmap;
stbtt_bakedchar font_char_data[CHAR_COUNT];

static void text_init(void)
{
    // TODO @ROBUSTNESS: Assert that it's called once
    char *font_data = read_file("assets/Consolas.ttf");
    font_bitmap = (uint8_t *)malloc(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * sizeof(uint8_t));

    stbtt_BakeFontBitmap((uint8_t *)font_data, 0, 50, font_bitmap, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, ' ', CHAR_COUNT,
                         font_char_data);

    free(font_data);
}

// Allocate this vertex buffer outside, I think?
static void fill_vb_for_text(const char *text, Vec2 anchor, float scale, float **out_vertex_buffer,
                             size_t *out_vertex_buffer_size, uint32_t **out_index_buffer, size_t *out_index_buffer_size)
{
    const size_t char_count = strlen(text);

    size_t vertex_buffer_size = char_count * 16 * sizeof(float); // TODO @DOCS: Explain the data layout
    size_t index_buffer_size = char_count * 6 * sizeof(uint32_t);

    // TODO @LEAK: Where to manage these?
    float *vertex_buffer = (float *)malloc(vertex_buffer_size);
    uint32_t *index_buffer = (uint32_t *)malloc(index_buffer_size);

    uint32_t vert_curr = 0;
    uint32_t ind_curr = 0;
    for (size_t i = 0; i < char_count; i++)
    {
        char ch = text[i];
        float pixel_pos_x, pixel_pos_y;
        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(font_char_data, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, ch - ' ', &pixel_pos_x, &pixel_pos_y,
                           &quad, 1);

        // Anchor: bottom-left corner's normalized position
        // Our quads have their origin at bottom left. But textures have their at top left. Therefore we invert the V
        // coordinate of the UVs

        // Bottom left vertex
        vertex_buffer[vert_curr + 0] = (float)i * scale + anchor.x; // X:0
        vertex_buffer[vert_curr + 1] = (float)i * scale + anchor.y; // Y:0
        vertex_buffer[vert_curr + 2] = quad.s0;                     // U
        vertex_buffer[vert_curr + 3] = 1.0f - quad.t1;              // V

        // Bottom right vertex
        vertex_buffer[vert_curr + 4] = (float)(i + 1) * scale + anchor.x; // 1
        vertex_buffer[vert_curr + 5] = (float)i * scale + anchor.y;       // 0
        vertex_buffer[vert_curr + 6] = quad.s1;                           // U
        vertex_buffer[vert_curr + 7] = 1.0f - quad.t1;                    // V

        // Top right vertex
        vertex_buffer[vert_curr + 8] = (float)(i + 1) * scale + anchor.x; // 1
        vertex_buffer[vert_curr + 9] = (float)(i + 1) * scale + anchor.y; // 1
        vertex_buffer[vert_curr + 10] = quad.s1;                          // U
        vertex_buffer[vert_curr + 11] = 1.0f - quad.t0;                   // V

        // Top left vertex
        vertex_buffer[vert_curr + 12] = (float)i * scale + anchor.x;       // 0
        vertex_buffer[vert_curr + 13] = (float)(i + 1) * scale + anchor.y; // 1
        vertex_buffer[vert_curr + 14] = quad.s0;                           // U
        vertex_buffer[vert_curr + 15] = 1.0f - quad.t0;                    // V

        // Two triangles
        index_buffer[ind_curr + 0] = vert_curr + 0;
        index_buffer[ind_curr + 1] = vert_curr + 1;
        index_buffer[ind_curr + 2] = vert_curr + 2;
        index_buffer[ind_curr + 3] = vert_curr + 0;
        index_buffer[ind_curr + 4] = vert_curr + 2;
        index_buffer[ind_curr + 5] = vert_curr + 3;

        vert_curr += 16;
        ind_curr += 6;
    }

    *out_vertex_buffer = vertex_buffer;
    *out_vertex_buffer_size = vertex_buffer_size;
    *out_index_buffer = index_buffer;
    *out_index_buffer_size = index_buffer_size;
}

static void text_deinit(void)
{
    free(font_bitmap);
}
