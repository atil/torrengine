// NOTE @DOCS: Game origin: up-left

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#pragma warning(disable : 4505) // Unreferenced functions

#include "application.h"
#include "pong.h"
#include <memory>

#pragma warning(pop)

int main(void) {
    std::unique_ptr<IGame> pong = std::make_unique<PongGame>();
    Application app(std::move(pong));
    app.loop();
    return 0;
}
