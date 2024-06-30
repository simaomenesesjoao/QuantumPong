#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <cstddef>
#include <iostream>
#include <unistd.h>
#include <SDL2/SDL.h>

#include "../macros.hpp"
#include "shared_memory.hpp"
#include "graphics.hpp"



graphics::graphics(shared_memory *mem){
    memory = mem;
}


void graphics::CreateTextureFromString(std::string textureText, SDL_Color textColor, SDL_Texture **texture, SDL_Surface **surface){
    *surface = TTF_RenderText_Blended( gFont, textureText.c_str(), textColor );
    if(*surface == NULL) 
        std::cout << "Problem generating surface\n";

    *texture = SDL_CreateTextureFromSurface( rend, *surface );
	if(*texture == NULL) 
        std::cout << "Problem generating texture\n";

}


void graphics::init(unsigned WIDTH, unsigned HEIGHT){
    width = WIDTH;
    height = HEIGHT;

    x0 = width/2;
    x1 = width/2;
    y0 = height/2;
    y1 = height/2;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
        printf("error initializing SDL: %s\n", SDL_GetError()); 
    }

    win = SDL_CreateWindow("Quantum Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    rend = SDL_CreateRenderer(win, -1, render_flags);
    wavefunctionTexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);


    PIXEL_SIZE = 4;
    Nbytes = width*height*PIXEL_SIZE;

    buffer_pixels_wavefunction = new uint8_t[Nbytes];         // buffer with pixels to put on screen
    buffer_pixels_potential = new uint8_t[Nbytes];         // buffer with pixels to put on screen
    buffer_potential = new int[width*height];    // buffer with potential value
    buffer_wavefunction = new int[width*height]; // buffer with wavefunction value
    buffer_SDL = new int[HEADER_LEN];            // buffer with SDL events

    for(int i=0; i<width*height; i++){
        buffer_potential[i] = 0;
    }


    //Initialize SDL_ttf and font-related things
    if( TTF_Init() == -1 ){
        printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
    }

    gFont = TTF_OpenFont( "Arial.TTF", 28 );
	if( gFont == NULL ){
		printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
	}


    // Render the text
    SDL_Color textColor = { 255, 255, 255}; 
	CreateTextureFromString("Disconnected", textColor, &DisconnectedStatusTexture, &DisconnectedStatusSurface);
	CreateTextureFromString("Connected", textColor, &ConnectedStatusTexture, &ConnectedStatusSurface);
	CreateTextureFromString("In end screen", textColor, &inEndStatusTexture, &inEndStatusSurface);
    CreateTextureFromString("Wants new game", textColor, &WantNewStatusTexture, &WantNewStatusSurface);
	CreateTextureFromString("Player 1:", textColor, &Player1Texture, &Player1Surface);
	CreateTextureFromString("Player 2:", textColor, &Player2Texture, &Player2Surface);

    // Create the victory and defeat screens
    SDL_Surface* surface;
    surface = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 0));
    victoryTexture = SDL_CreateTextureFromSurface(rend, surface);
    
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 255, 255));
    defeatTexture = SDL_CreateTextureFromSurface(rend, surface);


    // Create the white mask
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));
    whiteMaskTexture = SDL_CreateTextureFromSurface(rend, surface);


    // Create the paddles
    int paddle_width = 100;
    int paddle_height = 20;
    surface = SDL_CreateRGBSurface(0, paddle_width, paddle_height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 255, 0, 255));
    paddle1Texture = SDL_CreateTextureFromSurface(rend, surface);

    SDL_FreeSurface(surface);

}

void graphics::finalize(){
    // Finalize SDL
    // delete [] buffer_pixels; // automatically taken care of by SDL_LockTexture?
    delete [] buffer_SDL;
    delete [] buffer_potential;
    delete [] buffer_wavefunction;

    SDL_DestroyTexture(wavefunctionTexture);

    SDL_DestroyTexture(DisconnectedStatusTexture);
    SDL_DestroyTexture(ConnectedStatusTexture);
    SDL_DestroyTexture(inEndStatusTexture);
    SDL_DestroyTexture(Player1Texture);
    SDL_DestroyTexture(Player2Texture);
    SDL_DestroyTexture(placeholderTexture);
    SDL_DestroyTexture(victoryTexture);
    SDL_DestroyTexture(defeatTexture);
    SDL_DestroyTexture(whiteMaskTexture);

    SDL_FreeSurface(DisconnectedStatusSurface);
    SDL_FreeSurface(ConnectedStatusSurface);
    SDL_FreeSurface(inEndStatusSurface);
    SDL_FreeSurface(Player1Surface);
    SDL_FreeSurface(Player2Surface);
    SDL_FreeSurface(placeholderSurface);

	TTF_CloseFont(gFont);
	gFont = NULL;

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();	
};




void graphics::update_lobby(int p1Status, int p2Status){
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



    // CHANGE: reuse the variables from PlayerStateMachine. maybe put them in macros.hpp
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

void graphics::endScreen(bool won){
    std::cout << "graphics::endScreen\n" << std::flush;


    SDL_RenderClear(rend);
    if(won)
	    SDL_RenderCopy( rend, victoryTexture, NULL, NULL);
    if(!won)
        SDL_RenderCopy( rend, defeatTexture, NULL, NULL);
    SDL_RenderPresent(rend);


}



void graphics::update_wavefunction(uint8_t *buffer1){
    std::cout << "graphics: entered update_wavefunction\n" << std::flush;
    for(int i=0; i<width*height; i++) buffer_wavefunction[i] = (int)buffer1[i];
    std::cout << "graphics: left update_wavefunction\n" << std::flush;
}

void graphics::draw_wavefunction(){

    int pitch;
    // A função SDL_LocKTexture aloca a memoria interiormente!! não pode ser estática
    SDL_LockTexture(wavefunctionTexture, NULL,  (void **)&buffer_pixels_wavefunction, &pitch);

    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            buffer_pixels_wavefunction[PIXEL_SIZE*n+0] = 0; // A
            buffer_pixels_wavefunction[PIXEL_SIZE*n+1] = 0; // R
            buffer_pixels_wavefunction[PIXEL_SIZE*n+2] = buffer_wavefunction[n]; // G
            buffer_pixels_wavefunction[PIXEL_SIZE*n+3] = 0; // B
        }
    }

    SDL_UnlockTexture(wavefunctionTexture);
    SDL_RenderCopy(rend, wavefunctionTexture, NULL, NULL);
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

    int pitch;
    // A função SDL_LocKTexture aloca a memoria interiormente!! não pode ser estática
    SDL_LockTexture(potTexture, NULL,  (void **)&buffer_pixels_potential, &pitch);

    for(unsigned i=0; i < height; i++){
        for(unsigned j=0; j < width; j++){
            unsigned n = (i*width + j);
            
            // ARGB
            // buffer_pixels[PIXEL_SIZE*n+0] = 0; // A
            // buffer_pixels[PIXEL_SIZE*n+1] += 0; // R
            // buffer_pixels[PIXEL_SIZE*n+2] += 0; // G
            buffer_pixels_potential[PIXEL_SIZE*n+3] += buffer_potential[n]; // B
        }
    }
    std::cout << "graphics: left draw_potential\n" << std::flush;

    SDL_UnlockTexture(potTexture);
    SDL_RenderCopy(rend, potTexture, NULL, NULL);
}





void graphics::draw_paddle(int x, int y, SDL_Texture* texture){
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

    dstrect.x = x - paddle_width/2;;
    dstrect.y = y - paddle_height/2;;
    dstrect.w = paddle_width;
    dstrect.h = paddle_height;

    std::cout << "graphics: left draw_paddle\n" << std::flush;
    SDL_RenderCopy(rend, texture, &srcrect, &dstrect);
}


void graphics::update(){
    std::cout << "graphics: entered update\n" << std::flush;

    SDL_RenderClear(rend);
    
    // updates wavefunction and potential textures
    SDL_SetTextureAlphaMod( wavefunctionTexture,alpha );
    draw_wavefunction();

    SDL_SetTextureAlphaMod( whiteMaskTexture, 255-alpha );
    SDL_RenderCopy(rend, whiteMaskTexture, NULL, NULL);

    // draw_potential();
    
    // draw_paddle(x0, y0, paddle1Texture);
    // draw_paddle(x1, y1, paddle2Texture);

    
    SDL_RenderPresent(rend);
    
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
                        
                    case SDLK_n:
                        std::cout << " pressed n\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_n;
                        break;

                    case SDLK_ESCAPE:
                        std::cout << " pressed ESCAPE\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_esc;
                        break;

                    case SDLK_RETURN:
                        std::cout << " pressed RETURN\n";
                        buffer_SDL[0] = EV_PRESSED_KEY;
                        buffer_SDL[1] = KEY_return;
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
