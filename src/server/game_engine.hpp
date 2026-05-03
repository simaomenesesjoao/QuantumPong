#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H 1

#include <thread>
#include <vector>
#include "simulator.hpp"
#include "connection_handler.hpp"

class game_engine {
public:
    game_engine() = default;

    void init(unsigned port);
    void run();      // launches threads, blocks until done
    void finalize();

private:
    simulator          engine;
    connection_handler conn;
    std::vector<std::thread> threads;

    int delay_streamer   = 100000;

    void streamer();
};

#endif // GAME_ENGINE_H
