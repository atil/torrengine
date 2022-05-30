enum class KeyCode
{
    W = 0x0,
    S,
    Up,
    Down,
    Esc,
    Enter,
    MAX
};

struct Input {
    bool curr[(usize)KeyCode::MAX];
    bool prev[(usize)KeyCode::MAX];

    void update(GLFWwindow *window) {
        // NOTE @BUGFIX: There used to write "sizeof(KeyCode)" here, which is the size of an integer (the enum's
        // underlying type. But our array is 6 bools
        memcpy(prev, curr, (usize)KeyCode::MAX * sizeof(bool));
        curr[(usize)KeyCode::W] = (bool)glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        curr[(usize)KeyCode::S] = (bool)glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        curr[(usize)KeyCode::Down] = (bool)glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        curr[(usize)KeyCode::Up] = (bool)glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        curr[(usize)KeyCode::Esc] = (bool)glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        curr[(usize)KeyCode::Enter] = (bool)glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    }

    bool just_pressed(KeyCode key_code) {
        return curr[(usize)key_code] && !prev[(usize)key_code];
    }

    bool is_down(KeyCode key_code) {
        return curr[(usize)key_code];
    }
};
