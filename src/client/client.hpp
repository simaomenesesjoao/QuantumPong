#ifndef CLIENT_H
#define CLIENT_H 1

#include <cstdint>
#include <string>
#include <netinet/in.h>
#include "shared_memory.hpp"
#include <semaphore.h>

class client {
    public:
        int sock;
        bool close, socket_has_data, connected;
        int player_number;
        struct sockaddr_in serv_addr;
        

        uint8_t *buffer_receive;
        uint8_t *buffer_send;
        uint8_t *buffer_header;
        std::string ip;
        unsigned Nbytes_receive, Nbytes_send, Nbytes_header;
        shared_memory *memory;
        fd_set rfd;
        struct timeval timeout;
        sem_t semaphore1, semaphore2;


    client(shared_memory *, std::string, unsigned);
    int connect_to_server();
    int winner;
    void init_buffers(unsigned);
    void finalize();
    void send_event_to_server(uint8_t *, unsigned);
    bool read_one_from_server();

};

#endif