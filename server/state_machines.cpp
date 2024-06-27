#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include "../event_queue.hpp"
#include <unistd.h>
#define N 10


// Using a simpler implementation of a State Machine 
class Server;

class Player{
public:
    event_queue *eventQueue;
    int playerNumber;
    void (Player::*activeHandler)(uint8_t*);

    int PlayerDisconnected = 0;
    int PlayerIdle = 1;
    int PlayerWantNew = 2;
    int PlayerInGame = 3;
    int PlayerFreeze = 4;
    int PlayerWantUnpause = 5;
    int PlayerInEnd = 6;
    int state;


    std::string stateStrings[10];
    
    Player *otherPlayer;
    Server *server;
    // std::string stateString = "default";
    
    
    Player(event_queue* eq, int pNum){
        eventQueue = eq;
        playerNumber = pNum;
        activeHandler = &Player::PlayerDisconnectedHandler;
        state = PlayerDisconnected;
        stateStrings[PlayerDisconnected] = "PlayerDisconnected";
        stateStrings[PlayerIdle]         = "PlayerIdle";
        stateStrings[PlayerWantNew]      = "PlayerWantNew";
        stateStrings[PlayerInGame]       = "PlayerInGame";
        stateStrings[PlayerFreeze]       = "PlayerFreeze";
        stateStrings[PlayerWantUnpause]  = "PlayerWantUnpause";
        stateStrings[PlayerInEnd]  = "PlayerInEnd";
    }

    void addOtherPlayer(Player *player){otherPlayer = player;}    
    void addServer(Server *serv){server = serv;}    
    void handle(uint8_t* data){(this->*activeHandler)(data);}


    

    void PlayerDisconnectedHandler(uint8_t* data){
        std::cout << "PlayerDisconnectedHandler " << playerNumber << "\n";
        Event<EV_GENERIC> ev(data);

        if(ev.event_ID == EV_CONNECT && ev.player_number == playerNumber){
            activeHandler = &Player::PlayerIdleHandler;            
            PlayerIdleOnEntry();
        }
    }



    void PlayerIdleOnEntry(){
        std::cout << "PlayerIdleOnEntry " << playerNumber << "\n";
        state = PlayerIdle;
        // CHANGE: send lobby update here
    }

    void PlayerIdleHandler(uint8_t* data){
        std::cout << "PlayerIdleHandler " << playerNumber << "\n";
        Event<EV_GENERIC> evg(data);

        if(evg.event_ID == EV_PRESSED_KEY && evg.player_number == playerNumber){
            Event<EV_PRESSED_KEY> ev(data);
            if(ev.keycode == KEY_n){
                activeHandler = &Player::PlayerWantNewHandler;
                PlayerWantNewOnEntry();
            }
        }
    }



    void PlayerWantNewOnEntry(){
        state = PlayerWantNew;
        std::cout << "PlayerWantNewOnEntry " << playerNumber << "\n";
        std::cout << "mystate: " << state << " other state: " << otherPlayer->state << "\n";
        


        // Check if both players are in the WantNew state
        if(otherPlayer->state == PlayerWantNew){
            std::cout << "Both players want to start\n ";
            eventQueue->add_event(Event<EV_START_GAME>().buffer_b);
        }

        // CHANGE: send lobby update here
    }

    void PlayerWantNewHandler(uint8_t* data){
        std::cout << "PlayerWantNewHandler " << playerNumber << "\n";
        Event<EV_GENERIC> evg(data);

        // Toggle StartNewGame off
        if(evg.event_ID == EV_PRESSED_KEY && evg.player_number == playerNumber){
            Event<EV_PRESSED_KEY> ev(data);
            if(ev.keycode == KEY_n){
                activeHandler = &Player::PlayerIdleHandler; 
                PlayerIdleOnEntry();
            }
        }

        // Actually start the game
        if(evg.event_ID == EV_START_GAME){
            activeHandler = &Player::PlayerInGameHandler;
            PlayerInGameOnEntry();
        }
    }

    void PlayerInGameOnEntry(){

        std::cout << "PlayerInGameOnEntry " << playerNumber << "\n";
        state = PlayerInGame;
    }

    void PlayerInGameHandler(uint8_t* data){
        std::cout << "PlayerInGameHandler " << playerNumber << "\n";
        Event<EV_GENERIC> evg(data);

        // Quit GameKEY
        if(evg.event_ID == EV_PRESSED_KEY){
            Event<EV_PRESSED_KEY> ev(data);
            if(ev.keycode == KEY_esc){
                activeHandler = &Player::PlayerInEndHandler;
                PlayerInEndOnEntry();
            }
        }

        // Someone won
        if(evg.event_ID == EV_PLAYER_WON){
            activeHandler = &Player::PlayerInEndHandler;
            PlayerInEndOnEntry();
        }

        // Pause Game
        if(evg.event_ID == EV_PRESSED_SPACE){
            activeHandler = &Player::PlayerFreezeHandler;
            PlayerFreezeOnEntry();
        }

    }

    void PlayerFreezeOnEntry(){

        std::cout << "PlayerFreezeOnEntry" << playerNumber << "\n";
        state = PlayerFreeze;
    }

    void PlayerFreezeHandler(uint8_t* data){
        std::cout << "PlayerFreezeHandler " << playerNumber << "\n";
        Event<EV_GENERIC> evg(data);

        // Toggle StartNewGame off
        if(evg.event_ID == EV_PRESSED_SPACE && evg.player_number == playerNumber){
            activeHandler = &Player::PlayerWantUnpauseHandler; 
            PlayerWantUnpauseOnEntry();
        }
    }

    void PlayerWantUnpauseOnEntry(){
        std::cout << "PlayerWantUnpauseOnEntry " << playerNumber << "\n";
        state = PlayerWantUnpause;        
        if(otherPlayer->state == PlayerWantUnpause)
            eventQueue->add_event(Event<EV_UNPAUSE_GAME>(-1).buffer_b);

    }

    void PlayerWantUnpauseHandler(uint8_t* data){
        std::cout << "PlayerWantUnpauseHandler " << playerNumber << "\n";
        Event<EV_GENERIC> evg(data);

        if(evg.event_ID == EV_UNPAUSE_GAME){
            activeHandler = &Player::PlayerInGameHandler;
            PlayerInGameOnEntry();
        }

        if(evg.event_ID == EV_PRESSED_SPACE && evg.player_number == playerNumber){
            activeHandler = &Player::PlayerFreezeHandler;
            PlayerFreezeOnEntry();
        }
        
    }

    void PlayerInEndOnEntry(){
        std::cout << "PlayerInEndOnEntry " << playerNumber << "\n";
        state = PlayerInEnd;        
    }

    void PlayerInEndHandler(uint8_t* data){
        std::cout << "PlayerInEndHandler " << playerNumber << "\n";
        Event<EV_GENERIC> evg(data);

        // Enter
        if(evg.event_ID == EV_PRESSED_KEY && evg.player_number == playerNumber){
            Event<EV_PRESSED_KEY> ev(data);
            if(ev.keycode == KEY_return){
                activeHandler = &Player::PlayerIdleHandler;
                PlayerIdleOnEntry();
            }
        }


    }
};


class Server{
public:
    event_queue *eventQueue;
    void (Server::*activeHandler)(uint8_t*);

    int GameOff = 0;
    int GameRunning = 1;
    int GamePaused = 2;
    int state;
    std::string stateStrings[10];

    Player *player1, *player2;
    
    Server(event_queue* eq){
        eventQueue = eq;
        activeHandler = &Server::GameOffHandler;
        state = GameOff;
        stateStrings[GameOff]     = "GameOff";
        stateStrings[GameRunning] = "GameRunning";
        stateStrings[GamePaused]  = "GamePaused";
    }

    void addPlayers(Player *p1, Player *p2){
        player1 = p1;
        player2 = p2;
    }

    void handle(uint8_t* data){(this->*activeHandler)(data);}



    void GameOffOnEntry(){
        std::cout << "GameOffOnEntry\n";
        state = GameOff;
    }
    void GameOffHandler(uint8_t* data){
        std::cout << "GameOffHandler\n";
        Event<EV_GENERIC> evg(data);

        // Start the game
        if(evg.event_ID == EV_START_GAME){
            activeHandler = &Server::GameRunningHandler;
            GameRunningOnEntry();
        }
    }


    void GameRunningOnEntry(){
        std::cout << "GameRunningOnEntry\n";
        state = GameRunning;
    }
    void GameRunningHandler(uint8_t* data){
        std::cout << "GameOnHandler\n";
        Event<EV_GENERIC> evg(data);

        // Player quit
        if(evg.event_ID == EV_PRESSED_KEY){
            Event<EV_PRESSED_KEY> ev(data);
            if(ev.keycode == KEY_esc){
                activeHandler = &Server::GameOffHandler;
                GameOffOnEntry();
            }
        }

        if(evg.event_ID == EV_PLAYER_WON){
            activeHandler = &Server::GameOffHandler;
            GameOffOnEntry();
        }


        // Pause Game
        if(evg.event_ID == EV_PRESSED_SPACE){
            activeHandler = &Server::GamePausedHandler;
            GamePausedOnEntry();
        }

    }

    void GamePausedOnEntry(){
        std::cout << "GamePausedOnEntry\n";
        state = GamePaused;
    }
    void GamePausedHandler(uint8_t* data){
        std::cout << "GamePausedHandler\n";
        Event<EV_GENERIC> evg(data);

        // Unpause
        if(evg.event_ID == EV_UNPAUSE_GAME){
            activeHandler = &Server::GameRunningHandler;
            GameRunningOnEntry();
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
};




int main(){

    event_queue eq(20);
    int event = -1;
    uint8_t *data;




    // Create player and server instances, and pass the pointers
    Player player1(&eq, 0), player2(&eq, 1);
    Server server(&eq);
    
    player1.addOtherPlayer(&player2);
    player2.addOtherPlayer(&player1);
    player1.addServer(&server);
    player2.addServer(&server);
    server.addPlayers(&player1, &player2);


    


    int numZeros = 0; // Number of times the event queue reached zero events

    while(true){
        eq.read(&event, &data);
        std::cout << "event " << event << "\n"; 

        // Add events. The reason why I don't add them all in the beginning is because 
        // the program adds event internally in its runtime and those generated ones 
        // have to be processed before the remaining events on the event list enumerated below
        if(event<0){
            if(numZeros==0){                        
                eq.add_event(Event<EV_CONNECT>(0,4).buffer_b);
                eq.add_event(Event<EV_CONNECT>(1,5).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(0,KEY_n).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(0,KEY_n).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(1,KEY_n).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(0,KEY_n).buffer_b);
            } else if(numZeros==1){
                eq.add_event(Event<EV_PRESSED_SPACE>(0).buffer_b);
                eq.add_event(Event<EV_PRESSED_SPACE>(1).buffer_b);
                eq.add_event(Event<EV_PRESSED_SPACE>(1).buffer_b);
                eq.add_event(Event<EV_PRESSED_SPACE>(1).buffer_b);
                eq.add_event(Event<EV_PRESSED_SPACE>(0).buffer_b);
            } else if(numZeros==2){
                eq.add_event(Event<EV_PLAYER_WON>(1).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(1, KEY_return).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(1, KEY_n).buffer_b);
                eq.add_event(Event<EV_PRESSED_KEY>(0, KEY_return).buffer_b);
            } else {
                break;
            }
            numZeros++;
            continue;
        }

        std::cout << "states before: " << player1.stateStrings[player1.state] << " " << player2.stateStrings[player2.state] << " " << server.stateStrings[server.state] << "\n";
        player1.handle(data);
        player2.handle(data);
        server.handle(data);
        std::cout << " ----- states after: " << player1.stateStrings[player1.state] << " " << player2.stateStrings[player2.state] << " " << server.stateStrings[server.state] << "\n";
    }



    return 0;
}