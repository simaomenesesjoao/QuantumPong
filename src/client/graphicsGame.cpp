#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
// #include <cstddef>
#include <SDL2/SDL_video.h>
#include <iostream>
#include <SDL2/SDL.h>
#include "../macros.hpp"
#include "graphics.hpp"


GraphicsGame::GraphicsGame(SDLAux* aux, int width, int height, int   player_number):
    aux(aux),rend(aux->renderer),width(width),height(height),player_number(player_number){
    std::cout << "-------- GraphicsGame constructor called\n" << std::flush;


    pixel_size = 4;

    // Score related
    score_top = 0;
    score_bot = 0;
    score_width = 20;
    scoreBarTexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, score_width, height);
    SDL_SetTextureBlendMode( scoreBarTexture, SDL_BLENDMODE_BLEND );

    // Wavefunction related
    wavefunctionTexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    potTexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    magTexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureBlendMode( wavefunctionTexture, SDL_BLENDMODE_BLEND );
    SDL_SetTextureBlendMode( potTexture, SDL_BLENDMODE_BLEND );
    SDL_SetTextureBlendMode( magTexture, SDL_BLENDMODE_BLEND );
    
    // White mask related
    SDL_Surface *surface;
    surface = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));
    whiteMaskTexture = SDL_CreateTextureFromSurface(rend, surface);
    SDL_SetTextureBlendMode( whiteMaskTexture, SDL_BLENDMODE_BLEND );
    SDL_SetTextureAlphaMod( whiteMaskTexture, 0);
    SDL_FreeSurface(surface);




    // Paddle related
    x0 = width/2;
    x1 = width/2;
    y0 = height/2;
    y1 = height/2;
    
    int paddle_width = 100;
    int paddle_height = 20;

    
    SDL_Surface *surface2;
    surface2 = SDL_CreateRGBSurface(0, paddle_width, paddle_height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(surface2, NULL, SDL_MapRGB(surface2->format, P1R, P1G, P1B));
    paddle1Texture = SDL_CreateTextureFromSurface(rend, surface2);
    

    SDL_FillRect(surface2, NULL, SDL_MapRGB(surface2->format, P2R, P2G, P2B));
    paddle2Texture = SDL_CreateTextureFromSurface(rend, surface2);
    SDL_FreeSurface(surface2);

    SDL_SetTextureBlendMode( paddle1Texture, SDL_BLENDMODE_BLEND );
    SDL_SetTextureBlendMode( paddle2Texture, SDL_BLENDMODE_BLEND );

    SDL_SetTextureAlphaMod(paddle1Texture, 150);
    SDL_SetTextureAlphaMod(paddle2Texture, 150);
    std::cout << "GraphicsGame constructor left\n" << std::flush;
}

GraphicsGame::~GraphicsGame(){
    std::cout << "-------- GraphicsGame destructor called\n" << std::flush;
    // Wavefunction related
    SDL_DestroyTexture(wavefunctionTexture);
    SDL_DestroyTexture(potTexture);
    SDL_DestroyTexture(magTexture);

    // Score bar related
    SDL_DestroyTexture(scoreBarTexture);

    // White texture related
    SDL_DestroyTexture(whiteMaskTexture);

    // Paddle related
    SDL_DestroyTexture(paddle1Texture);
    SDL_DestroyTexture(paddle2Texture);
}




void GraphicsGame::update_wavefunction(buffer *buffer_wf1){
    std::cout << "GraphicsGame::update_wavefunction entered\n" << std::flush;

    int pitch;
    int PIXEL_SIZE = 4;
    uint8_t *buf;
    SDL_LockTexture(wavefunctionTexture, NULL,  (void **)&buf, &pitch);
    std::cout << "GraphicsGame::update_wavefunction after wavefunction lock\n" << std::flush;

    buffer_wf1->update_tail();

    std::cout << "GraphicsGame::update_wavefunction after update_tail\n" << std::flush;

    for(int i=0; i < height; i++){
        for(int j=0; j < width; j++){
            int n = (i*width + j);
            // ARGB
            buf[PIXEL_SIZE*n+0] = buffer_wf1->read_data[n]; // A
            buf[PIXEL_SIZE*n+1] = 0; // R
            buf[PIXEL_SIZE*n+2] = buffer_wf1->read_data[n]; // G
            buf[PIXEL_SIZE*n+3] = 0; // B
        }
    }

    std::cout << "after looploop\n"<< std::flush;

    SDL_UnlockTexture(wavefunctionTexture);
    std::cout << "GraphicsGame::update_wavefunction left\n" << std::flush; 
}



void GraphicsGame::update_score(){

    int pitch;
    uint8_t *buf;
    SDL_LockTexture(scoreBarTexture, NULL,  (void **)&buf, &pitch);

    // Vertical line to separate from the game screen
    for(int i=0; i < height; i++){
        int n = i*score_width;        
        buf[pixel_size*n+0] = 255; // A
        buf[pixel_size*n+1] = 255; // R
        buf[pixel_size*n+2] = 255; // G
        buf[pixel_size*n+3] = 255; // B   
    }



    int height_top = (int)(score_top*height);
    int height_bot = (int)(score_bot*height);
    std::cout << "score top bot: " << height_top << " " << height_bot << "\n";



    for(int j=1; j < score_width; j++){
        // for(unsigned i=0; i < height_top; i++){
        for(int i=height-height_top; i < height; i++){
            int n = i*score_width + j;
            buf[pixel_size*n+0] = 255; // A
            buf[pixel_size*n+1] = P1R*0.8; // R
            buf[pixel_size*n+2] = P1G*0.8; // G
            buf[pixel_size*n+3] = P1B*0.8;   // B
        }
    }

    for(int j=1; j < score_width; j++){
        for(int i=height_bot; i < height-height_top; i++){
            int n = i*score_width + j;
            buf[pixel_size*n+0] = 255; // A
            buf[pixel_size*n+1] = 0;   // R
            buf[pixel_size*n+2] = 0;   // G
            buf[pixel_size*n+3] = 0;   // B
        }
    }


    for(int j=1; j < score_width; j++){
        for(int i=0; i < height_bot; i++){
            int n = i*score_width + j;
            buf[pixel_size*n+0] = 255; // A
            buf[pixel_size*n+1] = P2R*0.8;   // R
            buf[pixel_size*n+2] = P2G*0.8; // G
            buf[pixel_size*n+3] = P2B*0.8; // B
        }
    }




    // Horizontal line to separate the scores
    int mid = height/2;
    for(int i=0; i < score_width; i++){
        int n = mid*score_width + i;
        buf[pixel_size*n+0] = 255; // A
        buf[pixel_size*n+1] = 255; // R
        buf[pixel_size*n+2] = 255; // G
        buf[pixel_size*n+3] = 255; // B   
    }


    SDL_UnlockTexture(scoreBarTexture);
}




void GraphicsGame::draw_paddle(int x, int y, SDL_Texture* texture){
    std::cout << "graphics: entered draw_paddle\n" << std::flush;

    int paddle_width = 100;
    int paddle_height = 20;
    
    
    //Render texture to screen
    SDL_Rect srcrect;
    SDL_Rect dstrect;

    // Image that we want to blit
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = paddle_width;
    srcrect.h = paddle_height;

    dstrect.x = x - paddle_width/2;
    dstrect.y = y - paddle_height/2;
    dstrect.w = paddle_width;
    dstrect.h = paddle_height;

    std::cout << "graphics: left draw_paddle\n" << std::flush;
    SDL_RenderCopy(rend, texture, &srcrect, &dstrect);
}




void GraphicsGame::update_potential(int x, int y, int dx, int dy, buffer *buffer_pot1){
    std::cout << "GraphicsGame::update_potential entered\n" << std::flush;

    int pitch;
    uint8_t *buf;
    SDL_LockTexture(potTexture, NULL,  (void **)&buf, &pitch);
    buffer_pot1->update_tail_by1();

    int n, m;
    for(int i=0; i<dx; i++){
        for(int j=0; j<dy; j++){
            n = x+i + (y+j)*width;
            m = i + dx*j;
            // std::cout << "nm:" << n << "," << m << " " << (int)buffer_pot->read_data[m] << "\n" << std::flush;
            // ARGB
            buf[pixel_size*n+0] = buffer_pot1->read_data[m]; // A
            buf[pixel_size*n+1] = 0; // R
            buf[pixel_size*n+2] = 0; // G
            buf[pixel_size*n+3] = buffer_pot1->read_data[m]; // B
        }
    }
    std::cout << "GraphicsGame::update_potential left\n" << std::flush;

    SDL_UnlockTexture(potTexture);
    
}

void GraphicsGame::update_magnetic(int x, int y, int dx, int dy, buffer *buffer_mag1){
    std::cout << "GraphicsGame::update_magnetic entered\n" << std::flush;

    int pitch;
    uint8_t *buf;
    SDL_LockTexture(magTexture, NULL,  (void **)&buf, &pitch);
    buffer_mag1->update_tail_by1();

    int n, m;
    for(int i=0; i<dx; i++){
        for(int j=0; j<dy; j++){
            n = x+i + (y+j)*width;
            m = i + dx*j;
            // ARGB
            buf[pixel_size*n+0] = buffer_mag1->read_data[m]; // A
            buf[pixel_size*n+1] = buffer_mag1->read_data[m]; // R
            buf[pixel_size*n+2] = 0; // G
            buf[pixel_size*n+3] = 0; // B
        }
    }
    std::cout << "GraphicsGame::update_magnetic left\n" << std::flush;

    SDL_UnlockTexture(magTexture);
    
}
 void GraphicsGame::update_white(int alpha){
    SDL_SetTextureAlphaMod( whiteMaskTexture, alpha);
}

void GraphicsGame::update(){
    std::cout << "GraphicsGame::update entered\n" << std::flush;
    SDL_Rect srcrect{0,0,(int)width, (int)height};
    std::cout << "GraphicsGame::update after srcrect\n" << std::flush;

    // Score rectangles
    SDL_Rect srcrect1{0,0,(int)score_width, (int)height};
    SDL_Rect dstrect1{(int)width,0,(int)score_width, (int)height};
    std::cout << "GraphicsGame::update after score rectangles\n" << std::flush;
    
    SDL_RenderClear(rend);
    std::cout << "GraphicsGame::update after SDL_RenderClear\n" << std::flush;
    
    
    // updates wavefunction and potential textures    
    
    // SDL_SetTextureAlphaMod( wavefunctionTexture, 255);
    
    SDL_RenderCopy(rend, wavefunctionTexture, &srcrect, &srcrect);
    std::cout << "GraphicsGame::update after wavefunctionText\n" << std::flush;

    
    SDL_RenderCopy(rend, potTexture, &srcrect, &srcrect);
    std::cout << "GraphicsGame::update after potText\n" << std::flush;

    
    SDL_RenderCopy(rend, magTexture, &srcrect, &srcrect);
    std::cout << "GraphicsGame::update after magText\n" << std::flush;


    // SDL_SetTextureAlphaMod( scoreBarTexture, 255);
    SDL_RenderCopy(rend, scoreBarTexture, &srcrect1, &dstrect1);
    
    // Paddles
    std::cout << "GraphicsGame::update after scoreBarTexture\n" << std::flush;

    draw_paddle(x0, y0, paddle1Texture);
    draw_paddle(x1, y1, paddle2Texture);
    std::cout << "GraphicsGame::update after draw_paddle\n" << std::flush;
    

    // SDL_SetTextureAlphaMod( whiteMaskTexture, 255-alpha );
    SDL_RenderCopy(rend, whiteMaskTexture, &srcrect, &srcrect);    
    

    SDL_RenderPresent(rend);
    
    std::cout << "GraphicsGame::update left\n" << std::flush;
}





void GraphicsGame::reset(){
    std::cout << "GraphicsGame::reset entered\n" << std::flush;

    score_top = 0.0;
    score_bot = 0.0;
    
    int pitch;
    int PIXEL_SIZE = 4;
    uint8_t *buf;

    SDL_LockTexture(wavefunctionTexture, NULL,  (void **)&buf, &pitch);
    for(int i=0; i < height*width*PIXEL_SIZE; i++) buf[i] = 0;
    SDL_UnlockTexture(wavefunctionTexture);

    SDL_LockTexture(potTexture, NULL,  (void **)&buf, &pitch);
    for(int i=0; i < height*width*PIXEL_SIZE; i++) buf[i] = 0;
    SDL_UnlockTexture(potTexture);
    
    SDL_LockTexture(magTexture, NULL,  (void **)&buf, &pitch);
    for(int i=0; i < height*width*PIXEL_SIZE; i++) buf[i] = 0;
    SDL_UnlockTexture(magTexture);
    
    std::cout << "GraphicsGame::reset left\n" << std::flush; 
}
