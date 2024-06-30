#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <string>
#include <SDL2/SDL_ttf.h>
#include "shared_memory.hpp"

class graphics {

    public:
        SDL_Window* win;
        SDL_Renderer* rend;
        TTF_Font* gFont;


        SDL_Texture *wavefunctionTexture, *potTexture;



        SDL_Texture *DisconnectedStatusTexture, *ConnectedStatusTexture, *inEndStatusTexture, *WantNewStatusTexture;
        SDL_Surface *DisconnectedStatusSurface, *ConnectedStatusSurface, *inEndStatusSurface, *WantNewStatusSurface;

        SDL_Texture *Player1Texture, *Player2Texture, *placeholderTexture;
        SDL_Surface *Player1Surface, *Player2Surface, *placeholderSurface;

        SDL_Texture *paddle1Texture, *paddle2Texture;
        SDL_Texture *victoryTexture, *defeatTexture;

        SDL_Texture *whiteMaskTexture;

        unsigned width, height;
        unsigned Nbytes;
        int alpha = 255;

        uint8_t *buffer_pixels_wavefunction, *buffer_pixels_potential;
        int *buffer_potential;
        int *buffer_wavefunction;
        int *buffer_SDL;

        int x0, y0, x1, y1;

        unsigned PIXEL_SIZE;
        shared_memory *memory;

    graphics(shared_memory*);
    void init(unsigned, unsigned);
    void finalize();
    void CreateTextureFromString(std::string , SDL_Color , SDL_Texture **, SDL_Surface **);
    
    bool get_one_SDL_event();

    void draw_wavefunction();
    void draw_potential();
    void update_potential(int, int, int, int, uint8_t*);
    void update_wavefunction(uint8_t*);
    void draw_paddle(int, int, SDL_Texture*);
    void update();

    // void set_screen(unsigned, unsigned, unsigned);
    void update_lobby(int, int);
    void endScreen(bool);

};
