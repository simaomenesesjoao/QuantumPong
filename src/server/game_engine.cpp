#include <unistd.h>
#include <iostream>
#include "../macros.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"

void game_engine::init(unsigned port) {
    conn.init(port);
    std::cout << "game_engine: ready on port " << port << "\n" << std::flush;
}

void game_engine::streamer() {
    while (true) {
        usleep(delay_streamer);
        if (!engine.streaming || engine.buffer_f == nullptr) continue;
        conn.send(engine.buffer_f, engine.buffer_f_size);
        // std::cout << "." << std::flush;
    }
}

void game_engine::run() {
    threads.emplace_back(&simulator::loop, &engine);
    threads.emplace_back(&connection_handler::accept_and_listen, &conn, &engine);
    threads.emplace_back(&game_engine::streamer, this);

    for (auto &t : threads) t.join();
}

void game_engine::finalize() {
    engine.finalize();
}
