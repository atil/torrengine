#define GLFW_DLL          // Dynamically linking glfw
#define GLFW_INCLUDE_NONE // Disable including dev environment header

#include "common.h"

DISABLE_WARNINGS
#include <cstring>
#include <GLFW/glfw3.h>
ENABLE_WARNINGS

#include "input.h"

void Input::update(GLFWwindow *window) {
    // NOTE @BUGFIX: There used to write "sizeof(KeyCode)" here, which is the size of an integer (the
    // enum's underlying type. But our array is 6 bools
    memcpy(prev, curr, (usize)KeyCode::MAX * sizeof(bool));
    curr[(usize)KeyCode::W] = (bool)glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    curr[(usize)KeyCode::S] = (bool)glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    curr[(usize)KeyCode::Down] = (bool)glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    curr[(usize)KeyCode::Up] = (bool)glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    curr[(usize)KeyCode::Esc] = (bool)glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    curr[(usize)KeyCode::Enter] = (bool)glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    curr[(usize)KeyCode::Debug1] = (bool)glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
    curr[(usize)KeyCode::Debug2] = (bool)glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
}

bool Input::just_pressed(KeyCode key_code) {
    return curr[(usize)key_code] && !prev[(usize)key_code];
}

bool Input::is_down(KeyCode key_code) {
    return curr[(usize)key_code];
}