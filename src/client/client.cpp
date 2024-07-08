#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../macros.hpp"
#include "../event_queue.hpp"
#include "client.hpp"


client::client(){}

void client::initConnection(std::string ip, unsigned port){
    socket_has_data = false;
    close = false;
    connected = false;
    winner = -1;

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


    sem_init(&semaphore1, 0, 0);
    sem_init(&semaphore2, 0, 0);
}

int client::connect_to_server(){
    std::cout << "connecting to server \n" << std::flush;

    // Connect to server
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        std::cout << "connection failed\n" << std::flush;
        return -1;
    }
    connected = true;

    std::cout << "connected \n" << std::flush;
    return 0;
}

void client::init_buffers(unsigned N){
    std::cout << "Initing buffers\n" << std::flush;
    // Nbytes_receive = memory->buffer_length;
    Nbytes_header = HEADER_LEN;
    Nbytes_send = HEADER_LEN;
    Nbytes_receive = N;

    buffer_header = new uint8_t[Nbytes_header]; 
    buffer_send = new uint8_t[Nbytes_send];
    buffer_receive = new uint8_t[Nbytes_receive];

    for(unsigned i=0; i<Nbytes_send; i++){
        buffer_send[i] = 0;
    }
}

void client::finalize(){
    delete [] buffer_send;
    delete [] buffer_header;
    delete [] buffer_receive;
}

void client::send_event_to_server(uint8_t *data, unsigned len_data){
    send(sock, data, len_data*sizeof(uint8_t), MSG_NOSIGNAL);
}

bool client::read_one_from_server(){
    std::cout << "Entered read one from server in socket " << sock << "\n";

    while(true){
        // usleep(100*1000);
        //std::cout << "loop read one\n";
        if(close) {
            std::cout << "Thread detected SDL closure\n";
            break;
        }

        if(!connected){
            usleep(100*1000);
            continue;
        }
        // std::cout << "loop read one connected\n";

        // Set up the fd_set for select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 1;  // Timeout after 1 second
        timeout.tv_usec = 0;


        int activity = select(sock + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity < 0) {
            std::cout << "activity<0\n" << std::endl;
            //perror("select");
            //close(sockfd);
            return 1;
        } else if (activity == 0) {
            // std::cout << "Timeout occurred, no data within 1 second." << std::endl;
        } else {

            // std::cout << "activity found on socket" << sock << " \n";
            // dont think I need to check the socket if there was activity

            int bytesReceived = recv(sock, buffer_header, Nbytes_header*sizeof(uint8_t), MSG_WAITALL);

            Event<EV_GENERIC> event(buffer_header);
            int event_id = event.event_ID;
            int payload_size = event.payload_size;
            
            // std::cout << "redceived: " << bytesReceived << " from server event " << event_id << " payload:" << payload_size << "\n" << std::flush;
            if(event_id != EV_STREAM){
                std::cout << "received: " << bytesReceived << " from server event " << event_id << " payload:" << payload_size << "\n" << std::flush;
                
           
                for(unsigned i=0; i<10; i++){
                    std::cout << (int)buffer_header[i] << " ";
                }
                std::cout << "\n";
            }
            
            if(bytesReceived <= 0){
                close = true;
                break;
            }
            
            if(payload_size > 0){

                // std::cout << "has payload\n";
                bytesReceived = recv(sock, buffer_receive, payload_size*sizeof(uint8_t),MSG_WAITALL);
                // if(payload_size < 300*500){
                //     std::cout << "Small payload received:\n";
                //     for(int i=0; i<payload_size; i++){
                //         std::cout << (int)buffer_receive[i] << " ";
                //     }
                //     std::cout << "finished receiving\n";
                    
                
                // }
                // std::cout << "payload! received: " << bytesReceived << " from server event " << event << " payload:" << has_payload << "\n" << std::flush;

                if(bytesReceived <= 0){
                    close = true;
                    break;
                }
            }

            // std::cout << "before semaphore\n";

            
            // If the game is closed, then the semaphore will no longer be updated, 
            // so I need to return here
            if(close) return 0;
            socket_has_data = true;
            sem_wait(&semaphore1);
            // std::cout << "after semaphore\n";
        }
    }

    return 1;
}




