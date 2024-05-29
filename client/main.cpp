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
#include "shared_memory.hpp"
#include "client.hpp"
#include "graphics.hpp"

// g++ *.cpp -o client  `sdl2-config --libs --cflags` -Wall -lm && ./client 

void on_change_screen(shared_memory *memory, client *cl, graphics *gr, int *data){
    std::cout << "on_change_screen\n";
    int screen_number = data[2];
    if(screen_number == 0) gr->set_screen(200,0,0);
    if(screen_number > 0 && screen_number < 5) gr->set_screen(0,screen_number*50, 0);

    if(screen_number == 5) gr->set_screen(200,200,200);
    if(screen_number == 6) gr->set_screen(0,250,250);
    if(screen_number == 7) gr->set_screen(250,250,0);
}


void on_send_init_info(shared_memory *memory, client *cl, graphics *gr, int *data){
    
    cl->player_number = data[2];    
    std::cout << "on_send_init_info. player number is: " << cl->player_number << "\n";
}

void on_pressed_space(shared_memory *memory, client *cl, graphics *gr, int *data){
    std::cout << "on_pressed_space\n";
    // cl->player_number = data[2];

    uint8_t data_send[HEADER_LEN];
    data_send[0] = PAYLOAD_OFF;
    data_send[1] = EV_PRESSED_SPACE;
    
    cl->send_event_to_server(data_send, HEADER_LEN);
}

void on_stream(shared_memory *memory, client *cl, graphics *gr, int *data){
    std::cout << "on_stream\n";
    
    gr->update();
}



int main(){
    int delay = 25;
    int data[HEADER_LEN];
    shared_memory memory;

    unsigned width = 300;
    unsigned height = 700;

    graphics gr(&memory);
    gr.init(width, height);
    
    std::string ip =  "127.0.0.1";
    unsigned port = 8080;


    //std::string ip =  "192.168.8.109";
    //unsigned port = 8443;
    //std::string ip =  "194.164.120.17";

    client cl(&memory, ip, port);
    cl.init_buffers();
    


    std::thread t1 = std::thread(&client::read_one_from_server, &cl);

    int event;
    int connection_status = -1;    
    auto start = std::chrono::system_clock::now();

    unsigned count = 0; 
    while(!cl.close) {
        std::cout << "new iteration\n";

        // Attempt to connect
        if(connection_status == -1)
            connection_status = cl.connect_to_server();

        // Check if SDL has events
        bool SDL_has_data = gr.get_one_SDL_event(); 
        if(SDL_has_data){
            SDL_has_data = false;
            event = memory.SDL_buffer[0];

            if(event == EV_SDL_QUIT){
                cl.close = true;
                std::cout << "SDL event quit\n";
                break; 
            }

            if(event == EV_PRESSED_SPACE) on_pressed_space(&memory, &cl, &gr, data);

            event = -1;
        }

        // Check if server has events
        std::cout <<  "header len: " << HEADER_LEN << "\n";

        if(cl.socket_has_data){
            for(unsigned i=0; i<HEADER_LEN; i++){
                data[i] = (int)memory.header[i];
            }
            event = data[1];
            cl.socket_has_data = false;
            sem_post(&cl.semaphore1);

            if(event == EV_SEND_INIT_INFO) on_send_init_info(&memory, &cl, &gr, data);
            if(event == EV_CHANGE_SCREEN) on_change_screen(&memory, &cl, &gr, data);
            if(event == EV_STREAM) on_stream(&memory, &cl, &gr, data);
            

        }

        usleep(delay*1000); 

        count++;
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;    
        std::cout << "elapsed time: " << elapsed_seconds.count() << "s" << std::endl;
        start = std::chrono::system_clock::now();
    }

    std::cout << "Exited loop\n";
    t1.join();

    gr.finalize();
    cl.finalize();
    return 0;
}
