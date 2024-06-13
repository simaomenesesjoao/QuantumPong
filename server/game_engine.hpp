#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H 1

#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include "../event_queue.hpp"
#include "simulator.hpp"

class game_engine{
    public:
        std::string event_names[100];
        event_queue *eq;
        simulator *physics;
        std::mutex mutex;
        std::vector<std::thread> threads;

        // Game state
        bool *connection_status;
        bool ready[2], streaming[2];
        int sockets[2], ready_to_toggle[2];
        bool all_ready, paused, can_toggle, game_finished;

        int delay_event_loop;
        int delay_streamer;


    game_engine();
    void game_loop();
    void listener(int);
    void streamer(int);
    // void sender(int, uint8_t*, unsigned);
    void sender2(int, uint8_t*, unsigned);
    void set_event_names();
    void print_state();

    void toggle_ready(int);

    void on_connect(uint8_t *);
    void on_disconnect(uint8_t *);
    void on_change_screen(uint8_t *);
    void on_send_init_info(uint8_t *);
    void on_pressed_space(uint8_t *);
    void on_pressed_key(uint8_t *);
    void on_update_status(uint8_t *);
    void on_player_won(uint8_t *);
    void on_mousebuttondown(uint8_t *);
    void on_add_pot(uint8_t *);
    // void on_add_mag(uint8_t *);


};

#endif // GAME_ENGINE_H