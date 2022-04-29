struct Widget {
    String string;
    TextTransform transform;
};

static Widget widget_init(char *chars, TextTransform transform) {
    Widget widget;
    widget.string = str_init(chars);
    return widget;
}

static void widget_deinit(Widget *widget) {
    str_deinit(&widget->string);
    free(widget);
}