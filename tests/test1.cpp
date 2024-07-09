#include "../src/client/front_end.hpp"
#include "../src/server/game_engine.hpp"
#include <thread>
#include <vector>
#include <unistd.h>

int main(){
    std::vector<std::thread> threads;

    game_engine engine;
    engine.init();
    threads.push_back(std::thread(&game_engine::game_loop, &engine));

    usleep(1000000);
    frontEnd client1;
    client1.init();
    threads.push_back(std::thread(&frontEnd::game_loop, &client1));

    // usleep(1000000);
    // frontEnd client2;
    // client2.init();   
    // threads.push_back(std::thread(&frontEnd::game_loop, &client2));


    engine.finalize();
    client1.finalize();
    // client2.finalize();
    return 0;
}