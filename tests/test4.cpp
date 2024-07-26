
#include "../src/event_queue.hpp"
#include <cstdint>
#include <iostream>
#define HEADER_LEN 100

int function() {
    event_queue eq(30, EVENT_QUEUE_VERBOSE);
    Event<EV_GENERIC> ev1(-1);
    std::cout << "ev: " << ev1.event_ID << "\n";

    eq.add_event(ev1.buffer_b);

    uint8_t data[HEADER_LEN];

    int event_ID;
    eq.pop_oldest(&event_ID, data);
    std::cout << "id: " << event_ID << "\n";
    // return event_ID;
    return 0;
}

int main(){
    function();
    return 0;
}