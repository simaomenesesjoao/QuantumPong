#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <iostream>
#include <SDL2/SDL.h>
#include "../macros.hpp"
#include "graphics.hpp"


Graphics::Graphics(int width, int height, int player_number, event_queue *eq):    
    width(width), height(height), player_number(player_number), eventQueue(eq),
    aux(width, height),
    lobby(&aux, width, height, player_number),
    game(&aux, width, height, player_number),
    end(&aux, width, height, player_number){

    SDL_listener_running = true;
    thread_listener = std::thread(&Graphics::listen_SDL_events, this, eq);

    score_width = aux.score_width;
    window_height = aux.window_height;
    window_width = aux.window_width;
    win = aux.win;
    renderer = aux.renderer;

}

Graphics::~Graphics(){

    // Kill the listener thread
    SDL_listener_running = false; 
    thread_listener.join();
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();	
}


void Graphics::listen_SDL_events(event_queue *eventQueue){
    std::cout << "entered SDL listener\n";
    // Wrapper around SDL events to make them compatible with QuantumPong events
    
    SDL_Event event;

    while(SDL_listener_running){
        while(SDL_PollEvent(&event)){

            switch (event.type) {

                case SDL_QUIT:
                    eventQueue->add_event(Event<EV_EXIT>(player_number).buffer_b);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    switch(event.button.button){
                        case SDL_BUTTON_LEFT:
                            eventQueue->add_event(Event<EV_MOUSEBUTTONDOWN>(player_number, BUTTON_LEFT, event.button.x, event.button.y).buffer_b);
                            break;

                        case SDL_BUTTON_RIGHT:
                            eventQueue->add_event(Event<EV_MOUSEBUTTONDOWN>(player_number, BUTTON_RIGHT, event.button.x, event.button.y).buffer_b);
                            break;

                        default:
                            break;
                    }


                case SDL_KEYDOWN:
                    std::cout << "Key " << (char)event.key.keysym.sym << " " << ((event.key.state == SDL_PRESSED) ? "pressed" : "released") << "\n";

                    switch(event.key.keysym.sym){
                        case SDLK_SPACE:
                            eventQueue->add_event(Event<EV_PRESSED_SPACE>(player_number).buffer_b);
                            break;

                        case SDLK_w:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_w).buffer_b);
                            break;

                        case SDLK_a:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_a).buffer_b);
                            break;

                        case SDLK_s:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_s).buffer_b);
                            break;

                        case SDLK_d:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_d).buffer_b);
                            break;
                            
                        case SDLK_n:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_n).buffer_b);
                            break;

                        case SDLK_ESCAPE:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_esc).buffer_b);
                            break;

                        case SDLK_RETURN:
                            eventQueue->add_event(Event<EV_PRESSED_KEY>(player_number, KEY_return).buffer_b);
                            break;

                        default:
                            break;
                    }

                    break;

                default:
                    break;
            
            }
        }
    }
    
    std::cout << "Left SDL listener\n";
}