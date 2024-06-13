#include <SDL2/SDL.h>
#include "shared_memory.hpp"

class graphics {

    public:
        SDL_Window* win;
        SDL_Renderer* rend;
        SDL_Texture* tex;

        unsigned width, height;
        unsigned Nbytes;

        uint8_t *buffer_pixels;
        int *buffer_potential;
        int *buffer_wavefunction;
        int *buffer_SDL;

        int x0, y0, x1, y1;

        unsigned PIXEL_SIZE;
        shared_memory *memory;

    graphics(shared_memory*);
    void init(unsigned, unsigned);
    void finalize();
    
    bool get_one_SDL_event();

    void draw_wavefunction();
    void draw_potential();
    void update_potential(int, int, int, int, uint8_t*);
    void update_wavefunction(uint8_t*);
    void draw_paddle(int, int, int, int, int);
    void update();

    void set_screen(unsigned, unsigned, unsigned);

};
