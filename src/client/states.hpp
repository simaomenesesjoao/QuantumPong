#ifndef STATES_H
#define STATES_H 1

#include <cstdint>
#include "graphics.hpp"

#include <thread>
#include "../event_queue.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>


class frontEnd;

class State{
    public:
        frontEnd *context;
        State(frontEnd *contex):context(contex){
            std::cout << "Entered State constructor\n";
        };

        virtual void handle(uint8_t *data){};
        virtual ~State(){
            std::cout << "Entered State destructor\n";
        };

        void send_event_to_server(uint8_t *data, unsigned len_data, int sock){
            std::cout << "sending event to server " << (int)data[0] << " " << (int)data[1] << " in socket " << sock << "\n";
            send(sock, data, len_data*sizeof(uint8_t), MSG_NOSIGNAL);
        }

};



class StateDisconnected: public State{

    private:
        std::thread worker_thread;
        
        int sock;
        bool connector_running, listener_running;

    public:
        StateDisconnected(frontEnd *contex);

        ~StateDisconnected() override{
            std::cout << "Entered StateDisconnected destructor\n";
        };

        void handle(uint8_t *data) override;
        void connect_to_server(std::string, unsigned);
        void listen_server_events(int);

};




class StateConnected: public State{
    private:
        bool server_listener_running, sdl_listener_running;
        int player_number,width, height,sock;
        buffer buffer_wf, buffer_mag, buffer_pot;
        Graphics graphics;
        std::thread thread_SDL_listener, thread_server_listener;

    public:
        
        StateConnected(frontEnd *contex, int, int, int, int);

        ~StateConnected() override{
            std::cout << "Entered StateConnected destructor\n";
            server_listener_running = false;
            thread_server_listener.join();
        }
        void listen_server_events(int sock);
        void handle(uint8_t *data) override;

};







#endif // STATES_H