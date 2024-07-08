#include "front_end.hpp"

// g++ *.cpp -o client  `sdl2-config --libs --cflags` -Wall -lm -lSDL2_ttf && ./client 
int main(){
    frontEnd client;
    client.init();
    client.game_loop();
    client.finalize();
    
    return 0;
}
