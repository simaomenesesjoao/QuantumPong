
#include <iostream>
#include <string>
#include <cstdint>
#include <netinet/in.h>
#include "../event_queue.hpp"
#include <unistd.h>
#include "connection_handler.hpp"
#include "state_machines.hpp"


void Player::streamer(){
    // Streams the wavefunction to the client. It uses the server's internal wavefunction buffer 'buffer_f' which
    // is made of 100 bytes (HEADER_LEN) of header information and a payload with the wavefunction 
    if(PSTREAMER_DEBUG>0){ std::cout << "Entered streamer for player " << playerNumber << "\n";}

    
    uint8_t *buffer_f = server->engine->buffer_f;
    unsigned buffer_f_size = server->engine->buffer_f_size;

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
    // Receives events from the remote client
    
    uint8_t* read_buf = new uint8_t[HEADER_LEN];

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

    if(result < 0){
        eventQueue->add_event(Event<EV_DISCONNECT>(playerNumber).buffer_b);
    }
}


void Player::setStreamerDelayMS(int delay){
    delay_streamer = delay; 
}

void Player::addEventQueue(event_queue *eq){
    eventQueue = eq;
}

void Player::addConnectionHandler(connection_handler *conn){
    connection = conn;
}


Player::Player(int pNum){
    playerNumber = pNum;
    delay_streamer = 100*1000; 

    socket = -1;
    activeHandler = &Player::PlayerDisconnectedHandler;
    state = PlayerDisconnected;
    stateString = "PlayerDisconnected";

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






// void Player::ProcessDisconnection(uint8_t* data){
//     Event<EV_GENERIC> evg(data);
// }

void Player::PlayerDisconnectedOnEntry(){
    state = PlayerDisconnected;
    stateString = "PlayerDisconnected";
    activeHandler = &Player::PlayerDisconnectedHandler;
    connection->connection_status[playerNumber] = false;
    socket = -1;
}

void Player::PlayerDisconnectedHandler(uint8_t* data){
    std::cout << "PlayerDisconnectedHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    if(evg.event_ID == EV_CONNECT && evg.player_number == playerNumber){
        Event<EV_CONNECT> ev(data);
        socket = ev.socket;

        activeHandler = &Player::PlayerIdleHandler;            
        sender(Event<EV_SEND_INIT_INFO>(playerNumber).buffer_b, HEADER_LEN);
        PlayerIdleOnEntry();
        
        threads.push_back(std::thread(&Player::listener, this));

    }

}



void Player::PlayerIdleOnEntry(){
    std::cout << "PlayerIdleOnEntry " << playerNumber << "\n";
    state = PlayerIdle;
    stateString = "PlayerIdle";

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

    // Check if a player disconnected
    if(evg.event_ID == EV_DISCONNECT){

        if(evg.player_number == playerNumber){
            PlayerDisconnectedOnEntry();
        } else {

        }
        eventQueue->add_event(Event<EV_UPDATE_STATUS>(playerNumber, state, otherPlayer->state).buffer_b);
        
    }
}



void Player::PlayerWantNewOnEntry(){
    state = PlayerWantNew;
    stateString = "PlayerWantNew";
    activeHandler = &Player::PlayerWantNewHandler;


    // Check if both players are in the WantNew state. If not, update the lobby status
    if(otherPlayer->state == PlayerWantNew){
        std::cout << "Both players want to start\n ";
        eventQueue->add_event(Event<EV_START_GAME>(-1).buffer_b);
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

    // Check if a player disconnected. Change state first, then update state
    if(evg.event_ID == EV_DISCONNECT){
        
        if(evg.player_number == playerNumber){
            PlayerDisconnectedOnEntry();
            
        } else {
            
            PlayerIdleOnEntry();
        }
        eventQueue->add_event(Event<EV_UPDATE_STATUS>(playerNumber, state, otherPlayer->state).buffer_b);
        
    }

}


void Player::onSend_Pot(uint8_t* data){
    if(VERBOSE>0){ std::cout << " onSend_Pot\n";}

    // CHANGE: a lot of this processing can be done on the engine
    Event<EV_SEND_POT> event(data);
    
    int x = event.x;
    int y = event.y;
    int dx = event.dx;
    int dy = event.dy;
    int Lx = server->engine->Lx;
    int Ly = server->engine->Ly;

    // std::cout << event.x << " " << event.y << " " << event.dx << " " << event.dy << "\n";
    // std::cout << "Lxy:" << Lx << " " << Ly << "\n";
    
    if(x+dx > Lx) dx = Lx - x; 
    if(y+dy > Ly) dy = Ly - y;

    if(x<0){
        dx += x;
        x = 0;
    }
    if(y<0){
        dy += y;
        y = 0;
    }
    int buf_size = dx*dy;

    event.payload_size = buf_size;
    event.x = x;
    event.y = y;
    event.dx = dx;
    event.dy = dy;

    // std::cout << "event contains \n";
    // for(int i=0; i<HEADER_LEN;i++)
    //     std::cout << (int)event.buffer_b[i] << " ";
    // std::cout << "\n" << std::flush;

    // std::cout << "pot event contains:\n";
    // std::cout << event.x0 << " " << event.y0 << " " << event.x1 << " " << event.y1 << "\n";



    uint8_t buffer[buf_size+HEADER_LEN];
    for(unsigned i=0; i<HEADER_LEN; i++)
        buffer[i] = event.buffer_b[i];

    server->engine->get_pot(x, y, dx, dy, buffer+HEADER_LEN);

    sender(buffer, buf_size+HEADER_LEN);

}



void Player::onSend_Mag(uint8_t* data){
    if(VERBOSE>0){ std::cout << " onSend_Mag\n";}

    // CHANGE: a lot of this processing can be done on the engine
    Event<EV_SEND_MAG> event(data);

    int x = event.x;
    int y = event.y;
    int dx = event.dx;
    int dy = event.dy;
    int Lx = server->engine->Lx;
    int Ly = server->engine->Ly;

    std::cout << event.x << " " << event.y << " " << event.dx << " " << event.dy << "\n";
    std::cout << "Lxy:" << Lx << " " << Ly << "\n";
    
    if(x+dx > Lx) dx = Lx - x; 
    if(y+dy > Ly) dy = Ly - y;

    if(x<0){
        dx += x;
        x = 0;
    }
    if(y<0){
        dy += y;
        y = 0;
    }
    int buf_size = dx*dy;

    event.payload_size = buf_size;
    event.x = x;
    event.y = y;
    event.dx = dx;
    event.dy = dy;

    // std::cout << "event contains \n";
    // for(int i=0; i<HEADER_LEN;i++)
    //     std::cout << (int)event.buffer_b[i] << " ";
    // std::cout << "\n" << std::flush;


    uint8_t buffer[buf_size+HEADER_LEN];
    for(unsigned i=0; i<HEADER_LEN; i++)
        buffer[i] = event.buffer_b[i];

    server->engine->get_mag(x, y, dx, dy, buffer+HEADER_LEN);

    sender(buffer, buf_size+HEADER_LEN);

}


void Player::PlayerInGameOnEntry(){

    std::cout << "PlayerInGameOnEntry " << playerNumber << "\n";
    state = PlayerInGame;
    stateString = "PlayerInGame";
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

    // Update paddles
    if(evg.event_ID == EV_PADDLE_UPDATE){
        sender(data, HEADER_LEN);
    }

    // Update potential
    if(evg.event_ID == EV_SEND_POT){
        onSend_Pot(data);
    }

    // Update magnetic
    if(evg.event_ID == EV_SEND_MAG){
        onSend_Mag(data);
    }


    // Check if a player disconnected. 
    if(evg.event_ID == EV_DISCONNECT){
        if(evg.player_number == playerNumber){
            PlayerDisconnectedOnEntry();
        } else {
            PlayerInEndOnEntry(playerNumber);
        }
        
        
    }

}

void Player::PlayerFreezeOnEntry(){
    if(P_ON_ENTRY) std::cout << "PlayerFreezeOnEntry" << playerNumber << "\n";

    state = PlayerFreeze;
    stateString = "PlayerFreeze";
    activeHandler = &Player::PlayerFreezeHandler;
}

void Player::PlayerFreezeHandler(uint8_t* data){
    if(P_ON_HANDLER) std::cout << "PlayerFreezeHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    // Toggle StartNewGame off
    if(evg.event_ID == EV_PRESSED_SPACE && evg.player_number == playerNumber){
        PlayerWantUnpauseOnEntry();
    }


    // Check if a player disconnected.
    if(evg.event_ID == EV_DISCONNECT){
        if(evg.player_number == playerNumber){
            PlayerDisconnectedOnEntry();
        } else {
            PlayerInEndOnEntry(playerNumber);
        }
        
        
    }
}

void Player::PlayerWantUnpauseOnEntry(){
    if(P_ON_ENTRY) std::cout << "Player::PlayerWantUnpauseOnEntry " << playerNumber << "\n";

    state = PlayerWantUnpause;
    stateString = "PlayerWantUnpause";
    activeHandler = &Player::PlayerWantUnpauseHandler; 

    if(otherPlayer->state == PlayerWantUnpause){
        Event<EV_UNPAUSE_GAME> unpauseEvent(-1);
        eventQueue->add_event(unpauseEvent.buffer_b);
    }

}

void Player::PlayerWantUnpauseHandler(uint8_t* data){
    if(P_ON_HANDLER) std::cout << "Player::PlayerWantUnpauseHandler " << playerNumber << "\n";
    Event<EV_GENERIC> evg(data);

    if(evg.event_ID == EV_UNPAUSE_GAME){
        PlayerInGameOnEntry();
        sender(evg.buffer_b, HEADER_LEN);
    }

    if(evg.event_ID == EV_PRESSED_SPACE && evg.player_number == playerNumber){
        PlayerFreezeOnEntry();
    }   


    // Check if a player disconnected.
    if(evg.event_ID == EV_DISCONNECT){
        if(evg.player_number == playerNumber){
            PlayerDisconnectedOnEntry();
        } else {
            PlayerInEndOnEntry(playerNumber);
        }
        
        
    }
}


void Player::PlayerInEndOnEntry(int winner){
    if(P_ON_ENTRY) std::cout << "PlayerInEndOnEntry " << playerNumber << "\n";

    state = PlayerInEnd;   
    stateString = "PlayerInEnd";
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


    // Check if a player disconnected.
    if(evg.event_ID == EV_DISCONNECT){
        if(evg.player_number == playerNumber){
            PlayerDisconnectedOnEntry();
        } else {
            PlayerInEndOnEntry(playerNumber);
        }
        
        
    }
}