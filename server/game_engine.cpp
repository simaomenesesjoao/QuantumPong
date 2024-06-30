#include <iostream>
#include <vector>
#include <unistd.h>
#include <thread>
#include <semaphore.h>
#include <netinet/in.h>
#include <CL/opencl.hpp>
#include "../macros.hpp"
#include "../event_queue.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"
#include "state_machines.hpp"


game_engine::game_engine(){}

void game_engine::game_loop(){
    delay_event_loop = 25*1000; 
    delay_streamer = 80*1000;
    int delay_simulation = 25*1000;

    int event = -1;
    uint8_t *data;

    event_queue eventQueue(30);

    connection_handler conn(&eventQueue);
    conn.init(8080);
    // connection_status = conn.connection_status;

    unsigned Lx = 300;
    unsigned Ly = 700;
    simulator engine(&eventQueue);
    engine.init(Lx, Ly, delay_simulation);
    // engine.paused = &paused;
    // physics = &engine;
    
    threads.push_back(std::thread(&connection_handler::process_connections, &conn));
    threads.push_back(std::thread(&simulator::loop, &engine));

    // Create player and server instances, and pass the pointers
    Player player1(&eventQueue, 0), player2(&eventQueue, 1);
    Server server(&eventQueue);
    
    player1.addOtherPlayer(&player2);
    player2.addOtherPlayer(&player1);
    player1.addServer(&server);
    player2.addServer(&server);
    server.addPlayers(&player1, &player2);
    server.addSimulator(&engine);


    

    while(true){
        usleep(delay_event_loop);
        eventQueue.read(&event, &data);
        if(event<0) continue;
        std::cout << "event: " << event << "\n";
        std::cout << "states before: " << player1.stateStrings[player1.state] << " " << player2.stateStrings[player2.state] << " " << server.stateStrings[server.state] << "\n";
        player1.handle(data);
        player2.handle(data);
        server.handle(data);
        std::cout << " ----- states after: " << player1.stateStrings[player1.state] << " " << player2.stateStrings[player2.state] << " " << server.stateStrings[server.state] << "\n";
    }
    
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    // t3.join();

    engine.finalize();

}
