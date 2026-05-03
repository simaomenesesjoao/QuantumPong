#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H 1

#include <netinet/in.h>
#include <mutex>
#include <cstdint>
#include <cstddef>

class simulator;

class connection_handler {
public:
    connection_handler() = default;

    void init(unsigned port);
    void accept_and_listen(simulator *sim); // blocks; call in its own thread
    void send(const uint8_t *data, size_t len);

private:
    int server_fd      = -1;
    int gateway_socket = -1;
    struct sockaddr_in address {};
    int addrlen = sizeof(address);
    int opt     = 1;
    std::mutex send_mutex;
};

#endif // CONNECTION_HANDLER_H
