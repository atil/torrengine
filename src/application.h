#include "common.h"

DISABLE_WARNINGS
#include <memory>
#include <GLFW/glfw3.h>
ENABLE_WARNINGS

struct IGame {
    virtual void init(struct Engine &) = 0;
    virtual ~IGame() = default;
};

struct Application {
    struct GLFWwindowDestroyer {
        void operator()(struct GLFWwindow *window);
    };

    std::unique_ptr<struct Engine> engine;
    std::unique_ptr<struct GLFWwindow, GLFWwindowDestroyer> window;
    std::unique_ptr<struct IGame> game;

    PREVENT_COPY_MOVE(Application);
    Application(std::unique_ptr<IGame> game);
    ~Application();

    void loop();
};
