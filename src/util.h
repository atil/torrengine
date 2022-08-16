#pragma once

#include "common.h"

struct Util {
    static u8 *read_file(const char *file_path) {
        FILE *f = fopen(file_path, "rb");
        if (!f) {
            printf("failed to open file %s", file_path);
            return NULL;
        }

        fseek(f, 0, SEEK_END);
        size_t length = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);

        u8 *buffer = (u8 *)malloc(length + 1);
        fread(buffer, sizeof(u8), length, f);
        fclose(f);
        buffer[length] = 0;

        return buffer;
    }
};