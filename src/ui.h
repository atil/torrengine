struct Widget {
    String string;
    TextTransform transform;
    FontData *font_data;
};

static Widget widget_init(char *chars, TextTransform transform, FontData *font_data) {
    Widget widget;
    widget.string = str_init(chars);
    widget.transform = transform;
    widget.font_data = font_data;
    return widget;
}

static void widget_set_string(Widget *widget, u32 integer) {
    char int_str_buffer[32]; // TODO @ROBUSTNESS: Assert that it's a 32-bit integer
    sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", integer);
    str_deinit(&widget->string);
    widget->string = str_init(int_str_buffer);
}

static void widget_deinit(Widget *widget) {
    str_deinit(&widget->string);
    free(widget);
}
