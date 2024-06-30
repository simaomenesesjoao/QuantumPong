#include <iostream>
#include <string>
#include <cstdint>
#include "../event_queue.hpp"
#include <unistd.h>
#include "state_machines.hpp"


Server::Server(event_queue* eq){
    eventQueue = eq;
    activeHandler = &Server::GameOffHandler;
    state = GameOff;
    stateStrings[GameOff]     = "GameOff";
    stateStrings[GameRunning] = "GameRunning";
    stateStrings[GamePaused]  = "GamePaused";
}

void Server::addPlayers(Player *p1, Player *p2){
    player1 = p1;
    player2 = p2;
}

void Server::handle(uint8_t* data){
    (this->*activeHandler)(data);
}

void Server::addSimulator(simulator *sim){
    engine = sim;
}






void Server::GameOffOnEntry(){
    std::cout << "GameOffOnEntry\n";
    state = GameOff;
}
void Server::GameOffHandler(uint8_t* data){
    std::cout << "GameOffHandler\n";
    Event<EV_GENERIC> evg(data);

    // Start the game
    if(evg.event_ID == EV_START_GAME){
        activeHandler = &Server::GameRunningHandler;
        GameRunningOnEntry();

        engine->paused = false;
    }
}



void Server::GameRunningOnEntry(){
    std::cout << "GameRunningOnEntry\n";
    state = GameRunning;
    activeHandler = &Server::GameRunningHandler;
}
void Server::GameRunningHandler(uint8_t* data){
    std::cout << "GameOnHandler\n";
    Event<EV_GENERIC> evg(data);
    
    // Player quit
    if(evg.event_ID == EV_PRESSED_KEY){
        Event<EV_PRESSED_KEY> ev(data);
        if(ev.keycode == KEY_esc){
            activeHandler = &Server::GameOffHandler;
            GameOffOnEntry();
            engine->reset_state();
            

        }
    }

    // Player won
    if(evg.event_ID == EV_PLAYER_WON){
        activeHandler = &Server::GameOffHandler;
        GameOffOnEntry();
        engine->reset_state();
    }


    // Pause Game
    if(evg.event_ID == EV_PRESSED_SPACE){
        GamePausedOnEntry();
        engine->paused=true;
    }

}



void Server::GamePausedOnEntry(){

    activeHandler = &Server::GamePausedHandler;
    std::cout << "GamePausedOnEntry\n";
    state = GamePaused;
    
}
void Server::GamePausedHandler(uint8_t* data){
    std::cout << "GamePausedHandler\n";
    Event<EV_GENERIC> evg(data);
    std::cout << "event id: " << evg.event_ID << "\n";
    // Unpause
    if(evg.event_ID == EV_UNPAUSE_GAME){
        GameRunningOnEntry();
        engine->paused=false;
    }


    // Player quit
    if(evg.event_ID == EV_PRESSED_KEY){
        Event<EV_PRESSED_KEY> ev(data);
        if(ev.keycode == KEY_esc){
            activeHandler = &Server::GameOffHandler;
            GameOffOnEntry();
        }
    }
}
