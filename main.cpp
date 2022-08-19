// NOTE @DOCS: Game origin: up-left

#include "common.h"

DISABLE_WARNINGS
#include <memory>
ENABLE_WARNINGS

#include "application.h"
#include "pong.cpp"

int main() {
    std::unique_ptr<IGame> pong = std::make_unique<PongGame>();
    Application app(std::move(pong));
    app.loop();
    return 0;
}
