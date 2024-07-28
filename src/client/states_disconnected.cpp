#include "front_end.hpp"
#include "states.hpp"
#include "../macros.hpp"
#include "../event_queue.hpp"

#include <unistd.h>
#include <iostream>



StateDisconnected::StateDisconnected(frontEnd *contex):
    State(contex){
        
    std::cout << "Entered StateDisconnected constructor\n";
    
    std::string ip =  "127.0.0.1";
    unsigned port = 8080;


    // std::string ip =  "213.181.110.225";
    // unsigned port = 42381;

    connector_running = true;
    listener_running = false;
    worker_thread = std::thread(&StateDisconnected::connect_to_server, this, ip, port);
}



void StateDisconnected::handle(uint8_t *data){
    std::cout << "StateDisconnected::handle entered\n";
    Event<EV_GENERIC> ev(data);
    int event_ID = ev.event_ID;

    if(event_ID == EV_CONNECT){
        std::cout << "StateDisconnected::handle event EV_CONNECT\n";
        Event<EV_CONNECT> ev_c(data);
        sock = ev_c.socket;
        connector_running = false;
        worker_thread.join();
        listener_running = true;
        worker_thread = std::thread(&StateDisconnected::listen_server_events, this, sock);

    } else if(event_ID == EV_SEND_INIT_INFO){
        std::cout << "StateDisconnected::handle event EV_SEND_INIT_INFO\n";
        Event<EV_SEND_INIT_INFO> ev(data);
        int player_number = ev.player_number;
        // These should be obtained from EV_SEND_INIT_INFO
        int width = 300;
        int height = 700;
        listener_running = false;
        worker_thread.join();
        std::cout << "worker thread joined\n";
        
        send_event_to_server(Event<EV_CONNECT_REPLY>(player_number).buffer_b, HEADER_LEN, sock);
        context->setState(new StateConnected(context, width, height, player_number, sock));

    } else {
        std::cout << "UNHANDLED\n";
    }   
}






void StateDisconnected::connect_to_server(std::string ip, unsigned port){
    // Connects to the server and listens a specific event
    // if(VERBOSE>0){ std::cout << "Entered client::connect_and_listen_server_events\n";}
    
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        exit(1);
    }


    // CHANGE: exponential connection time attempts    
    while(connector_running && connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        std::cout << "client: attempting to connect to server \n" << std::flush;
        usleep(100*1000);
    }



    context->eventQueue.add_event(Event<EV_CONNECT>(-1, sock).buffer_b);
    std::cout << "left StateDisconnected::connect_to_server\n";
}


void StateDisconnected::listen_server_events(int sock){


    fd_set readfds;
    struct timeval timeout;
    uint8_t buffer_header[HEADER_LEN];
    unsigned Nbytes_header = HEADER_LEN;
    
    while(listener_running){
        // Set up the fd_set for select. These structs get modified by 'select', so it's
        // required to reset them everytime
        
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);        
        timeout.tv_usec = 0;
        timeout.tv_sec = 1;  
        int activity = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

        if(activity==0){
            std::cout << "no activity on sock " << sock << "\n";
            continue;

        } else if (activity < 0) {
            context->eventQueue.add_event(Event<EV_DISCONNECT>(-1).buffer_b);
            break;        
        }
    
        std::cout << "Listener on sock " << sock <<  " \n" << std::flush;
        int bytesReceived = recv(sock, buffer_header, Nbytes_header*sizeof(uint8_t), MSG_WAITALL);
        std::cout << "Listener received something on sock " << sock <<  " \n" << std::flush;

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
    
            
        

        if(bytesReceived>0){
            for(unsigned i=0; i<10; i++){
                std::cout << (int)buffer_header[i] << " ";
            }
            context->eventQueue.add_event(buffer_header);
        }
        
    }
    if(VERBOSE>0){ std::cout << "Left server listener\n";}


}

