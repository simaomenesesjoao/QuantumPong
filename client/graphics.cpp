#include <iostream>
#include <unistd.h>
#include <SDL2/SDL.h>

#include "../macros.hpp"
#include "shared_memory.hpp"
#include "graphics.hpp"

graphics::graphics(shared_memory *mem){
    memory = mem;


}
void graphics::init(unsigned WIDTH, unsigned HEIGHT){
    width = WIDTH;
    height = HEIGHT;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { printf("error initializing SDL: %s\n", SDL_GetError()); }

    win = SDL_CreateWindow("QP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    rend = SDL_CreateRenderer(win, -1, render_flags);
    tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);


    PIXEL_SIZE = 4;
    Nbytes = width*height*PIXEL_SIZE;
    buffer = new uint8_t[Nbytes]; // Internal SDL buffer 
    memory->SDL_buffer = new int[100]; // Internal SDL buffer 
    memory->buffer = new uint8_t[width*height]; 
    memory->buffer_length = width*height;

}

void graphics::finalize(){
    // Finalize SDL
    delete [] memory->buffer;
    delete [] memory->SDL_buffer;

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
};

void graphics::set_screen(unsigned red, unsigned green, unsigned blue){

    SDL_RenderClear(rend);
    int pitch;
    SDL_LockTexture(tex, NULL,  (void **)&buffer, &pitch);

    // Colormap
    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            //buffer[n+2] = m*40; // G
            buffer[PIXEL_SIZE*n+1] = red;
            buffer[PIXEL_SIZE*n+2] = green;
            buffer[PIXEL_SIZE*n+3] = blue;
        }
    }


    SDL_UnlockTexture(tex);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
}

void graphics::update(){

    //std::cout << "Updating graphics. 1\n" << std::flush;
    SDL_RenderClear(rend);
    //std::cout << "Updating graphics. 1.5\n" << std::flush;
    int pitch;
    // A função SDL_LocKTexture aloca a memoria interiormente!! não pode ser estática
    SDL_LockTexture(tex, NULL,  (void **)&buffer, &pitch);

    //std::cout << "Updating graphics. 2\n" << std::flush;
    //for(unsigned n=0; n<Nbytes_receive; n++){
        //uint8_t b = buffer_receive[n];
        //if((int)b != 0) {
            //std::cout << "nonzero found \n" << std::flush;
            //break;
        //}
    //}

    // Colormap
    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            buffer[PIXEL_SIZE*n+0] = 0; // R
            buffer[PIXEL_SIZE*n+1] = 0; // R
            buffer[PIXEL_SIZE*n+2] = memory->buffer[n]; // G
            buffer[PIXEL_SIZE*n+3] = 0; // B
        }
    }


    //std::cout << "Updating graphics. 3\n" << std::flush;
    SDL_UnlockTexture(tex);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
    //SDL_Delay(1000 / 1);
    //std::cout << "Updating graphics 4\n. " << std::flush;
}

bool graphics::get_one_SDL_event(){

    SDL_Event event;

    bool has_relevant_event = false;
    memory->SDL_buffer[0] = -1; // default event

    while(SDL_PollEvent(&event)){;

        //std::cout << "SLD event found " << event.type << " \n" << std::flush;
        switch (event.type) {

            case SDL_QUIT:
                std::cout << "event was quit\n";
                memory->SDL_buffer[0] = EV_SDL_QUIT;
                //has_relevant_event = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
                memory->SDL_buffer[0] = 1;
                memory->SDL_buffer[1] = event.button.x;
                memory->SDL_buffer[2] = event.button.y;
                //has_relevant_event = true;

                break;

            case SDL_KEYDOWN:
                std::cout << "Key " << (char)event.key.keysym.sym << " " << ((event.key.state == SDL_PRESSED) ? "pressed" : "released") << "\n";

                switch(event.key.keysym.sym){
                    case SDLK_SPACE:
                        std::cout << " pressed space\n";
                        memory->SDL_buffer[0] = EV_PRESSED_SPACE;
                        break;

                    default:
                        break;
                }

                break;

            default:
                break;
        }

        // Check if relevant event has been found
        has_relevant_event = (memory->SDL_buffer[0] != -1);
        if(has_relevant_event) break;
    }
    return has_relevant_event;
}
