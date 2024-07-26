#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <iostream>
#include <SDL2/SDL.h>
#include "../macros.hpp"
#include "graphics.hpp"
#include <sstream>

GraphicsLobby::GraphicsLobby(SDLAux* aux, int width, int height, int player_number):
    aux(aux),width(width), height(height), player_number(player_number),rend(aux->renderer){
    if( TTF_Init() == -1 ){
        printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
    }

    gFont = TTF_OpenFont( "Arial.TTF", 28 );
    if( gFont == NULL ) printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );


    // Render the text
    SDL_Color textColor = { 255, 255, 255}; 
    CreateTextureFromString("Disconnected", textColor, &DisconnectedStatusTexture, &DisconnectedStatusSurface);
    CreateTextureFromString("Connected", textColor, &ConnectedStatusTexture, &ConnectedStatusSurface);
    CreateTextureFromString("In end screen", textColor, &inEndStatusTexture, &inEndStatusSurface);
    CreateTextureFromString("Wants new game", textColor, &WantNewStatusTexture, &WantNewStatusSurface);
    


    SDL_Color textColor1, textColor2;
    textColor1 = { P1R, P1G, P1B}; 
    textColor2 = { P2R, P2G, P2B}; 
    std::ostringstream stringStream1, stringStream2;
    std::string p1, p2;

    switch(player_number){
        case 0:
            stringStream1 << "Player 1 (You, bot):";
            p1 = stringStream1.str();

            stringStream2 << "Player 2 (top):";
            p2 = stringStream2.str();
            break;
        case 1:

            stringStream1 << "Player 1 (bot):";
            p1 = stringStream1.str();

            stringStream2 << "Player 2 (You, top):";
            p2 = stringStream2.str();
            break;

        default:
            std::cout << "Player number was not assigned. value: " << player_number << "\n" << std::flush;
            break;
    }

    
    
    CreateTextureFromString(p1, textColor1, &Player1Texture, &Player1Surface);
    CreateTextureFromString(p2, textColor2, &Player2Texture, &Player2Surface);
    
}


GraphicsLobby::~GraphicsLobby(){

    TTF_CloseFont(gFont);
    gFont = NULL;
    TTF_Quit();

    SDL_DestroyTexture(DisconnectedStatusTexture);
    SDL_DestroyTexture(ConnectedStatusTexture);
    SDL_DestroyTexture(inEndStatusTexture);
    SDL_DestroyTexture(WantNewStatusTexture);
    SDL_DestroyTexture(Player1Texture);
    SDL_DestroyTexture(Player2Texture);

    SDL_FreeSurface(DisconnectedStatusSurface);
    SDL_FreeSurface(ConnectedStatusSurface);
    SDL_FreeSurface(inEndStatusSurface);
    SDL_FreeSurface(WantNewStatusSurface);
    SDL_FreeSurface(Player1Surface);
    SDL_FreeSurface(Player2Surface);
    // placeholder texture and surface are just pointers to others, 
    // they don't need deallocation
}


void GraphicsLobby::CreateTextureFromString(std::string textureText, SDL_Color textColor, SDL_Texture **texture, SDL_Surface **surface){
    *surface = TTF_RenderText_Blended( gFont, textureText.c_str(), textColor );
    if(*surface == NULL) 
        std::cout << "Problem generating surface\n";

    *texture = SDL_CreateTextureFromSurface( rend, *surface);
    if(*texture == NULL) 
        std::cout << "Problem generating texture\n";
    
}






void GraphicsLobby::update_lobby(int p1Status, int p2Status){
    std::cout << "graphics::set_screen\n" << std::flush;


    int y = 350;
    int x = 20;
    int fontHSpacing = 30;

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = Player1Surface->w;
    rect.h = Player1Surface->h;


    SDL_RenderClear(rend);

    // Draw words
    rect.w = Player1Surface->w;
    SDL_RenderCopy( rend, Player1Texture, NULL, &rect);
    
    rect.w = Player2Surface->w;
    rect.y = y + fontHSpacing*3;
    SDL_RenderCopy( rend, Player2Texture, NULL, &rect);


    // Process player 1
    switch(p1Status){
        case 1: 
            placeholderSurface = ConnectedStatusSurface;
            placeholderTexture = ConnectedStatusTexture;
            break;

        case 2: 
            placeholderSurface = WantNewStatusSurface;
            placeholderTexture = WantNewStatusTexture;
            break;

        case 6: 
            placeholderSurface = inEndStatusSurface;
            placeholderTexture = inEndStatusTexture;
            break;

        default: 
            placeholderSurface = DisconnectedStatusSurface;
            placeholderTexture = DisconnectedStatusTexture;
            break;

    }

    rect.y = y + fontHSpacing;
    rect.w = placeholderSurface->w;
    rect.h = placeholderSurface->h;
    SDL_RenderCopy( rend, placeholderTexture, NULL, &rect);




    // Process player 2
    switch(p2Status){
        case 1: 
            placeholderSurface = ConnectedStatusSurface;
            placeholderTexture = ConnectedStatusTexture;
            break;

        case 2: 
            placeholderSurface = WantNewStatusSurface;
            placeholderTexture = WantNewStatusTexture;
            break;

        case 6: 
            placeholderSurface = inEndStatusSurface;
            placeholderTexture = inEndStatusTexture;
            break;

        default: 
            placeholderSurface = DisconnectedStatusSurface;
            placeholderTexture = DisconnectedStatusTexture;
            break;

    }

    rect.y = y + 4*fontHSpacing;
    rect.w = placeholderSurface->w;
    rect.h = placeholderSurface->h;
    SDL_RenderCopy( rend, placeholderTexture, NULL, &rect);


    SDL_RenderPresent(rend);

}