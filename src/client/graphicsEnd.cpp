#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <iostream>
#include <SDL2/SDL.h>
#include "graphics.hpp"


void GraphicsEnd::CreateTextureFromString(std::string textureText, SDL_Color textColor, SDL_Texture **texture, SDL_Surface **surface){
    // std::cout << "before surface \n" << std::flush;  
    *surface = TTF_RenderText_Blended( gFont, textureText.c_str(), textColor );
    if(*surface == NULL) 
        std::cout << "Problem generating surface\n";

    // std::cout << "in the middle\n" << std::flush;  
    *texture = SDL_CreateTextureFromSurface( rend, *surface);
    if(*texture == NULL) 
        std::cout << "Problem generating texture\n";
    // std::cout << "after texture\n" << std::flush;  
}

GraphicsEnd::GraphicsEnd(SDLAux* aux, int width, int height, int player_number):
    aux(aux),rend(aux->renderer),width(width),height(height),player_number(player_number){
    std::cout << "GraphicsEnd constructor entered\n" << std::flush;


    //Initialize SDL_ttf and font-related things
    if( TTF_Init() == -1 ){
        printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
    }


    gFont = TTF_OpenFont( "Arial.TTF", 28 );
    // Good place to throw an
    if( gFont == NULL ) printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
    SDL_Color textColor = {255, 255, 255}; 
    CreateTextureFromString("Victory", textColor, &victoryTextTexture, &victoryTextSurface);
    CreateTextureFromString("Defeat", textColor, &defeatTextTexture, &defeatTextSurface);

    // Create the victory and defeat screens. Background and text are done separately
    SDL_Surface* surface;
    surface = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 79, 22));
    victoryTexture = SDL_CreateTextureFromSurface(rend, surface);
    
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 105, 24, 0));
    defeatTexture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_FreeSurface(surface);
    std::cout << "GraphicsEnd constructor left\n" << std::flush;
    
    
}

GraphicsEnd::~GraphicsEnd(){
    TTF_CloseFont(gFont);
    TTF_Quit();
    SDL_DestroyTexture(victoryTexture);
    SDL_DestroyTexture(defeatTexture);

    SDL_DestroyTexture(victoryTextTexture);
    SDL_DestroyTexture(defeatTextTexture);
    SDL_DestroyTexture(victoryTexture);
    SDL_DestroyTexture(defeatTexture);

    SDL_FreeSurface(victoryTextSurface);
    SDL_FreeSurface(defeatTextSurface);
}

void GraphicsEnd::endScreen(bool won){
    std::cout << "graphics::endScreen\n" << std::flush;
    SDL_Rect srcrect, dstrect;

    SDL_RenderClear(rend);
    if(won){
        SDL_RenderCopy( rend, victoryTexture, NULL, NULL);

        srcrect.x = 0;
        srcrect.y = 0;
        srcrect.w = victoryTextSurface->w;
        srcrect.h = victoryTextSurface->h;

        dstrect.x = 10;
        dstrect.y = 10;
        dstrect.w = victoryTextSurface->w;
        dstrect.h = victoryTextSurface->h;
        SDL_RenderCopy( rend, victoryTextTexture, &srcrect, &dstrect);


    } else {
        SDL_RenderCopy( rend, defeatTexture, NULL, NULL);
        
        srcrect.x = 0;
        srcrect.y = 0;
        srcrect.w = defeatTextSurface->w;
        srcrect.h = defeatTextSurface->h;

        dstrect.x = 10;
        dstrect.y = 10;
        dstrect.w = defeatTextSurface->w;
        dstrect.h = defeatTextSurface->h;
        SDL_RenderCopy( rend, defeatTextTexture, &srcrect, &dstrect);    
    }

    SDL_RenderPresent(rend);


}