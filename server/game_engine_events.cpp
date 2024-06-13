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
#include "../event_queue.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"

void game_engine::on_connect(uint8_t *data){
    Event<EV_CONNECT> event_on_connect(data);
    int player_number = event_on_connect.player_number;
    int socket = event_on_connect.socket;
    if(VERBOSE>0){
        std::cout << "on_connect: player" << player_number << " in socket " << socket << "\n" << std::flush;
    }

    connection_status[player_number] = true;
    sockets[player_number] = socket;

    if(connection_status[0] && connection_status[1]){
        can_toggle = true;
    }

    threads.push_back(std::thread(&game_engine::listener, this, player_number));

    eq->add_event(Event<EV_SEND_INIT_INFO>(player_number).buffer_b);
    eq->add_event(Event<EV_CHANGE_SCREEN>(player_number, 0).buffer_b);

}

void game_engine::on_disconnect(uint8_t *data){

    Event<EV_DISCONNECT> event(data);
    int player_number = event.player_number;

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

    int screen = 1;
    // Event<EV_CHANGE_SCREEN> event_change_screen(1-player_number, screen);
    eq->add_event(Event<EV_CHANGE_SCREEN>(1-player_number, screen).buffer_b);

}

void game_engine::on_change_screen(uint8_t *data){
    Event<EV_CHANGE_SCREEN> event(data);
    int player_number = event.player_number;
    if(VERBOSE>0){ std::cout << "p" << player_number << " on_change_screen\n";}
    sender2(player_number, data, HEADER_LEN);
}

void game_engine::on_send_init_info(uint8_t *data){
    Event<EV_SEND_INIT_INFO> event(data);
    int player_number = event.player_number;
    if(VERBOSE>0){std::cout << "p" << player_number << " on_send_init_info\n";}
    sender2(player_number, data, HEADER_LEN);
}

void game_engine::on_update_status(uint8_t *data){
    // CHANGE: these methods don't need to exist, using a generic event template
    //         all of this can be done in "sender2". Also change sender2 to sender
    Event<EV_UPDATE_STATUS> event(data);
    int player_number = event.player_number;
    if(VERBOSE>0){std::cout << "p" << player_number << " on_update_status\n";}
    sender2(player_number, data, HEADER_LEN);
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


void game_engine::on_pressed_space(uint8_t *data){
    Event<EV_PRESSED_SPACE> event(data);
    int player_number = event.player_number;
    if(VERBOSE>0){std::cout << "p" << player_number << " on_pressed_space\n";}

    // int previously_paused = paused;
    if(can_toggle) toggle_ready(player_number);

    // If the state switched into paused, update the players' screens
    // if((paused != previously_paused) && paused){
    if(paused &&  can_toggle){
        std::cout << "is paused and can toggle\n";
        int screen_number = 1 + ready[0] + 2*ready[1];
        streaming[0] = false;
        streaming[1] = false;

        for(unsigned player_number=0; player_number<2; player_number++)
            eq->add_event(Event<EV_CHANGE_SCREEN>(player_number, screen_number).buffer_b);
        
    } else 

    // If the state switched into running, then streaming can begin
    // if(can_toggle && (paused != previously_paused) && !paused){
    if(can_toggle && !paused){
        int screen_number = 5;
        for(unsigned player_number=0; player_number<2; player_number++)
            eq->add_event(Event<EV_CHANGE_SCREEN>(player_number, screen_number).buffer_b);
        
        eq->add_event(Event<EV_CHANGE_SCREEN>(player_number, screen_number).buffer_b);
        streaming[0] = true;
        streaming[1] = true;
        threads.push_back(std::thread(&game_engine::streamer, this, 0));
        threads.push_back(std::thread(&game_engine::streamer, this, 1));        
    } else 

    if(!can_toggle && game_finished){
        ready_to_toggle[player_number] = true;
        eq->add_event(Event<EV_CHANGE_SCREEN>(player_number, 0).buffer_b);

        if(ready_to_toggle[0] && ready_to_toggle[1]){
            can_toggle = true;
        }
    }

    eq->add_event(Event<EV_UPDATE_STATUS>(player_number, ready, connection_status).buffer_b);
}




void game_engine::on_pressed_key(uint8_t *data){
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

    int x0 = physics->top_player_x;
    int y0 = physics->top_player_y;
    int x1 = physics->bot_player_x;
    int y1 = physics->bot_player_y;

    if(player_number == 0){
        x0 += dx;
        y0 += dy;
    }

    if(player_number == 1){
        x1 += dx;
        y1 += dy;
    }
    
    // std::cout << "positions" << x0 << " " << y0 << " " << x1 << " " << y1 << "\n";
    physics->update_paddles(x0, y0, x1, y1);
}

void game_engine::on_player_won(uint8_t *data){

    Event<EV_PLAYER_WON> event(data);
    int player_number = event.player_number;

    if(VERBOSE>0){
        std::cout << "p" << player_number << " on_player_won\n";
    }

    // Send victory screen
    eq->add_event(Event<EV_CHANGE_SCREEN>(player_number, 6).buffer_b);
    eq->add_event(Event<EV_CHANGE_SCREEN>(1-player_number, 7).buffer_b);

    game_finished = true;
    streaming[0] = false;
    streaming[1] = false;
    ready_to_toggle[0] = false;
    ready_to_toggle[1] = false;
    can_toggle = false;
}

void game_engine::on_mousebuttondown(uint8_t *data){

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
        unsigned w = 70;
        float v = 0.9;
        physics->set_local_pot(x, y, w, w, v);
        
    }

    // if(button == 3){
    //     unsigned w = 70;
    //     float v = 0.005;
    //     physics->set_local_B(x, y, v);

    //     EventUnion<EV_ADD_POT> event;
    //     event.event_data.event_ID = EV_ADD_POT;
    //     event.event_data.has_payload = false;
    //     event.event_data.player_number = player_number;
    //     event.event_data.x = x;
    //     event.event_data.y = y;
    //     event.event_data.depth = v;
    //     event.event_data.radius = w;

    //     eq->add_event(event.buffer);
    // }

}

void game_engine::on_add_pot(uint8_t *data){
    if(VERBOSE>0){ std::cout << " on_add_pot\n";}

    Event<EV_SEND_POT> event(data);
    int x = event.x;
    int y = event.y;
    int dx = event.dx;
    int dy = event.dy;
    int Lx = physics->Lx;
    int Ly = physics->Ly;
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
    event.dx = dx;
    event.dy = dy;

    // std::cout << "event contains \n";
    // for(int i=0; i<HEADER_LEN;i++)
    //     std::cout << (int)event.buffer_b[i] << " ";
    // std::cout << "\n" << std::flush;

    std::cout << "pot event contains:\n";
    std::cout << event.x0 << " " << event.y0 << " " << event.x1 << " " << event.y1 << "\n";



    uint8_t buffer[buf_size+HEADER_LEN];
    for(unsigned i=0; i<HEADER_LEN; i++)
        buffer[i] = event.buffer_b[i];

    physics->get_pot(x, y, dx, dy, buffer+HEADER_LEN);

    // std::cout << "potential gotten from physics:\n";
    // int n;
    // for(int x0=0; x0<dx; x0++){
    //     for(int y0=0; y0<dy; y0++){
    //         n = x0 + dx*y0;
    //         std::cout << (int)buffer[n+HEADER_LEN] << " ";
    //     }
    //     std::cout << "\n";
    // }

    // When the simulator is set up, it sends out an event that the potential
    // has been modified (because it has), but the players may not yet be connected
    // so this event needs to be handled with care
    for(unsigned player=0; player<2; player++)
        if(connection_status[player])
            sender2(player, buffer, buf_size+HEADER_LEN);

}


// void game_engine::on_add_mag(int *data){
//     // int player_number = data[1];
//     // int x = data[2];
//     // int y = data[3];

//     // if(VERBOSE>0){ std::cout << "p" << player_number << " on_add_pot\n";}

//     // int data_send[HEADER_LEN];
//     // data_send[0] = PAYLOAD_OFF;
//     // data_send[1] = EV_ADD_POT;
//     // data_send[2] = x/256;
//     // data_send[3] = x%256;
//     // data_send[4] = y/256;
//     // data_send[5] = y%256;

//     // for(unsigned player=0; player<2; player++)
//     //     sender(player, data_send, HEADER_LEN);
// }

