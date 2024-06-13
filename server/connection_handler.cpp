#include <cstdint>
#include <unistd.h>
#include <semaphore.h>
#include <mutex>
#include <netinet/in.h>
#include <iostream>
#include "../macros.hpp"
#include "../event_queue.hpp"
#include "connection_handler.hpp"

connection_handler::connection_handler(event_queue *ev_q){
    eq = ev_q;
}

void connection_handler::init(unsigned port){
    connection_status[0] = 0;
    connection_status[1] = 0;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    accepting_connections = true;
};



void connection_handler::process_connections(){

    while(accepting_connections){

        std::cout << "listening to connections. " << std::flush;
        std::cout << "connection status: " << connection_status[0] << " " << connection_status[1] << "\n" << std::flush;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }


        std::cout << "new connection in socket" << new_socket << "\n" << std::flush;

        // Assign player number and update number of players
        int player_number;
        if(!connection_status[0]){
            player_number = 0;
        } else { 
            if(!connection_status[1]){
                player_number = 1;
            } else {
                std::cout << "too many players already connected\n";
                continue;
            }
        }
        // This line only runs if a new player has been added
        connection_status[player_number] = 1;
        
        Event<EV_CONNECT> event(player_number, new_socket);
        eq->add_event(event.buffer_b); 

    }

    std::cout << "exited process_connections" << std::flush;
}
