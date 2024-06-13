#include <SDL2/SDL_events.h>
#include <iostream>
#include <semaphore.h>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <SDL2/SDL.h>
#include <chrono>
#include "../macros.hpp"
#include "../event_queue.hpp"
#include "shared_memory.hpp"
#include "client.hpp"
#include "graphics.hpp"

// g++ *.cpp -o client  `sdl2-config --libs --cflags` -Wall -lm && ./client 

void on_change_screen(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    std::cout << "on_change_screen\n";

    Event<EV_CHANGE_SCREEN> event(data);
    int screen_number = event.screen_number;
    
    if(screen_number == 0) gr->set_screen(200,0,0);
    if(screen_number > 0 && screen_number < 5) gr->set_screen(0,screen_number*50, 0);

    if(screen_number == 5) gr->set_screen(200,200,200);
    if(screen_number == 6) gr->set_screen(0,250,250);
    if(screen_number == 7) gr->set_screen(250,250,0);
}


void on_send_init_info(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    
    Event<EV_SEND_INIT_INFO> event(data);
    cl->player_number = event.player_number;
       
    std::cout << "on_send_init_info. player number is: " << cl->player_number << "\n";
}

void on_stream(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    Event<EV_STREAM> event(data);
    
    // gr->x0 = event.x0;
    // gr->y0 = event.y0;
    // gr->x1 = event.x1;
    // gr->y1 = event.y1;

    // std::cout << "paddles: " << memory->x0 << " " << memory->y0 << " " << memory->x1 << " " << memory->y1 << "\n";
    gr->update_wavefunction(cl->buffer_receive);
    gr->update();
}


void on_send_pot(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    std::cout << "main: entered on_send_pot\n" << std::flush;

    Event<EV_SEND_POT> event(data);
    int x = event.x;
    int y = event.y;
    int dx = event.dx;
    int dy = event.dy;

    // Update paddle positions
    gr->x0 = event.x0;
    gr->y0 = event.y0;
    gr->x1 = event.x1;
    gr->y1 = event.y1;
    // uint8_t *buffer;


    // std::cout << "x,y,dx,dy:" << x << " " << y << " " << dx << " " << dy <<"\n";
    // std::cout << "paddles: " << event.x0 << " " << event.y0 << " " << event.x1 << " " << event.y1 << "\n";


    // buffer = event.buffer_b+HEADER_LEN;
    // std::cout << "started printing buffer_receive\n";
    // int n;
    // for(int i=0; i<dx; i++){
    //     for(int j=0; j<dy; j++){
    //         n = i+dy*j;
    //         std::cout << (int)cl->buffer_receive[n] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // std::cout << "done\n"<<std::flush;

    gr->update_potential(x, y, dx, dy, cl->buffer_receive);
    gr->update();
    std::cout << "main: left on_send_pot\n" << std::flush;
}

void on_pressed_space(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    std::cout << "on_pressed_space\n";
    cl->send_event_to_server(data, HEADER_LEN);
}

void on_pressed_key(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    std::cout << "on_pressed_key\n";
    cl->send_event_to_server(data, HEADER_LEN);
}

void on_mousebuttondown(shared_memory *memory, client *cl, graphics *gr, uint8_t *data){
    std::cout << "on_mousebuttondown\n";    
    cl->send_event_to_server(data, HEADER_LEN);
}

int main(){
    int delay = 5;
    uint8_t data[HEADER_LEN];
    shared_memory memory;

    unsigned width = 300;
    unsigned height = 700;

    graphics gr(&memory);
    gr.init(width, height);
    
    std::string ip =  "127.0.0.1";
    unsigned port = 8080;


    //std::string ip =  "192.168.8.109";
    //unsigned port = 8443;

    client cl(&memory, ip, port);
    cl.init_buffers(width*height);
    
    std::thread t1 = std::thread(&client::read_one_from_server, &cl);

    int event;
    int connection_status = -1;    
    auto start = std::chrono::system_clock::now();

    unsigned count = 0; 
    while(!cl.close) {

        // Attempt to connect
        if(connection_status == -1)
            connection_status = cl.connect_to_server();

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
                on_pressed_space(&memory, &cl, &gr, event.buffer_b);
            }

            if(event == EV_PRESSED_KEY){
                Event<EV_PRESSED_KEY> event(cl.player_number, data[0]);
                on_pressed_key(&memory, &cl, &gr, event.buffer_b);
            }

            if(event == EV_MOUSEBUTTONDOWN) {
                Event<EV_MOUSEBUTTONDOWN> event(cl.player_number, gr.buffer_SDL[1], gr.buffer_SDL[2], gr.buffer_SDL[3]);
                on_mousebuttondown(&memory, &cl, &gr, event.buffer_b);
            }

            event = -1;
        }

        // Check if server has events
        if(cl.socket_has_data){
            event = (int)cl.buffer_header[0];
            

            if(event == EV_SEND_INIT_INFO) on_send_init_info(&memory, &cl, &gr, cl.buffer_header);
            if(event == EV_CHANGE_SCREEN) on_change_screen(&memory, &cl, &gr, cl.buffer_header);
            if(event == EV_STREAM) on_stream(&memory, &cl, &gr, cl.buffer_header);
            if(event == EV_SEND_POT) on_send_pot(&memory, &cl, &gr, cl.buffer_header);
            cl.socket_has_data = false;
            sem_post(&cl.semaphore1);

        }

        usleep(delay*1000); 

        count++;
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        start = std::chrono::system_clock::now();
    }

    std::cout << "Exited loop\n";
    t1.join();

    gr.finalize();
    cl.finalize();
    return 0;
}
