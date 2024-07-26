#include <unistd.h>
#include <semaphore.h>
#include <mutex>
#include <iostream>
#include <cstdint>
#include "event_queue.hpp"
// #include "macros.hpp"

event_queue::event_queue(unsigned size, unsigned width, int verbose){
    // Allocates memory for the event queue, which has 'size' rows and 'width' cols
    // The data is encoded using as uint8_t (byte) to facilitate serialization while
    // communicating with the server. The codification is done using unions via the 
    // templated Event<> class defined in the header.
    // row0: 001 000 400 001 032 ... 000 000
    //       ^-------------^
    //       encodes event id (int)
    // The event queue is implemented as a ring, with a head and a tail
    
    event_queue_verbose = verbose;

    if(event_queue_verbose > 0){ std::cout << "event_queue: Initiating event_queue with size " << size << "\n" << std::flush; }

    events = new uint8_t*[size];
    for(unsigned i=0; i<size; i++){
        events[i] = new uint8_t[width];

        for(unsigned j=0; j<width; j++){
            events[i][j] = 0;
        }
    }

    queue_width = width;
    queue_size = size;
    head = 0;
    tail = 0;
}


event_queue::~event_queue(){

    if(event_queue_verbose > 0){ std::cout << "event_queue: destructor\n" << std::flush; }

    for(unsigned i=0; i<queue_size; i++){
        delete[] events[i];
    }
    delete [] events;
}

void event_queue::add_event(uint8_t *data){
    // Adds a new event to the event queue, using *data as the source of data

    if(event_queue_verbose>1){std::cout << "event_queue: Attempting to add event " << (int)data[0] << " \n" << std::flush;}
    if(event_queue_verbose>2){
        std::cout << "event_queue: event data: ";
        for(unsigned i=0; i<queue_width; i++){
            std::cout << (int)data[i] << " ";
        }
        std::cout << "\n" << std::flush;
    }



    mutex.lock();

    unsigned new_head = (head+1)%queue_size;

    // If the new head is the same as the tail, it means the queue is full
    if(new_head == tail){
        // CHANGE: good place to add an exception handler. Careful with mutex
        std::cout << "event_queue: Adding event failed. Queue full.\n";
        
    } else {
        for(unsigned i=0; i<queue_width; i++){
            events[head][i] = data[i];
        }
    }
    
    head = new_head;
    mutex.unlock();
}


void event_queue::pop_oldest(int *event, uint8_t *data){
    // Reads the oldest event from the event queue

    if(event_queue_verbose>1){std::cout << "event_queue " << event_queue_verbose << ": Reading events\n" << std::flush;}
    mutex.lock();

    if(tail == head){
        *event = -1;        
    } else {

        for(unsigned i=0; i<queue_width; i++){
            data[i] = events[tail][i];
        }
        *event = (int)events[tail][0];
        tail = (tail+1)%queue_size;
        if(event_queue_verbose>0){ std::cout << "event_queue: Found event " << *event << "\n" << std::flush;}
    }

    mutex.unlock();
}









buffer::buffer(unsigned size, unsigned width, int buffer_verbose){
    // Allocates memory for the buffer, which has 'size' rows and 'width' cols
    // It is implemented as a ring, with a head and a tail

    if(buffer_verbose > 0){ std::cout << "buffer: Initiating buffer with size " << size << "\n" << std::flush; }

    

    queue_width = width;
    queue_size = size;
    locked = false;
    head = 1; // location that will be written
    tail = 0; // location that will be read
   

    data = new uint8_t*[queue_size];
    for(unsigned i=0; i<queue_size; i++){
        data[i] = new uint8_t[queue_width];

        for(unsigned j=0; j<queue_width; j++){
            data[i][j] = 0;
        }
    }

    write_data = data[head]; // pointer to the data location that will be written
    read_data = data[tail];

    std::cout << "----- current data:" << (int)write_data[0] << " " << (int)write_data[1] << "\n";
}

void buffer::reset(){

    for(unsigned i=0; i<queue_size; i++){
        for(unsigned j=0; j<queue_width; j++){
            data[i][j] = 0;
        }
    }
    head = 1; // location that will be written
    tail = 0; // location that will be read

}

buffer::~buffer(){

    if(buffer_verbose > 0){ std::cout << "buffer: destructor\n" << std::flush; }

    for(unsigned i=0; i<queue_size; i++){
        delete[] data[i];
    }
    delete [] data;
}

void buffer::lock(){
    mutex_lock.lock();
    locked = true;
    mutex_lock.unlock();
}



void buffer::unlock(){

    mutex_lock.lock();
    locked = false;
    mutex_lock.unlock();

    mutex.lock();
    head = (head+1)%queue_size;
    write_data = data[head];
    mutex.unlock();

}

// void buffer::peek(uint8_t *data1){
//     // Reads the oldest event from the buffer and tries to increment the tail 
//     if(buffer_verbose>0){std::cout << "buffer: peek\n" << std::flush;}
    
//     for(unsigned i=0; i<queue_width; i++){
//         data1[i] = data[tail][i];
//     }

//     // Try to increment the tail by one. If the new tail location is the head, 
//     // that means that slot can be written at any point and is not thread safe
//     // in this case, the tail cannot be incremented.
//     mutex.lock();
//     unsigned new_tail = (tail+1)%queue_size;
//     if(new_tail != head)
//         tail = new_tail;
//     read_data = data[tail];
//     mutex.unlock();
// }


void buffer::update_tail(){
    
    if(buffer_verbose>0){std::cout << "buffer::update_tail peek\n" << std::flush;}
    
    // Try to update the tail. If the new tail location is the head, 
    // that means that that slot can be written at any point and is not thread safe
    // in this case, the tail cannot be incremented.
    mutex.lock();
    // unsigned new_tail = tail;
    // for(int i=0; i<queue_size; i++){
    //     new_tail = (new_tail+1)%queue_size;
    //     if(new_tail != head && tail != head)
    //         tail = new_tail;
    // }

    // always be one behind the slot being written
    tail = (head-1+queue_size)%queue_size;
    read_data = data[tail];
    std::cout << "buffer::update_tail tail: " << tail << "\n" << std::flush;
    
    mutex.unlock();
}




void buffer::update_tail_by1(){
    if(buffer_verbose>0){std::cout << "buffer::update_tail_by1\n" << std::flush;}
    
    unsigned new_tail = (tail+1)%queue_size;
    mutex.lock();
    if(new_tail != head){
        tail = new_tail;
        read_data = data[tail];
    }
    mutex.unlock();
    std::cout << "buffer::update_tail_by1 tail: " << tail << "\n" << std::flush;
}
