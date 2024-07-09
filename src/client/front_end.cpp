#include "front_end.hpp"
#include <iostream>
#include "../macros.hpp"
#include "../event_queue.hpp"
#include <unistd.h>


void frontEnd::on_pause_game(uint8_t *data){
    std::cout << "on_pause_game\n";
    gr.alpha = 100;
    gr.update();
}


void frontEnd::on_unpause_game(uint8_t *data){
    std::cout << "on_unpause_game\n";
    gr.alpha = 255;
    gr.update();
}

void frontEnd::on_end_screen(uint8_t *data){
    std::cout << "on_end_screen\n";

    Event<EV_END_SCREEN> event(data);
    gr.endScreen(event.player_number == cl.player_number);
    gr.reset_potential();
    gr.reset_wavefunction();
    gr.reset_magnetic();
    gr.score_top = 0;
    gr.score_bot = 0;


    
}


void frontEnd::on_update_status(uint8_t *data){
    std::cout << "on_update_status\n";

    Event<EV_UPDATE_STATUS> event(data);
    std::cout << "statuses: " << event.state_p1 << " " << event.state_p2 << "\n";
    
    gr.update_lobby(event.state_p1, event.state_p2);
}


void frontEnd::on_send_init_info(uint8_t *data){
    
    Event<EV_SEND_INIT_INFO> event(data);
    cl.player_number = event.player_number;
    gr.initAfterPlayerNumber(event.player_number);
       
    std::cout << "on_send_init_info. player number is: " << cl.player_number << "\n";
}

void frontEnd::on_stream(uint8_t *data){
    Event<EV_STREAM> event(data);

    gr.update_wavefunction(cl.buffer_receive);
    gr.update();
    gr.score_top = event.score_top;
    gr.score_bot = event.score_bot;
}


void frontEnd::on_paddle_update(uint8_t *data){
    Event<EV_PADDLE_UPDATE> event(data);
    int player = event.player_number;

    if(player == 0){
        gr.x0 = event.x;
        gr.y0 = event.y;
    }
    if(player == 1){
        gr.x1 = event.x;
        gr.y1 = event.y;
    }

    gr.update();
}

void frontEnd::on_send_pot(uint8_t *data){
    std::cout << "main: entered on_send_pot\n" << std::flush;

    Event<EV_SEND_POT> event(data);
    int x = event.x;
    int y = event.y;
    int dx = event.dx;
    int dy = event.dy;

    // Update paddle positions
    gr.x0 = event.x0;
    gr.y0 = event.y0;
    gr.x1 = event.x1;
    gr.y1 = event.y1;

    gr.update_potential(x, y, dx, dy, cl.buffer_receive);
    gr.update();
    std::cout << "main: left on_send_pot\n" << std::flush;
}


void frontEnd::on_send_mag(uint8_t *data){
    std::cout << "main: entered on_send_mag\n" << std::flush;

    Event<EV_SEND_MAG> event(data);
    int x = event.x;
    int y = event.y;
    int dx = event.dx;
    int dy = event.dy;

    gr.update_magnetic(x, y, dx, dy, cl.buffer_receive);
    gr.update();
    std::cout << "main: left on_send_mag\n" << std::flush;
}

void frontEnd::on_pressed_space(uint8_t *data){
    std::cout << "on_pressed_space\n";
    cl.send_event_to_server(data, HEADER_LEN);
}

void frontEnd::on_pressed_key(uint8_t *data){
    std::cout << "on_pressed_key\n";
    cl.send_event_to_server(data, HEADER_LEN);
}

void frontEnd::on_mousebuttondown(uint8_t *data){
    std::cout << "on_mousebuttondown\n";    
    cl.send_event_to_server(data, HEADER_LEN);
}







void frontEnd::init(){

    unsigned width = 300;
    unsigned height = 700;
    std::string ip =  "127.0.0.1";
    unsigned port = 8080;

    // Initialize graphics
    gr.init(width, height);
    
    // Initialize the connection to the server
    cl.initConnection(ip, port);
    cl.init_buffers(width*height);
    
    threads.push_back(std::thread(&client::read_one_from_server, &cl));

    
}


void frontEnd::game_loop(){

    int delay = 5;

    uint8_t data[HEADER_LEN];
    int event;
    int connection_status = -1;    
    auto start = std::chrono::system_clock::now();

    unsigned count = 0; 
    while(!cl.close) {

        // Attempt to connect
        if(connection_status == -1){
            connection_status = cl.connect_to_server();
            continue;
        }
        

        // Check if SDL has events
        bool SDL_has_data = gr.get_one_SDL_event(); 
        if(SDL_has_data){
            SDL_has_data = false;
            event   = gr.buffer_SDL[0];
            data[0] = gr.buffer_SDL[1];
            data[1] = gr.buffer_SDL[2];
            data[2] = gr.buffer_SDL[3];

            if(event == EV_SDL_QUIT){
                cl.close = true;
                std::cout << "SDL event quit\n";
                break; 
            }

            if(event == EV_PRESSED_SPACE){
                Event<EV_PRESSED_SPACE> event(cl.player_number);
                on_pressed_space(event.buffer_b);
            }

            if(event == EV_PRESSED_KEY){
                Event<EV_PRESSED_KEY> event(cl.player_number, data[0]);
                on_pressed_key(event.buffer_b);
            }

            if(event == EV_MOUSEBUTTONDOWN) {
                Event<EV_MOUSEBUTTONDOWN> event(cl.player_number, gr.buffer_SDL[1], gr.buffer_SDL[2], gr.buffer_SDL[3]);
                on_mousebuttondown(event.buffer_b);
            }

            event = -1;
        }

        // Check if server has events
        if(cl.socket_has_data){
            event = (int)cl.buffer_header[0];

            if(event == EV_SEND_INIT_INFO) on_send_init_info(cl.buffer_header);
            if(event == EV_UPDATE_STATUS) on_update_status(cl.buffer_header);
            if(event == EV_END_SCREEN) on_end_screen(cl.buffer_header);
            if(event == EV_STREAM) on_stream(cl.buffer_header);
            if(event == EV_SEND_POT) on_send_pot(cl.buffer_header);
            if(event == EV_SEND_MAG) on_send_mag(cl.buffer_header);
            if(event == EV_PADDLE_UPDATE) on_paddle_update(cl.buffer_header);
            
            if(event == EV_PAUSE_GAME) on_pause_game(cl.buffer_header);
            if(event == EV_UNPAUSE_GAME) on_unpause_game(cl.buffer_header);

            cl.socket_has_data = false;
            sem_post(&cl.semaphore1);

        }

        usleep(delay*1000); 

        count++;
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        start = std::chrono::system_clock::now();
    }

    

}
void frontEnd::finalize(){

    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    } 


    gr.finalize();
    cl.finalize();

}
