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

void game_engine::streamer(int player_number){
    if(VERBOSE>0){ std::cout << "Entered streamer for player " << player_number << "\n";}
   
    int socket = sockets[player_number];

    auto start = std::chrono::system_clock::now();

    while(true){
        if(!streaming[player_number]) break;

        // std::cout << "buffer\n";
        // for(unsigned i=0; i<10; i++){
        //     std::cout << (int)physics->buffer_f[i] << " ";
        // }
        // std::cout << "\n";

        int result = send(socket, physics->buffer_f, physics->buffer_f_size*sizeof(uint8_t), MSG_NOSIGNAL);

        if(result < 0){
            eq->add_event(Event<EV_DISCONNECT>(player_number).buffer_b);
            break;
        }

        
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        // std::cout << "elapsed time p: " << player_number << " " << elapsed_seconds.count() << "s" << std::endl;
        start = std::chrono::system_clock::now();
        usleep(delay_streamer);
    }

}


void game_engine::sender2(int player_number, uint8_t *data, unsigned length){
    int socket = sockets[player_number];

    std::cout << "Sending to socket " << socket << " " << (int)data[0] << " " << (int)data[1] << "with length" << length <<  "\n";
    int result = send(socket, data, length*sizeof(uint8_t), MSG_NOSIGNAL);

    int n;
    // std::cout << "game_engine::sender2:\n";
    // for(int i=0; i<length; i++){
    //     std::cout << (int)data[i] << " ";
    // }
    // std::cout << "Done\n";

    if(result < 0){
        eq->add_event(Event<EV_DISCONNECT>(player_number).buffer_b);
    }
}

void game_engine::listener(int player_number){
    int socket = sockets[player_number];
    
    uint8_t* read_buf;
    read_buf = new uint8_t[HEADER_LEN];

    if(VERBOSE>0){ std::cout << "Entered listener for player " << player_number << " in socket " << socket << "\n";}


    while (true) {
        int result = recv(socket, read_buf, HEADER_LEN*sizeof(uint8_t), MSG_WAITALL);
        std::cout << "socket: " << socket << "read res: " << result << "\n" << std::flush;

        if(result>0){
            for(unsigned i=0; i<10; i++){
                std::cout << (int)read_buf[i] << " ";
            }
            eq->add_event(read_buf);
        }
        
        if(result <= 0){
            eq->add_event(Event<EV_DISCONNECT>(player_number).buffer_b);
            break;
        }

    }

    delete [] read_buf;
}

game_engine::game_engine(){
    can_toggle = true;
    paused = true;
    game_finished = false;
    for(unsigned i=0; i<2; i++){
        ready[i] = false;
        sockets[i] = -1;
        ready_to_toggle[i] = true;
        streaming[i] = false;
    }
}

void game_engine::print_state(){
    std::cout << "Printing game state:\n";
    std::cout << "Connection status: " << connection_status[0] << " " << connection_status[1];
    std::cout << "  Sockets: " << sockets[0] << " " << sockets[1];
    std::cout << "  Ready: " << ready[0] << " " << ready[1] << "  Paused:" << paused << "  Can toggle:" << can_toggle << "\n";
    std::cout << "  ready to toggle: " << ready_to_toggle[0] << " " << ready_to_toggle[1];
    std::cout << "  streaming: " << streaming[0] << " " << streaming[1] << "\n";
}

void game_engine::game_loop(){
    delay_event_loop = 25*1000; 
    delay_streamer = 80*1000;
    int delay_simulation = 25*1000;

    bool running = true;
    int event = -1;
    uint8_t *data;

    event_queue eq1(30);
    eq = &eq1;

    connection_handler conn(eq);
    conn.init(8080);
    connection_status = conn.connection_status;

    // std::cout << "engine1: connection status: " << conn.connection_status[0] << " " << conn.connection_status[1] << "\n" << std::flush;
    unsigned Lx = 300;
    unsigned Ly = 700;
    simulator engine(eq);
    engine.init(Lx, Ly, delay_simulation);
    engine.paused = &paused;
    physics = &engine;
    
    threads.push_back(std::thread(&connection_handler::process_connections, &conn));
    threads.push_back(std::thread(&simulator::loop, &engine));

    // std::cout << "engine2: connection status: " << connection_status[0] << " " << connection_status[1] << "\n" << std::flush;

    while(running){
        usleep(delay_event_loop);
        eq->read(&event, &data);

        if(event == EV_CONNECT) on_connect(data);
        if(event == EV_DISCONNECT) on_disconnect(data);
        if(event == EV_CHANGE_SCREEN) on_change_screen(data);
        if(event == EV_SEND_INIT_INFO) on_send_init_info(data);
        if(event == EV_PRESSED_SPACE) on_pressed_space(data);
        if(event == EV_PRESSED_KEY) on_pressed_key(data);
        if(event == EV_UPDATE_STATUS) on_update_status(data);
        if(event == EV_PLAYER_WON) on_player_won(data);
        if(event == EV_MOUSEBUTTONDOWN) on_mousebuttondown(data);
        if(event == EV_SEND_POT) on_add_pot(data);
        // if(event == EV_ADD_MAG) on_add_mag(data);
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
