#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H 1

#include <cstdint>

class shared_memory{
    public:
        // uint8_t *header;
        // unsigned header_length;

        // uint8_t *buffer;
        // unsigned buffer_length;

        // int *SDL_buffer;
        // unsigned SDL_buffer_length;

        int server_status;

        // int x0,y0,x1,y1;

    shared_memory(){
        // CHANGE: will cause segfault if they are too close to the border and
        // the paddle positions aren't updated. solution: set them to half the
        // geometry during initialization
        // x0 = 50;
        // y0 = 50;
        // x1 = 50;
        // y1 = 50;
    };
};

#endif
