struct Widget {
    String string;
    TextTransform transform;
    u8 _padding[4];
    FontData *font_data;

    Widget() : string(), font_data(nullptr) {
    }

    explicit Widget(char *chars, TextTransform transform, FontData *font_data)
        : string(chars), transform(transform), font_data(font_data) {
    }
};

static void widget_set_string(Widget *widget, u32 integer) {
    char int_str_buffer[32]; // TODO @ROBUSTNESS: Assert that it's a 32-bit integer
    sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", integer);
    widget->string.~String();
    widget->string = String(int_str_buffer);
}
