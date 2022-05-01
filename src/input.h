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
    bool curr[KeyCode::MAX];
    bool prev[KeyCode::MAX];

    void update(GLFWwindow *window) {
        memcpy(prev, curr, (usize)KeyCode::MAX * sizeof(KeyCode));
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

