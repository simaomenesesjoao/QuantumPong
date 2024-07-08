#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H 1

#include <thread>
#include <vector>
#include <mutex>
#include "state_machines.hpp"
#include "connection_handler.hpp"

class game_engine{
    public:

        int delay_event_loop;
        int delay_streamer;
        int delay_simulation;

        std::mutex mutex;
        std::vector<std::thread> threads;

        event_queue eventQueue;
        connection_handler conn;
        simulator engine;
        Player player1;
        Player player2;
        Server server;

        game_engine();
        void init();
        void game_loop();
        void finalize();
};

#endif // GAME_ENGINE_H