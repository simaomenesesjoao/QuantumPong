#ifndef FRONT_END_H
#define FRONT_END_H 1

#include <cstdint>
#include "client.hpp"
#include "graphics.hpp"
#include <vector>
#include <thread>

class frontEnd{
public:

    std::vector<std::thread> threads;

    graphics gr;
    client cl;
    
    void init();
    void finalize();
    void game_loop();

    void on_pause_game(uint8_t *data);
    void on_unpause_game(uint8_t *data);
    void on_end_screen(uint8_t *data);
    void on_update_status(uint8_t *data);
    void on_send_init_info(uint8_t *data);
    void on_stream(uint8_t *data);
    void on_paddle_update(uint8_t *data);
    void on_send_pot(uint8_t *data);
    void on_pressed_space(uint8_t *data);
    void on_pressed_key(uint8_t *data);
    void on_mousebuttondown(uint8_t *data);

};

#endif // FRONT_END_H