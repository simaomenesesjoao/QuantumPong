#include "front_end.hpp"
#include <iostream>
#include "../macros.hpp"
#include "../event_queue.hpp"
#include <unistd.h>
#include "states.hpp"


StateConnected::StateConnected(frontEnd *contex, int width, int height, int player_number, int sock):
    State(contex), player_number(player_number), width(width), height(height), sock(sock),
    buffer_wf(10,height*width,0), buffer_mag(10,height*width,0), buffer_pot(10,height*width,0),
    graphics(width, height, player_number, &(context->eventQueue)){
    // graphics takes care of the SDL listener, who connects to the event queue
    
    std::cout << "Entered StateConnected constructor. Information gathered:\n";
    std::cout << "player_number " << player_number << ", ";
    std::cout << "width, height " << width << " " << height << ", ";
    std::cout << "socket " << sock << "\n";

    server_listener_running = true;
    thread_server_listener = std::thread(&StateConnected::listen_server_events,this,sock);
}

void StateConnected::handle(uint8_t *data){
    Event<EV_GENERIC> evg(data);
    int id = evg.event_ID;

    if(id == EV_UPDATE_STATUS){
        Event<EV_UPDATE_STATUS> event(data);
        graphics.lobby.update_lobby(event.state_p1, event.state_p2);
    }

    if(id == EV_STREAM){
        // The server listener already updates the buffers
        std::cout << "frontEnd::on_stream. ";
        Event<EV_STREAM> event(data);

        graphics.game.update_wavefunction(&buffer_wf);

        std::cout << "score from event: " << event.score_top << " " << event.score_bot << "\n";
        graphics.game.score_top = event.score_top;
        graphics.game.score_bot = event.score_bot;
        graphics.game.update_score();

        graphics.game.update();
    }

    if(id == EV_SEND_POT){

        std::cout << "frontEnd:on_send_pot\n" << std::flush;

        Event<EV_SEND_POT> event(data);

        // Update paddle positions
        graphics.game.x0 = event.x0;
        graphics.game.y0 = event.y0;
        graphics.game.x1 = event.x1;
        graphics.game.y1 = event.y1;

        // Update potential buffer
        graphics.game.update_potential(event.x, event.y, event.dx, event.dy, &buffer_pot);

        // Draw
        graphics.game.update();
    }

    if(id == EV_SEND_MAG){
        std::cout << "frontEnd:EV_SEND_MAG\n" << std::flush;
        Event<EV_SEND_MAG> event(data);

        // Update magnetic buffer
        graphics.game.update_magnetic(event.x, event.y, event.dx, event.dy, &buffer_mag);

        // Draw
        graphics.game.update();
    }


    if(id == EV_PADDLE_UPDATE){
        std::cout << "frontEnd::on_paddle_update (17). ";
        Event<EV_PADDLE_UPDATE> event(data);
        int player = event.player_number;

        if(player == 0){
            graphics.game.x0 = event.x;
            graphics.game.y0 = event.y;
        }
        if(player == 1){
            graphics.game.x1 = event.x;
            graphics.game.y1 = event.y;
        }

        graphics.game.update();
    }


    if(id == EV_PAUSE_GAME){
        graphics.game.update_white(100);
        graphics.game.update();

    } 
    if(id == EV_UNPAUSE_GAME){
        graphics.game.update_white(0);
        graphics.game.update();
    }

    if(id == EV_END_SCREEN){

        std::cout << "EV_END_SCREEN::EV_END_SCREEN\n";

        Event<EV_END_SCREEN> event(data);
        graphics.end.endScreen(event.player_number == player_number);

        buffer_wf.reset();
        buffer_pot.reset();
        buffer_mag.reset();
        graphics.game.reset();
    }

    // These events (SDL events) are simply routed to the server
    if(id == EV_PRESSED_SPACE)   send_event_to_server(data, HEADER_LEN, sock);
    if(id == EV_PRESSED_KEY)     send_event_to_server(data, HEADER_LEN, sock);
    if(id == EV_MOUSEBUTTONDOWN) send_event_to_server(data, HEADER_LEN, sock);
}


void StateConnected::listen_server_events(int sock){

    fd_set readfds;
    struct timeval timeout;
    uint8_t buffer_header[HEADER_LEN];
    unsigned Nbytes_header = HEADER_LEN;
    
    while(server_listener_running){
        // Set up the fd_set for select. These structs get modified by 'select', so it's
        // required to reset them everytime
        
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);        
        timeout.tv_usec = 0;
        timeout.tv_sec = 1;  
        int activity = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

        if(activity==0){
            // std::cout << "no activity on sock " << sock << "\n";
            continue;

        } else if (activity < 0) {
            context->eventQueue.add_event(Event<EV_DISCONNECT>(-1).buffer_b);
            break;        
        }
    
        // std::cout << "Listener on sock " << sock <<  " \n" << std::flush;
        int bytesReceived = recv(sock, buffer_header, Nbytes_header*sizeof(uint8_t), MSG_WAITALL);
        // std::cout << "Listener received something on sock " << sock <<  " \n" << std::flush;

        if(bytesReceived <= 0){
            context->eventQueue.add_event(Event<EV_DISCONNECT>(-1).buffer_b);
            break;
        }
        
        Event<EV_GENERIC> event(buffer_header);
        int event_id = event.event_ID;
        int payload_size = event.payload_size;
        
        std::cout << "received: " << bytesReceived << " from server event " << event_id << " payload:" << payload_size << "\n" << std::flush;
        
    
        for(unsigned i=0; i<10; i++){
            std::cout << (int)buffer_header[i] << " ";
        }
        std::cout << "\n";
    

        if(payload_size > 0){

            buffer *buf;
            switch(event_id){
                case(EV_STREAM):
                    buf = &buffer_wf;
                    std::cout << "buffer wf\n" << std::flush;
                    break;

                case(EV_SEND_POT):
                    buf = &buffer_pot;
                    std::cout << "buffer pot\n" << std::flush;
                    break;

                case(EV_SEND_MAG):
                    buf = &buffer_mag;
                    std::cout << "buffer mag\n" << std::flush;
                    break;

                default:
                    std::cout << "Selecting buffer unsuccessful\n";
                    break;
            }
                
            std::cout << "reading from payload\n" << std::flush;
            buf->lock();
            int bytesReceivedPayload = recv(sock, buf->write_data, payload_size*sizeof(uint8_t),MSG_WAITALL);
            buf->unlock();
            std::cout << "finished reading from payload\n" << std::flush;

            if(bytesReceivedPayload <= 0){
                context->eventQueue.add_event(Event<EV_DISCONNECT>(-1).buffer_b);
                break;
            }
        }

        

        if(bytesReceived>0){
            for(unsigned i=0; i<10; i++){
                std::cout << (int)buffer_header[i] << " ";
            }
            context->eventQueue.add_event(buffer_header);
        }
        
    }
    if(VERBOSE>0){ std::cout << "Left server listener\n";}


}