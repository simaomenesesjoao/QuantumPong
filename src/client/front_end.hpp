#ifndef FRONT_END_H
#define FRONT_END_H 1

#include <cstdint>
#include "../event_queue.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include "states.hpp"

class frontEnd{
public:
    event_queue eventQueue;
    State *current_state;
    
    frontEnd();
    ~frontEnd();
    void setState(State*);
    void handle(uint8_t* data);
    void game_loop();

};


#endif // FRONT_END_H