#pragma once

enum class KeyCode {
    W = 0x0,
    S,
    Up,
    Down,
    Esc,
    Enter,
    Debug1,
    Debug2,
    MAX
};

struct Input {
    bool curr[(usize)KeyCode::MAX];
    bool prev[(usize)KeyCode::MAX];

    void update(struct GLFWwindow *window);
    bool just_pressed(KeyCode key_code);
    bool is_down(KeyCode key_code);
};
