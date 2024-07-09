#include "game_engine.hpp"

int main(){

    game_engine engine;
    engine.init();
    engine.game_loop();
    engine.finalize();

    return 0;
}