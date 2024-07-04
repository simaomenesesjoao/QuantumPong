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





void Server::processPressedKey(uint8_t *data){
    Event<EV_PRESSED_KEY> event(data);
    int player_number = event.player_number;
    int keycode = event.keycode;

    if(VERBOSE>0){std::cout << "p" << player_number << " on_pressed_key\n";}
    if(VERBOSE>0){std::cout << "key" << keycode << "\n";}

    int dd = 10;
    int dx = 0;
    int dy = 0;
    if(keycode == KEY_w) dy = -dd;
    if(keycode == KEY_s) dy =  dd;
    if(keycode == KEY_a) dx = -dd;
    if(keycode == KEY_d) dx =  dd;

    int x0 = engine->top_player_x;
    int y0 = engine->top_player_y;
    int x1 = engine->bot_player_x;
    int y1 = engine->bot_player_y;

    if(player_number == 0){
        x0 += dx;
        y0 += dy;
    }

    if(player_number == 1){
        x1 += dx;
        y1 += dy;
    }

    std::cout << "positions" << x0 << " " << y0 << " " << x1 << " " << y1 << "\n";
    engine->update_paddles(x0, y0, x1, y1, player_number);

    if(player_number == 0){
        std::cout << "-----sending to player 0\n";
        eventQueue->add_event(Event<EV_PADDLE_UPDATE>(player_number,engine->top_player_x, engine->top_player_y).buffer_b);
    }

    if(player_number == 1){
        std::cout << "-----sending to player 1\n";
        eventQueue->add_event(Event<EV_PADDLE_UPDATE>(player_number,engine->bot_player_x,engine->bot_player_y).buffer_b);
    }
    

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

    eventQueue->add_event(Event<EV_PADDLE_UPDATE>(0,engine->top_player_x, engine->top_player_y).buffer_b);
    eventQueue->add_event(Event<EV_PADDLE_UPDATE>(1,engine->bot_player_x, engine->bot_player_y).buffer_b);
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
            // engine->reset_state();
        } else {
            processPressedKey(ev.buffer_b);
        }   
    }

    // Player won
    if(evg.event_ID == EV_PLAYER_WON){
        activeHandler = &Server::GameOffHandler;
        GameOffOnEntry();
        // engine->reset_state();
    }


    // Pause Game
    if(evg.event_ID == EV_PRESSED_SPACE){
        GamePausedOnEntry();
        engine->paused=true;
    }

    // Some player added a potential
    if(evg.event_ID == EV_MOUSEBUTTONDOWN){
        Event<EV_MOUSEBUTTONDOWN> event(data);
    
        
        int player_number = event.player_number;
        int button = event.button_number;
        int x = event.x;
        int y = event.y;


        if(VERBOSE>0){
            std::cout << "p" << player_number << " on_mousebuttondown. button:" << button << " ";
            std::cout << "xy: " << x << " " << y <<  "\n";
        }

        if(button == 1){
            unsigned w = 100;
            float v = 0.9;
            engine->set_local_pot(x, y, w, w, v);

        }
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
