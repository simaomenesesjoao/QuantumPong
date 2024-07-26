#ifndef GRAPHICS_H
#define GRAPHICS_H 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <string>
#include <SDL2/SDL_ttf.h>
#include "../event_queue.hpp"
#include <iostream>
#include <thread>



class SDLAux{
public:
    SDL_Renderer* renderer;
    SDL_Window* win;
    int score_width, window_height, window_width;

    SDLAux(int width, int height){
        std::cout << "-------- SDLAux constructor called " << width << " " << height << " \n" << std::flush;

        // Initialize window here

        // Initialize SDL
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
            printf("error initializing SDL: %s\n", SDL_GetError()); 
        }

        
        score_width = 20;
        window_height = height;
        window_width = width + score_width;
        win = SDL_CreateWindow("Quantum Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);

        if (win == nullptr) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        }

        Uint32 render_flags = SDL_RENDERER_ACCELERATED;
        renderer = SDL_CreateRenderer(win, -1, render_flags);

        std::cout << "-------- SDLAux constructor left\n" << std::flush;
    }

};

class GraphicsLobby{
    private:
        SDLAux *aux;
        int width, height, player_number;
        SDL_Renderer* rend; // Managed by the Graphics class
        TTF_Font* gFont;

        // Text textures
        SDL_Texture *DisconnectedStatusTexture, *ConnectedStatusTexture, *inEndStatusTexture, *WantNewStatusTexture;
        SDL_Surface *DisconnectedStatusSurface, *ConnectedStatusSurface, *inEndStatusSurface, *WantNewStatusSurface;
        SDL_Texture *Player1Texture, *Player2Texture, *placeholderTexture;
        SDL_Surface *Player1Surface, *Player2Surface, *placeholderSurface;

        void CreateTextureFromString(std::string textureText, SDL_Color textColor, SDL_Texture **texture, SDL_Surface **surface);


    public:

        GraphicsLobby(SDLAux* aux, int width, int height, int player_number);
        ~GraphicsLobby();

        void update_lobby(int p1Status, int p2Status);
};


class GraphicsGame{
    SDLAux *aux;
    SDL_Renderer* rend; // Managed by the Graphics class

    SDL_Texture *wavefunctionTexture, *potTexture, *magTexture;

    SDL_Texture *paddle1Texture, *paddle2Texture;

    SDL_Texture *scoreBarTexture;

    SDL_Texture *whiteMaskTexture;
    
    int width, height, player_number;
    int pixel_size;
    
    
    public:

        // Score related
        int score_width;
        float score_top, score_bot;

        // Paddle related
        int x0,y0,x1,y1;


        GraphicsGame(SDLAux* aux, int width, int height, int   player_number); 
        ~GraphicsGame();

        
        void update_score();
        void draw_paddle(int x, int y, SDL_Texture* texture);
        void update_wavefunction(buffer*);
        void update_potential(int x, int y, int dx, int dy, buffer*);
        void update_magnetic(int x, int y, int dx, int dy, buffer*);
        void update_white(int alpha);
        void update();
        void reset();

};







class GraphicsEnd{
private:
    SDLAux *aux;
    SDL_Renderer* rend; // Managed by the Graphics class
    SDL_Texture *defeatTexture, *victoryTexture;

    SDL_Texture *victoryTextTexture, *defeatTextTexture;
    SDL_Surface *victoryTextSurface, *defeatTextSurface;

    TTF_Font* gFont;
    
    int width, height, player_number;

    void CreateTextureFromString(std::string textureText, SDL_Color textColor, SDL_Texture **texture, SDL_Surface **surface);
    
public:

    GraphicsEnd(SDLAux* aux, int width, int height, int player_number);
    ~GraphicsEnd();
    void endScreen(bool won);

};



class Graphics{
    
private:
    void listen_SDL_events(event_queue*);

public:
    int SDL_listener_running;
    std::thread thread_listener;
   

    int width, height, player_number;
    int window_height, window_width;
    int score_width;

    event_queue *eventQueue;
    SDLAux aux;

    SDL_Renderer* renderer;
    SDL_Window* win;

    GraphicsLobby lobby;
    GraphicsGame game;
    GraphicsEnd end;

    Graphics(int width, int height, int player_number, event_queue *eq);

    ~Graphics();
        
};

#endif