#include <iostream>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <netinet/in.h>
#include <CL/opencl.hpp>
#include "../macros.hpp"
#include <semaphore.h>
#include "event_queue.hpp"
#include "simulator.hpp"
#include "connection_handler.hpp"
#include "game_engine.hpp"

void game_engine::streamer(int player_number){
    if(VERBOSE>0){ std::cout << "Entered streamer for player " << player_number << "\n";}
    int delay=25;

    int socket = sockets[player_number];

    while(true){
        if(!streaming[player_number]) break;
        std::cout << "s" << player_number; //<< " " << physics->buffer_f_size;

        usleep(delay*1000);

        int result = send(socket, physics->buffer_f, physics->buffer_f_size*sizeof(uint8_t), MSG_NOSIGNAL);

        if(result < 0){
            eq->add_event(EV_DISCONNECT, player_number);
            break;
        }
    }

};

void game_engine::sender(int player_number, int *data, unsigned len_data){
    int socket = sockets[player_number];
    uint8_t data_b[len_data];
    for(unsigned i=0; i<len_data; i++){
        data_b[i] = data[i];
    }

    std::cout << "Sending to socket " << socket << " " << (int)data[0] << " " << (int)data[1] << "with lenegth" << len_data <<  "\n";
    int result = send(socket, data_b, len_data*sizeof(uint8_t), MSG_NOSIGNAL);

    if(result < 0){
        eq->add_event(EV_DISCONNECT, player_number);
    }
}

void game_engine::listener(int player_number){
    int socket = sockets[player_number];

    uint8_t* read_buf;
    read_buf = new uint8_t[HEADER_LEN];

    if(VERBOSE>0){ std::cout << "Entered listener for player " << player_number << " in socket " << socket << "\n";}

    int event;
    int data;

    while (true) {
        int result = recv(socket, read_buf, HEADER_LEN*sizeof(uint8_t), MSG_WAITALL);
        std::cout << "socket: " << socket << "read res: " << result << "\n" << std::flush;

        if(result>0){
            for(unsigned i=0; i<HEADER_LEN; i++){
                std::cout << (int)read_buf[i] << " ";
            }
            event = (int)read_buf[1];
            data = (int)read_buf[2];
            eq->add_event(event, player_number, data);
        }
        
        if(result <= 0){
            eq->add_event(EV_DISCONNECT, player_number);
            break;
        }

    }

    delete [] read_buf;
}

void game_engine::set_event_names(){
    event_names[EV_CONNECT]    = "EV_CONNECT";
    event_names[EV_DISCONNECT] = "EV_DISCONNECT";
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
    int delay = 25; // time delay in miliseconds of the main game loop
    bool running = true;
    int event = -1;
    int *data;

    event_queue eq1(10);
    eq = &eq1;

    connection_handler conn(eq);
    conn.init(8080);
    connection_status = conn.connection_status;


    unsigned Lx = 300;
    unsigned Ly = 700;
    unsigned pad = 1;
    unsigned local = 2;

    // Time evolution operator parameters
    float dt = 2.0;
    unsigned Ncheb = 10;

    unsigned height = Ly;
    unsigned width = Lx;

    // initial wavefunction parameters: center, momentum, spread
    unsigned ix = Lx/2;
    unsigned iy = Ly/2;
    float kx = 0.5;
    float ky = 1.0;
    float broad = 20.0;

    unsigned Ntimes = 200000;

    bool showcase = false;

    simulator engine(eq);
    engine.init_cl();
    engine.showcase = showcase;
    engine.init_geometry(Lx, Ly, pad, local);
    engine.init_window(Lx, Ly);
    engine.set_hamiltonian_sq();
    engine.init_buffers();
    engine.init_kernels();
    engine.initialize_tevop(dt, Ncheb);
    engine.set_H();
    if(showcase) engine.initialize_pot_from();
    engine.initialize_wf(ix, iy, kx, ky, broad);

    unsigned paddle_x = 100;
    unsigned paddle_y_bot = 50;
    unsigned paddle_y_top = Ly-50;
    unsigned paddle_width = 100;
    unsigned paddle_height = 20;
    engine.init_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot, paddle_width, paddle_height);
    engine.update_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot);

    engine.paused = &paused;
    physics = &engine;
    
    threads.push_back(std::thread(&connection_handler::process_connections, &conn));
    threads.push_back(std::thread(&simulator::loop, &engine, Ntimes));

    while(running){
        usleep(delay*1000);
        eq->read(&event, &data);

        if(event == EV_CONNECT) on_connect(data);
        if(event == EV_DISCONNECT) on_disconnect(data);
        if(event == EV_CHANGE_SCREEN) on_change_screen(data);
        if(event == EV_SEND_INIT_INFO) on_send_init_info(data);
        if(event == EV_PRESSED_SPACE) on_pressed_space(data);
        if(event == EV_UPDATE_STATUS) on_update_status(data);
        if(event == EV_PLAYER_WON) on_player_won(data);



        print_state();
        event = -1;
    }
    
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    // t3.join();

}
