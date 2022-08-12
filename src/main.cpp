// NOTE @DOCS: Game origin: up-left

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#pragma warning(disable : 4505) // Unreferenced functions

#include "game.h"
#include "pong.h"

#pragma warning(pop)

int main(void) {
    PongGame pong;
    main_game(pong);
    return 0;
}
