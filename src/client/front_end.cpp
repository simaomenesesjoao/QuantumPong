#include "front_end.hpp"
#include <iostream>
#include "../macros.hpp"
#include "../event_queue.hpp"
#include <unistd.h>


frontEnd::frontEnd():
    eventQueue(300, HEADER_LEN, 0),
    current_state(new StateDisconnected(this)){

    // running = true;
};

frontEnd::~frontEnd(){
    delete current_state;    
}

void frontEnd::setState(State* new_state){
    delete current_state;
    current_state = new_state;
}


void frontEnd::game_loop(){
    int delay = 20;
    int event;
    uint8_t data[HEADER_LEN];

    while(true){
        
        usleep(delay*1000); 
        eventQueue.pop_oldest(&event, data);
        if(event<0){
            continue;
        }
        if(event == EV_EXIT) break;

        std::cout << "event found";
        current_state->handle(data);
    }

}
