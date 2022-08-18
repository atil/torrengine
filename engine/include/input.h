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

class Input {
    bool curr[(usize)KeyCode::MAX];
    bool prev[(usize)KeyCode::MAX];

  public:
    void update(struct GLFWwindow *window);
    bool just_pressed(KeyCode key_code) const;
    bool is_down(KeyCode key_code) const;
};
