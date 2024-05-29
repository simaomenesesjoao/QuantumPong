#include <iostream>
#include <semaphore.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../macros.hpp"
#include "shared_memory.hpp"
#include "client.hpp"

client::client(shared_memory *memory_pt, std::string ip, unsigned port){
    memory = memory_pt;
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

    //FD_ZERO(&rfd);
    //FD_SET(sock, &rfd);

    //timeout.tv_sec = 0;
    //timeout.tv_usec = 100;
    sem_init(&semaphore1, 0, 0);
    sem_init(&semaphore2, 0, 0);
}

int client::connect_to_server(){
    std::cout << "connecting to server \n" << std::flush;

    // Connect to server
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        return -1;
    }
    connected = true;

    std::cout << "connected \n" << std::flush;
    return 0;
}

void client::init_buffers(){
    std::cout << "Initing buffers\n" << std::flush;
    Nbytes_receive = memory->buffer_length;
    Nbytes_header = memory->header_length = HEADER_LEN;
    Nbytes_send = HEADER_LEN;

    buffer_receive = memory->buffer; // Initialized by SDL interface
    memory->header = new uint8_t[Nbytes_header];
    buffer_header = memory->header; 
    buffer_send = new uint8_t[Nbytes_send];

    for(unsigned i=0; i<Nbytes_send; i++){
        buffer_send[i] = 0;
    }
}

void client::finalize(){
    //delete [] buffer_send;
    delete [] memory->header;
}

void client::send_event_to_server(uint8_t *data, unsigned len_data){
    send(sock, data, len_data*sizeof(uint8_t), MSG_NOSIGNAL);
}

bool client::read_one_from_server(){
    std::cout << "Entered read one from server in socket " << sock << "\n";

    while(true){
        //std::cout << "loop read one\n";
        if(close) {
            std::cout << "Thread detected SDL closure\n";
            break;
        }

        if(!connected){
            usleep(1000*1000);
            continue;
        }
        std::cout << "loop read one connected\n";

        // Set up the fd_set for select
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 1;  // Timeout after 5 seconds
        timeout.tv_usec = 0;


        int activity = select(sock + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity < 0) {
            std::cout << "activity<0\n" << std::endl;
            //perror("select");
            //close(sockfd);
            return 1;
        } else if (activity == 0) {
            std::cout << "Timeout occurred, no data within 1 second." << std::endl;
        } else {

            std::cout << "activity found on socket" << sock << " \n";
            //if (FD_ISSET(sock, &readfds)) {
                //std::cout << "activity in correct socket\n";
            //}
            // dont think I need to check the socket if there was activity

            int bytesReceived = recv(sock, buffer_header, HEADER_LEN*sizeof(uint8_t), MSG_WAITALL);
            int has_payload = (int)buffer_header[0];
            int event = (int)buffer_header[1];
            std::cout << "received: " << bytesReceived << " from server event " << event << " payload:" << has_payload << "\n" << std::flush;
            if(bytesReceived <= 0){
                close = true;
                break;
            }
            

            if(has_payload){
                std::cout << "has payload\n";
                bytesReceived = recv(sock, buffer_receive, Nbytes_receive*sizeof(uint8_t),MSG_WAITALL);
                std::cout << "payload! received: " << bytesReceived << " from server event " << event << " payload:" << has_payload << "\n" << std::flush;

                if(bytesReceived <= 0){
                    close = true;
                    break;
                }
            }

            


            socket_has_data = true;
            sem_wait(&semaphore1);
        }
    }

    return 1;
}




