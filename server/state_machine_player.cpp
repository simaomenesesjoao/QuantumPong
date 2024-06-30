
#include <iostream>
#include <string>
#include <cstdint>
#include <netinet/in.h>
#include "../event_queue.hpp"
#include <unistd.h>
#include "state_machines.hpp"


void Player::streamer(){
    unsigned delay_streamer = 100*1000;
    uint8_t *buffer_f = server->engine->buffer_f;
    unsigned buffer_f_size = server->engine->buffer_f_size;
    if(VERBOSE>0){ std::cout << "Entered streamer for player " << playerNumber << "\n";}
   

    auto start = std::chrono::system_clock::now();

    while(state == PlayerInGame){

        int result = send(socket, buffer_f, buffer_f_size*sizeof(uint8_t), MSG_NOSIGNAL);

        if(result < 0){
            eventQueue->add_event(Event<EV_DISCONNECT>(playerNumber).buffer_b);
            break;
        }

        
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        start = std::chrono::system_clock::now();
        usleep(delay_streamer);
    }
}

void Player::listener(){
    
    uint8_t* read_buf;
    read_buf = new uint8_t[HEADER_LEN];

    if(VERBOSE>0){ std::cout << "Entered listener for player " << playerNumber << " in socket " << socket << "\n";}


    while (state != PlayerDisconnected) {
        int result = recv(socket, read_buf, HEADER_LEN*sizeof(uint8_t), MSG_WAITALL);
        std::cout << "socket: " << socket << "read res: " << result << "\n" << std::flush;

        if(result>0){
            for(unsigned i=0; i<10; i++){
                std::cout << (int)read_buf[i] << " ";
            }
            eventQueue->add_event(read_buf);
        }
        
        if(result <= 0){
            eventQueue->add_event(Event<EV_DISCONNECT>(playerNumber).buffer_b);
            break;
        }
    }

    if(VERBOSE>0){ std::cout << "Left listener for player " << playerNumber << " in socket " << socket << "\n";}
    std::cout << "state: " << state << "\n";
    delete [] read_buf;
}

void Player::sender(uint8_t *data, unsigned length){

    std::cout << "Sending to socket " << socket << " " << (int)data[0] << " " << (int)data[1] << "with length" << length <<  "\n";
    int result = send(socket, data, length*sizeof(uint8_t), MSG_NOSIGNAL);

    int n;
    if(result < 0){
        eventQueue->add_event(Event<EV_DISCONNECT>(playerNumber).buffer_b);
    }
}




Player::Player(event_queue* eq, int pNum){
    eventQueue = eq;
    playerNumber = pNum;
    socket = -1;
    activeHandler = &Player::PlayerDisconnectedHandler;
    state = PlayerDisconnected;
    stateStrings[PlayerDisconnected] = "PlayerDisconnected";
    stateStrings[PlayerIdle]         = "PlayerIdle";
    stateStrings[PlayerWantNew]      = "PlayerWantNew";
    stateStrings[PlayerInGame]       = "PlayerInGame";
    stateStrings[PlayerFreeze]       = "PlayerFreeze";
    stateStrings[PlayerWantUnpause]  = "PlayerWantUnpause";
    stateStrings[PlayerInEnd]        = "PlayerInEnd";
}

Player::~Player(){
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void Player::addOtherPlayer(Player *player){otherPlayer = player;}    
void Player::addServer(Server *serv){server = serv;}    
void Player::handle(uint8_t* data){(this->*activeHandler)(data);}





    

void Player::PlayerDisconnectedHandler(uint8_t* data){
    std::cout << "PlayerDisconnectedHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    if(evg.event_ID == EV_CONNECT && evg.player_number == playerNumber){
        Event<EV_CONNECT> ev(data);
        socket = ev.socket;

        activeHandler = &Player::PlayerIdleHandler;            
        PlayerIdleOnEntry();
        
        threads.push_back(std::thread(&Player::listener, this));

        sender(Event<EV_SEND_INIT_INFO>(playerNumber).buffer_b, HEADER_LEN);
        // sender(Event<EV_CHANGE_SCREEN>(playerNumber,0).buffer_b, HEADER_LEN);
                
    }
}



void Player::PlayerIdleOnEntry(){
    std::cout << "PlayerIdleOnEntry " << playerNumber << "\n";
    state = PlayerIdle;

    activeHandler = &Player::PlayerIdleHandler; 
    eventQueue->add_event(Event<EV_UPDATE_STATUS>(playerNumber, state, otherPlayer->state).buffer_b);
    
}

void Player::PlayerIdleHandler(uint8_t* data){
    std::cout << "PlayerIdleHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    if(evg.event_ID == EV_PRESSED_KEY && evg.player_number == playerNumber){
        Event<EV_PRESSED_KEY> ev(data);
        if(ev.keycode == KEY_n){

            PlayerWantNewOnEntry();
        }
    }

    if(evg.event_ID == EV_UPDATE_STATUS){
        sender(data, HEADER_LEN);
    }
}



void Player::PlayerWantNewOnEntry(){
    state = PlayerWantNew;
    activeHandler = &Player::PlayerWantNewHandler;


    // std::cout << "PlayerWantNewOnEntry " << playerNumber << "\n";
    // std::cout << "mystate: " << state << " other state: " << otherPlayer->state << "\n";
    
    // Check if both players are in the WantNew state. If not, update the lobby status
    if(otherPlayer->state == PlayerWantNew){
        std::cout << "Both players want to start\n ";
        eventQueue->add_event(Event<EV_START_GAME>().buffer_b);
    } else {
        eventQueue->add_event(Event<EV_UPDATE_STATUS>(playerNumber, state, otherPlayer->state).buffer_b);
    }

}

void Player::PlayerWantNewHandler(uint8_t* data){
    std::cout << "PlayerWantNewHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    // Toggle StartNewGame off
    if(evg.event_ID == EV_PRESSED_KEY && evg.player_number == playerNumber){
        Event<EV_PRESSED_KEY> ev(data);
        if(ev.keycode == KEY_n){
            PlayerIdleOnEntry();
        }
    }

    // Actually start the game
    if(evg.event_ID == EV_START_GAME){
        PlayerInGameOnEntry();
    }

    if(evg.event_ID == EV_UPDATE_STATUS){
        sender(data, HEADER_LEN);
    }
}

void Player::PlayerInGameOnEntry(){

    std::cout << "PlayerInGameOnEntry " << playerNumber << "\n";
    state = PlayerInGame;
    activeHandler = &Player::PlayerInGameHandler;

    threads.push_back(std::thread(&Player::streamer, this));
}

void Player::PlayerInGameHandler(uint8_t* data){
    std::cout << "PlayerInGameHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    // Quit GameKEY
    if(evg.event_ID == EV_PRESSED_KEY){
        Event<EV_PRESSED_KEY> ev(data);
        if(ev.keycode == KEY_esc){
            PlayerInEndOnEntry(1-ev.player_number);
        }
    }

    // Someone won
    if(evg.event_ID == EV_PLAYER_WON){
        Event<EV_PLAYER_WON> ev(data);
        int winner = ev.player_number;

        PlayerInEndOnEntry(winner);
    }

    // Pause Game
    if(evg.event_ID == EV_PRESSED_SPACE){
        PlayerFreezeOnEntry();
        sender(Event<EV_PAUSE_GAME>(-1).buffer_b, HEADER_LEN);
    }

}

void Player::PlayerFreezeOnEntry(){
    std::cout << "PlayerFreezeOnEntry" << playerNumber << "\n";
    state = PlayerFreeze;
    activeHandler = &Player::PlayerFreezeHandler;
}

void Player::PlayerFreezeHandler(uint8_t* data){
    std::cout << "PlayerFreezeHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    // Toggle StartNewGame off
    if(evg.event_ID == EV_PRESSED_SPACE && evg.player_number == playerNumber){
        PlayerWantUnpauseOnEntry();
    }
}

void Player::PlayerWantUnpauseOnEntry(){
    std::cout << "PlayerWantUnpauseOnEntry " << playerNumber << "\n";
    state = PlayerWantUnpause;
    activeHandler = &Player::PlayerWantUnpauseHandler; 
    if(otherPlayer->state == PlayerWantUnpause){
        Event<EV_UNPAUSE_GAME> unpauseEvent(-1);
        eventQueue->add_event(unpauseEvent.buffer_b);
    }

}

void Player::PlayerWantUnpauseHandler(uint8_t* data){
    std::cout << "PlayerWantUnpauseHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    if(evg.event_ID == EV_UNPAUSE_GAME){
        PlayerInGameOnEntry();
        sender(evg.buffer_b, HEADER_LEN);
    }

    if(evg.event_ID == EV_PRESSED_SPACE && evg.player_number == playerNumber){
        PlayerFreezeOnEntry();
    }   
}


void Player::PlayerInEndOnEntry(int winner){
    std::cout << "PlayerInEndOnEntry " << playerNumber << "\n";
    state = PlayerInEnd;   
    activeHandler = &Player::PlayerInEndHandler;
    sender(Event<EV_END_SCREEN>(winner).buffer_b, HEADER_LEN); 
}

void Player::PlayerInEndHandler(uint8_t* data){
    std::cout << "PlayerInEndHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    // Enter
    if(evg.event_ID == EV_PRESSED_KEY && evg.player_number == playerNumber){
        Event<EV_PRESSED_KEY> ev(data);
        if(ev.keycode == KEY_return){
            PlayerIdleOnEntry();
        }
    }
}