// NOTE @DOCS: Game origin: up-left

// start from here
// - check todos
// - implement lua hello world

#include "common.h"

DISABLE_WARNINGS
#include <memory>
ENABLE_WARNINGS

#include "application.h"
#include "pong.h"
#include <cassert>

int main(void) {

    UNREACHABLE("LOL");

    std::unique_ptr<IGame> pong = std::make_unique<PongGame>();
    Application app(std::move(pong));
    app.loop();
    return 0;
}
