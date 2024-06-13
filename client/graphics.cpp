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

    x0 = width/2;
    x1 = width/2;
    y0 = height/2;
    y1 = height/2;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { printf("error initializing SDL: %s\n", SDL_GetError()); }

    win = SDL_CreateWindow("QP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    rend = SDL_CreateRenderer(win, -1, render_flags);
    tex = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);


    PIXEL_SIZE = 4;
    Nbytes = width*height*PIXEL_SIZE;

    buffer_pixels = new uint8_t[Nbytes];         // buffer with pixels to put on screen
    buffer_potential = new int[width*height];    // buffer with potential value
    buffer_wavefunction = new int[width*height]; // buffer with wavefunction value
    buffer_SDL = new int[HEADER_LEN];            // buffer with SDL events

    for(int i=0; i<width*height; i++){
        buffer_potential[i] = 0;
    }

}

void graphics::finalize(){
    // Finalize SDL
    // delete [] buffer_pixels; // automatically taken care of by SDL_LockTexture?
    delete [] buffer_SDL;
    delete [] buffer_potential;
    delete [] buffer_wavefunction;

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
};

void graphics::set_screen(unsigned red, unsigned green, unsigned blue){

    SDL_RenderClear(rend);
    int pitch;
    SDL_LockTexture(tex, NULL,  (void **)&buffer_pixels, &pitch);

    // Colormap
    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            //buffer[n+2] = m*40; // G
            buffer_pixels[PIXEL_SIZE*n+1] = red;
            buffer_pixels[PIXEL_SIZE*n+2] = green;
            buffer_pixels[PIXEL_SIZE*n+3] = blue;
        }
    }


    SDL_UnlockTexture(tex);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
}

void graphics::draw_wavefunction(){

    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            buffer_pixels[PIXEL_SIZE*n+0] = 0; // A
            buffer_pixels[PIXEL_SIZE*n+1] = 0; // R
            buffer_pixels[PIXEL_SIZE*n+2] = buffer_wavefunction[n]; // G
            buffer_pixels[PIXEL_SIZE*n+3] = 0; // B
        }
    }
}

void graphics::update_wavefunction(uint8_t *buffer1){
    std::cout << "graphics: entered update_wavefunction\n" << std::flush;

    for(int i=0; i<width*height; i++) buffer_wavefunction[i] = (int)buffer1[i];

    std::cout << "graphics: left update_wavefunction\n" << std::flush;
}

void graphics::update_potential(int x, int y, int dx, int dy, uint8_t *buffer1){

    std::cout << "graphics: entered update_potential\n" << std::flush;
    int n, m;

    // std::cout << "updating potential:\n";
    for(int i=0; i<dx; i++){
        for(int j=0; j<dy; j++){
            n = x+i + (y+j)*width;
            m = i + dx*j;
            // std::cout << "ijnm: " << i << " " << j << " " << n << " " << m << std::flush;
            buffer_potential[n] = (int)buffer1[m];
            // std::cout << "pot " << potential[n] << "\n" << std::flush;
            // std::cout << "buf " << buffer[m] << "\n" << std::flush;
        }
    }
    std::cout << "graphics: left update_potential\n" << std::flush;
}

void graphics::draw_potential(){
    std::cout << "graphics: entered draw_potential\n" << std::flush;

    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            // buffer_pixels[PIXEL_SIZE*n+0] = 0; // A
            // buffer_pixels[PIXEL_SIZE*n+1] += 0; // R
            // buffer_pixels[PIXEL_SIZE*n+2] += 0; // G
            buffer_pixels[PIXEL_SIZE*n+3] += buffer_potential[n]; // B
        }
    }
    std::cout << "graphics: left draw_potential\n" << std::flush;
}

void graphics::draw_paddle(int x, int y, int R, int G, int B){
    std::cout << "graphics: entered draw_paddle\n" << std::flush;

    int paddle_width = 100;
    int paddle_height = 20;
    int xmin = x - paddle_width/2;
    int xmax = x + paddle_width/2;

    int ymin = y - paddle_height/2;
    int ymax = y + paddle_height/2;


    for(int j = xmin; j < xmax; j++){
        for(int i = ymin; i < ymax; i++){
            unsigned n = (i*width + j);
            
            // ARGB
            buffer_pixels[PIXEL_SIZE*n+0]  = 0; // A
            buffer_pixels[PIXEL_SIZE*n+1] += R;
            buffer_pixels[PIXEL_SIZE*n+2] += G;
            buffer_pixels[PIXEL_SIZE*n+3] += B;
        }
    }
    std::cout << "graphics: left draw_paddle\n" << std::flush;
}


void graphics::update(){
    std::cout << "graphics: entered update\n" << std::flush;

    //std::cout << "Updating graphics. 1\n" << std::flush;
    SDL_RenderClear(rend);
    //std::cout << "Updating graphics. 1.5\n" << std::flush;
    int pitch;
    // A função SDL_LocKTexture aloca a memoria interiormente!! não pode ser estática
    SDL_LockTexture(tex, NULL,  (void **)&buffer_pixels, &pitch);

    draw_wavefunction();
    draw_potential();
    // std::cout << "paddles: " << x0 << " " << y0 << " " << x1 << " " << y1 << "\n";
    draw_paddle(x0, y0, 50,50,0);
    draw_paddle(x1, y1, 0,50,50);


    //std::cout << "Updating graphics. 3\n" << std::flush;
    SDL_UnlockTexture(tex);
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);
    //SDL_Delay(1000 / 1);
    //std::cout << "Updating graphics 4\n. " << std::flush;

    std::cout << "graphics: left update\n" << std::flush;
}

bool graphics::get_one_SDL_event(){

    SDL_Event event;

    buffer_SDL[0] = -1; // default event
    bool has_relevant_event = false;
    while(SDL_PollEvent(&event)){;


        switch (event.type) {

            case SDL_QUIT:
                std::cout << "event was quit\n";
                buffer_SDL[0] = EV_SDL_QUIT;
                //has_relevant_event = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
                std::cout << "mouse down event: " << (int)event.button.button << "\n";

                //has_relevant_event = true;
                
                switch(event.button.button){
                    case SDL_BUTTON_LEFT:
                        buffer_SDL[0] = EV_MOUSEBUTTONDOWN;
                        buffer_SDL[1] = BUTTON_LEFT;
                        buffer_SDL[2] = event.button.x;
                        buffer_SDL[3] = event.button.y;
                        std::cout << "pressed left\n";
                        break;

                    case SDL_BUTTON_RIGHT:
                        buffer_SDL[0] = EV_MOUSEBUTTONDOWN;
                        buffer_SDL[1] = BUTTON_RIGHT;
                        buffer_SDL[2] = event.button.x;
                        buffer_SDL[3] = event.button.y;
                        std::cout << "pressed right\n";
                        break;

                    default:
                        break;
                }

                break;

            case SDL_KEYDOWN:
                std::cout << "Key " << (char)event.key.keysym.sym << " " << ((event.key.state == SDL_PRESSED) ? "pressed" : "released") << "\n";

                switch(event.key.keysym.sym){
                    case SDLK_SPACE:
                        std::cout << " pressed space\n";
                        buffer_SDL[0] = EV_PRESSED_SPACE;
                        break;

                    case SDLK_w:
                        std::cout << " pressed w\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_w;
                        break;

                    case SDLK_a:
                        std::cout << " pressed a\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_a;
                        break;

                    case SDLK_s:
                        std::cout << " pressed s\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_s;
                        break;

                    case SDLK_d:
                        std::cout << " pressed d\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_d;
                        break;

                    default:
                        break;
                }

                break;

            default:
                break;
        }

        // Check if relevant event has been found

        
        has_relevant_event = (buffer_SDL[0] != -1);
        if(has_relevant_event) break;
    }
    return has_relevant_event;
}
