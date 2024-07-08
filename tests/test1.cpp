#include "../src/client/front_end.hpp"
#include "../src/server/game_engine.hpp"
#include <thread>
#include <vector>

int main(){
    std::vector<std::thread> threads;

    game_engine engine;
    // frontEnd client1;
    // frontEnd client2;
    engine.init();
    // client1.init();
    // client2.init();

    threads.push_back(std::thread(&game_engine::game_loop, &engine));
    // threads.push_back(std::thread(&frontEnd::game_loop, &client1));
    // threads.push_back(std::thread(&frontEnd::game_loop, &client2));


    engine.finalize();
    // client1.finalize();
    // client2.finalize();
    return 0;
}