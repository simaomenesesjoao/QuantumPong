#ifndef STATE_MACHINES_H
#define STATE_MACHINES_H 1

#include <thread>
#include <string>
#include <cstdint>
#include "../event_queue.hpp"
#include <unistd.h>
#include "simulator.hpp"

class Server;

class Player{
public:
    event_queue *eventQueue;
    int playerNumber;
    int socket;
    std::vector<std::thread> threads;
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

    Player(event_queue* eq, int pNum);
    ~Player();
    void addOtherPlayer(Player *player);   
    void addServer(Server *serv);
    void handle(uint8_t* data);
    void streamer();
    void listener();
    void sender(uint8_t*, unsigned);

    void PlayerDisconnectedHandler(uint8_t* data);

    void PlayerIdleOnEntry();
    void PlayerIdleHandler(uint8_t* data);

    void PlayerWantNewOnEntry();
    void PlayerWantNewHandler(uint8_t* data);

    void PlayerInGameOnEntry();
    void PlayerInGameHandler(uint8_t* data);

    void PlayerFreezeOnEntry();
    void PlayerFreezeHandler(uint8_t* data);

    void PlayerWantUnpauseOnEntry();
    void PlayerWantUnpauseHandler(uint8_t* data);

    void PlayerInEndOnEntry(int);
    void PlayerInEndHandler(uint8_t* data);
};


class Server{
public:
    event_queue *eventQueue;
    simulator *engine;
    void (Server::*activeHandler)(uint8_t*);

    int GameOff = 0;
    int GameRunning = 1;
    int GamePaused = 2;
    int state;
    std::string stateStrings[10];

    Player *player1, *player2;
    
    Server(event_queue* eq);
    void addPlayers(Player *p1, Player *p2);
    void handle(uint8_t*);
    void addSimulator(simulator*);

    void GameOffOnEntry();
    void GameOffHandler(uint8_t*);

    void GameRunningOnEntry();
    void GameRunningHandler(uint8_t*);

    void GamePausedOnEntry();    
    void GamePausedHandler(uint8_t*);
};


#endif // STATE_MACHINES_H