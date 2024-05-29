#include <iostream>
#include <semaphore.h>
#include <mutex>
#include <unistd.h>
#include <vector>
#include <thread>
#include <netinet/in.h>
#include <CL/opencl.hpp>
#include "event_queue.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"

int main(){

    game_engine engine;
    engine.game_loop();

    return 0;
}
