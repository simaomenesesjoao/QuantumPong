#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H 1

#include "../event_queue.hpp"
#include <netinet/in.h>

class connection_handler {
    public:

        bool connection_status[2];
        int number_of_players;
        event_queue *eq;

        int server_fd, new_socket;
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);
        bool accepting_connections;


    void init(unsigned port);
    void process_connections();
    connection_handler(event_queue*);
};

#endif 