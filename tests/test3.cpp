#define CATCH_CONFIG_MAIN 
#include "catch.hpp"
#include "../src/event_queue.hpp"
#include <cstdint>
#define HEADER_LEN 100

int function(unsigned num) {
    event_queue eq(30);
    Event<EV_CONNECT> ev1(1,4);
    Event<EV_DISCONNECT> ev2(0);
    Event<EV_CONNECT> ev3(1,3);
    Event<EV_GENERIC> ev4(-1);

    eq.add_event(ev1.buffer_b);
    eq.add_event(ev2.buffer_b);
    eq.add_event(ev3.buffer_b);
    eq.add_event(ev4.buffer_b);

    uint8_t data[HEADER_LEN];
    int event_ID;

    for(unsigned i=0; i<num; i++)
        eq.pop_oldest(&event_ID, data);
    
    
    
    return event_ID;
}

// Test case
TEST_CASE("Event queue is tested", "[event]") {
    REQUIRE(function(1) == EV_CONNECT);
    REQUIRE(function(2) == EV_DISCONNECT);
    REQUIRE(function(4) == EV_GENERIC);
}