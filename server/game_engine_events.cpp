#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <thread>
#include <semaphore.h>
#include <CL/opencl.hpp>
#include <mutex>
#include <netinet/in.h>
#include "../macros.hpp"
#include "event_queue.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"

void game_engine::on_connect(int *data){
    int player_number = data[1];
    int socket = data[2];
    if(VERBOSE>0){
        std::cout << "on_connect: player" << player_number << " in socket " << socket << "\n" << std::flush;
    }

    connection_status[player_number] = true;
    sockets[player_number] = socket;

    if(connection_status[0] && connection_status[1]){
        can_toggle = true;
    }

    threads.push_back(std::thread(&game_engine::listener, this, player_number));

    eq->add_event(EV_SEND_INIT_INFO, player_number);
    eq->add_event(EV_CHANGE_SCREEN, player_number, 0);

}

void game_engine::on_disconnect(int *data){
    int player_number = data[1];
    if(VERBOSE>0){
        std::cout << "p" << player_number << " on_disconnect\n";
    }


    // Check if this function was already called
    if(!connection_status[player_number]) return;

    can_toggle = false;
    ready[0] = 0;
    ready[1] = 0;
    paused = true;
    streaming[player_number] = false;
    connection_status[player_number] = false;
    sockets[player_number] = -1;

    // Tell the other player to update the screen
    eq->add_event(EV_CHANGE_SCREEN,1-player_number,1);

}

void game_engine::on_change_screen(int *data){
    int player_number = data[1];
    if(VERBOSE>0){ std::cout << "p" << player_number << " on_change_screen\n";}

    int screen_number = data[2];
    int data_send[HEADER_LEN];
    data_send[0] = PAYLOAD_OFF;
    data_send[1] = EV_CHANGE_SCREEN;
    data_send[2] = screen_number;

    sender(player_number, data_send, HEADER_LEN);
}

void game_engine::on_send_init_info(int *data){
    int player_number = data[1];
    if(VERBOSE>0){std::cout << "p" << player_number << " on_send_init_info\n";}


    int data_send[HEADER_LEN];
    data_send[0] = PAYLOAD_OFF;
    data_send[1] = EV_SEND_INIT_INFO;
    data_send[2] = player_number;

    sender(player_number, data_send, HEADER_LEN);
}

void game_engine::on_update_status(int *data){
    int player_number = data[1];
    if(VERBOSE>0){std::cout << "p" << player_number << " on_update_status\n";}

    int data_send[HEADER_LEN];
    data_send[0] = PAYLOAD_OFF;
    data_send[1] = EV_UPDATE_STATUS;
    data_send[2] = ready[0];
    data_send[3] = ready[1];
    data_send[4] = connection_status[0];
    data_send[5] = connection_status[1];

    sender(player_number, data_send, HEADER_LEN);
}


void game_engine::toggle_ready(int player_number){
    if(VERBOSE>0){
        std::cout << "p" << player_number << " toggle_ready\n";
    }
    ready[player_number] = !ready[player_number];
    
    // Both players agree to unpause
    if(paused && ready[0] && ready[1]){
        paused = false;
        ready[0] = 0;
        ready[1] = 0;
    } 

    // One player pauses
    else if(!paused && ( ready[0] ||  ready[1])){
        paused = true;
        ready[0] = 0;
        ready[1] = 0;
    } 
}


void game_engine::on_pressed_space(int *data){
    int player_number = data[1];
    if(VERBOSE>0){std::cout << "p" << player_number << " on_pressed_space\n";}

    // int previously_paused = paused;
    if(can_toggle) toggle_ready(player_number);

    // If the state switched into paused, update the players' screens
    // if((paused != previously_paused) && paused){
    if(paused &&  can_toggle){
        int screen_number = 1 + ready[0] + 2*ready[1];
        streaming[0] = false;
        streaming[1] = false;
        eq->add_event(EV_CHANGE_SCREEN, 0, screen_number);
        eq->add_event(EV_CHANGE_SCREEN, 1, screen_number);
    } else 

    // If the state switched into running, then streaming can begin
    // if(can_toggle && (paused != previously_paused) && !paused){
    if(can_toggle && !paused){
        eq->add_event(EV_CHANGE_SCREEN, 0, 5);
        eq->add_event(EV_CHANGE_SCREEN, 1, 5);
        streaming[0] = true;
        streaming[1] = true;
        threads.push_back(std::thread(&game_engine::streamer, this, 0));
        threads.push_back(std::thread(&game_engine::streamer, this, 1));        
    } else 

    if(!can_toggle && game_finished){
        ready_to_toggle[player_number] = true;
        eq->add_event(EV_CHANGE_SCREEN, player_number, 0);

        if(ready_to_toggle[0] && ready_to_toggle[1]){
            can_toggle = true;
        }
    }

    // eq->add_event(EV_UPDATE_STATUS, player_number);
}



void game_engine::on_player_won(int *data){

    int player_number = data[1];
    if(VERBOSE>0){
        std::cout << "p" << player_number << " on_player_won\n";
    }

    // Send victory screen
    eq->add_event(EV_CHANGE_SCREEN, player_number, 6);
    eq->add_event(EV_CHANGE_SCREEN, 1-player_number, 7);
    game_finished = true;
    streaming[0] = false;
    streaming[1] = false;
    ready_to_toggle[0] = false;
    ready_to_toggle[1] = false;
    can_toggle = false;
}


