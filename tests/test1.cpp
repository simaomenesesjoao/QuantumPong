#include "../src/client/front_end.hpp"
#include "../src/server/game_engine.hpp"
#include <thread>
#include <vector>
#include <unistd.h>
#include <iostream>



void contained(frontEnd *client1, frontEnd *client2){ 
    std::cout << "clients\n";
    int delay = 100;
    int buffer_SDL[HEADER_LEN]; // buffer with SDL events
    unsigned ev_window_id;

    client1->init();
    client2->init();
    while(!client1->cl.close || !client2->cl.close) {


    
        bool SDL_has_data = client1->gr.get_one_SDL_event(buffer_SDL, &ev_window_id);


        std::cout << "ev,window: " << buffer_SDL[0] <<  " " << ev_window_id << "\n";

        if(!client1->cl.close){
            if(SDL_has_data && ev_window_id == client1->gr.window_ID)
                client1->process_SDL(buffer_SDL);
                
            client1->process_server();
        }


        if(!client2->cl.close){
            if(SDL_has_data && ev_window_id == client2->gr.window_ID)
                client2->process_SDL(buffer_SDL);
                
            client2->process_server();
        }

        usleep(delay*1000); 

    }
    client2->finalize();
    client1->finalize();
}

void containServer(game_engine *engine){
    engine->game_loop();
    engine->finalize();
}


int main(){
    std::vector<std::thread> threads;

    game_engine engine;

    engine.init();
    threads.push_back(std::thread(&containServer, &engine));

    usleep(100*1000);
    frontEnd client1, client2;    
    threads.push_back(std::thread(&contained, &client1, &client2));

    

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    } 
    
    return 0;
}