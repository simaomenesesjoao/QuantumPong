#include <iostream>
#include "../event_queue.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"
#include "state_machines.hpp"


game_engine::game_engine():
    player1(0), player2(1), eventQueue(300){}

void game_engine::game_loop(){
    
    
    int event = -1;
    uint8_t *data;

    while(true){
        usleep(delay_event_loop);
        eventQueue.read(&event, &data);
        if(event<0) continue;
        
        player1.handle(data);
        player2.handle(data);
        server.handle(data);
        std::cout << "event:" << event << " ----- states after: " << player1.stateString << " " << player2.stateString << " " << server.stateString << "\n";
    }
    
}


void game_engine::init(){
    // Initialize the game with some default values
    int ms = 1000;

    delay_event_loop = 5*ms; 
    delay_streamer = 50*ms;
    delay_simulation = 25*ms;

    unsigned Lx = 300;
    unsigned Ly = 700;

    int port = 8080;

    // Initialize the connection handler
    conn.addEventQueue(&eventQueue);
    conn.init(port);

    // Initialize the physics engine
    engine.addEventQueue(&eventQueue);
    engine.init(Lx, Ly, delay_simulation);

    // Initialize player and server instances
    server.addEventQueue(&eventQueue);
    player1.addEventQueue(&eventQueue);
    player2.addEventQueue(&eventQueue);

    player1.addConnectionHandler(&conn);
    player2.addConnectionHandler(&conn);

    player1.setStreamerDelayMS(delay_streamer);
    player2.setStreamerDelayMS(delay_streamer);

    // Connect the server and players to each other through their references    
    player1.addOtherPlayer(&player2);
    player2.addOtherPlayer(&player1);
    player1.addServer(&server);
    player2.addServer(&server);
    server.addPlayers(&player1, &player2);
    server.addSimulator(&engine);


    // Start the threads
    threads.push_back(std::thread(&connection_handler::process_connections, &conn));
    threads.push_back(std::thread(&simulator::loop, &engine));
}

void game_engine::finalize(){

    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    } 
    engine.finalize();

}