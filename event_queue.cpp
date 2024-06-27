#include <unistd.h>
#include <semaphore.h>
#include <mutex>
#include <iostream>
#include <cstdint>
#include "event_queue.hpp"


event_queue::event_queue(unsigned size){
    if(VERBOSE > 0){ std::cout << "Initiating event_queue with size " << size << "\n"; }

    events = new uint8_t*[size];
    for(unsigned i=0; i<size; i++){
        events[i] = new uint8_t[HEADER_LEN];
    }

    queue_size = size;
    head = 0;
    tail = 0;

};


event_queue::~event_queue(){

    for(unsigned i=0; i<queue_size; i++){
        delete[] events[i];
    }
    delete [] events;
};

int event_queue::add_event(uint8_t *data){
    if(VERBOSE>0){std::cout << "Attempting to add from bytes\n";}

    mutex.lock();

    unsigned new_head = (head+1)%queue_size;
    if(new_head == tail){
        std::cout << "Adding event failed. Queue full\n";
        return 1;
    }

    for(unsigned i=0; i<HEADER_LEN; i++){
        events[head][i] = data[i];
        //std::cout << (int)events[head][i] << " ";
    }
    //std::cout << "\n";
    
    head = new_head;
    mutex.unlock();
    return 0;
}


// int event_queue::add_event(int ev, int *data){
//     if(VERBOSE>0){
//         std::cout << "Attempting to add event " << ev << " with data ";
//         for(unsigned i=0; i<5; i++) std::cout << data[i] << " ";
//         std::cout << "\n";
//     }

//     mutex.lock();

//     unsigned new_head = (head+1)%queue_size;
//     if(new_head == tail){
//         std::cout << "Adding event failed. Queue full\n";
//         return 1;
//     }

//     events[head][0] = ev;
//     for(unsigned i=0; i<HEADER_LEN-1; i++){
//         events[head][i+1] = data[i];
//     }

//     head = new_head;
//     mutex.unlock();
//     return 0;
// }

// int event_queue::add_event(int ev){
//     if(VERBOSE>0){std::cout << "Attempting to add event " << ev << " with no data\n";}

//     mutex.lock();

//     unsigned new_head = (head+1)%queue_size;
//     if(new_head == tail){
//         std::cout << "Adding event failed. Queue full\n";
//         return 1;
//     }

//     events[head][0] = ev;
//     head = new_head;
//     mutex.unlock();
//     return 0;
// }


// int event_queue::add_event(int ev, int data){
//     if(VERBOSE>0){std::cout << "Attempting to add event " << ev << " with data " << data << "\n";}

//     mutex.lock();

//     unsigned new_head = (head+1)%queue_size;
//     if(new_head == tail){
//         std::cout << "Adding event failed. Queue full\n";
//         return 1;
//     }

//     events[head][0] = ev;
//     events[head][1] = data;
//     head = new_head;
//     mutex.unlock();
//     return 0;
// }


// int event_queue::add_event(int ev, int data1, int data2){
//     if(VERBOSE>0){std::cout << "Attempting to add event " << ev << " with 2 data " << data1 << " " << data2 << "\n";}

//     mutex.lock();

//     unsigned new_head = (head+1)%queue_size;
//     if(new_head == tail){
//         std::cout << "Adding event failed. Queue full\n";
//         return 1;
//     }

//     events[head][0] = ev;
//     events[head][1] = data1;
//     events[head][2] = data2;
//     head = new_head;
//     mutex.unlock();
//     return 0;
// }

int event_queue::print_queue_data(){
    std::cout << "Printing queue data\n";
    for(unsigned i=0; i<head+8; i++){
        std::cout << " ";
    }
    std::cout << "h\n";

    std::cout << "events: ";
    for(unsigned i=0; i<queue_size; i++){
        std::cout << events[i] << " ";
    }
    std::cout << "\n";

    std::cout << "data:   ";
    for(unsigned i=0; i<queue_size; i++){
        std::cout << events[i][0] << " ";
    }
    std::cout << "\n";

    for(unsigned i=0; i<tail+8; i++){
        std::cout << " ";
    }
    std::cout << "t\n";
    return 0;
}


void event_queue::read(int *event, uint8_t **data){
    // if(VERBOSE>0){std::cout << "Reading events\n" << std::flush;}
    mutex.lock();
    if(tail == head){
        *event = -1;
        // std::cout << "No event found\n" << std::flush;
        
    } else {
        *data = events[tail];
        *event = (int)events[tail][0];
        tail = (tail+1)%queue_size;
        std::cout << "Found event" << *event << "\n" << std::flush;
    }
    mutex.unlock();
}
