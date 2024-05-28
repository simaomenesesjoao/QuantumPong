#define CL_HPP_TARGET_OPENCL_VERSION 210
#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h> // sleep function

// OpenCL has to be included before X11! Are there function name conflicts??
#include "opencl.hpp"
#include "x11.hpp"

void handleEvent(x11 *vis, kpm *eng) {
    unsigned ix = eng->Lx/2;
    unsigned iy = eng->Ly/2;
    float kx = 0.5;
    float ky = 1.0;
    float broad = 20.0;

    int paddle_x = 100;
    int paddle_y_bot = 50;
    int paddle_y_top = eng->Ly-50;
    int paddle_width = 100;
    int paddle_height = 20;

    int d = 20; // Movement increment 
    KeyCode keyCode;
    int xt, xb, yt, yb;
    uint32_t KeyEventCode;
    while (XPending(vis->display) > 0){
        //std::cout << "inside loop " << "\n";
        XEvent event;
        XNextEvent(vis->display, &event);

        //std::cout << "event type: " << event.type << "\n";
        switch (event.type) {
            case KeyPress:
                // Handle key press event
                keyCode = event.xkey.keycode;
                KeyEventCode = keyCode; 

                //std::cout << KeyEventCode << "\n"; 
                xt = eng->top_player_x;
                yt = eng->top_player_y;
                xb = eng->bot_player_x;
                yb = eng->bot_player_y;

                // Update player on top
                switch(KeyEventCode){
                    case 19:
                        //std::cout << "0: win bot\n";
                        eng->win = true;
                        break;

                    case 25:
                        //std::cout << "W\n";
                        eng->update_paddles(xt, yt-d, xb, yb);
                        break;
                    case 38:
                        //std::cout << "A\n";
                        eng->update_paddles(xt-d, yt, xb, yb);
                        break;
                    case 39:
                        //std::cout << "S\n";
                        eng->update_paddles(xt, yt+d, xb, yb);
                        break;
                    case 40:
                        //std::cout << "D\n";
                        eng->update_paddles(xt+d, yt, xb, yb);
                        break;

                    case 31:
                        //std::cout << "I\n";
                        eng->update_paddles(xt, yt, xb, yb-d);
                        break;
                    case 44:
                        //std::cout << "J\n";
                        eng->update_paddles(xt, yt, xb-d, yb);
                        break;
                    case 45:
                        //std::cout << "K\n";
                        eng->update_paddles(xt, yt, xb, yb+d);
                        break;
                    case 46:
                        //std::cout << "L\n";
                        eng->update_paddles(xt, yt, xb+d, yb);
                        break;

                    case 28:
                        std::cout << "T: absorption and scoring disabled\n";
                        eng->absorb_on = false;
                        break;

                    case 29:
                        std::cout << "Y: absorption and scoring enabled\n";
                        eng->absorb_on = true;
                        break;
                        
                    case 30:
                        eng->pressed_showcase = !eng->pressed_showcase;
                        std::cout << "U: Toggle wf: " << eng->pressed_showcase << "\n";
                        break;

                    // Change norm
                    case 10:
                        eng->modifier *= 1.5;
                        std::cout << "current modifier: " << eng->modifier << "\n";
                        break;
                    case 11:
                        eng->modifier /= 1.5;
                        std::cout << "current modifier: " << eng->modifier << "\n";
                        break;
                    case 12:
                        eng->radB *= 1.5;
                        std::cout << "3: Increasing B radius. Current B radius: " << eng->radB << "\n";
                        break;
                    case 13:
                        eng->radB /= 1.5;
                        std::cout << "4: Decreasing B radius. Current B radius: " << eng->radB << "\n";
                        break;

                    case 41:
                        eng->running = true;
                        std::cout << "F: running" << "\n";
                        break;
                    case 42:
                        eng->running = false;
                        std::cout << "G: stopping" << "\n";
                        break;

                    // reset
                    case 27:
                        std::cout << "R: restart" << "\n";
                        eng->reset_state();
                        eng->set_H();
                        eng->initialize_wf(ix, iy, kx, ky, broad);
                        eng->init_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot, paddle_width, paddle_height);
                        eng->update_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot);

                        eng->win = false;


                        break;
                }
                break;

            case ButtonPress:
                //std::cout << "Button pressed\n";
                if (event.xbutton.button == Button1) {
                    int mouseX = event.xbutton.x;
                    int mouseY = event.xbutton.y;
                    unsigned w = 70;
                    float v = 0.9;
                    std::cout << "Left mouse button press at X = " << mouseX << ", Y = " << mouseY << std::endl;
                    eng->set_local_pot(mouseX, mouseY, w, w, v);
                }

                if (event.xbutton.button == Button3) {
                    int mouseX = event.xbutton.x;
                    int mouseY = event.xbutton.y;
                    float v = 0.005;
                    std::cout << "Right mouse button press at X = " << mouseX << ", Y = " << mouseY << std::endl;
                    eng->set_local_B(mouseX, mouseY, v);
                }

                break;
        }
    }
}

int main(int argc, char** argv){

    unsigned Lx = 400;
    unsigned Ly = 700;
    //unsigned Lx = 1000;
    //unsigned Ly = 1600;
    unsigned pad = 1;
    unsigned local = 2;

    // Time evolution operator parameters
    float dt = 2.0;
    unsigned Ncheb = 10;

    unsigned height = Ly;
    unsigned width = Lx + 30;

    // initial wavefunction parameters: center, momentum, spread
    //unsigned ix = Lx/2;
    //unsigned iy = Ly/2;
    //float kx = 0.5;
    //float ky = 1.0;
    //float broad = 20.0;

    unsigned ix = Lx/2;
    unsigned iy = 100;
    float kx = 0.0;
    float ky = -1.0;
    float broad = 20.0;
    
    //unsigned ix = 120;
    //unsigned iy = 500;
    //float kx = 0.1;
    //float ky = 0.4;
    //float broad = 20.0;

    unsigned Ntimes = 200000;

    bool showcase = false;

    kpm engine;
    engine.init_cl();
    engine.showcase = showcase;
    engine.init_geometry(Lx, Ly, pad, local);
    engine.init_window(Lx, Ly);
    engine.set_hamiltonian_sq();
    engine.init_buffers();
    engine.init_kernels();
    engine.initialize_tevop(dt, Ncheb);
    engine.set_H();
    if(showcase) engine.initialize_pot_from();
    engine.initialize_wf(ix, iy, kx, ky, broad);

    unsigned paddle_x = 100;
    unsigned paddle_y_bot = 50;
    unsigned paddle_y_top = Ly-50;
    unsigned paddle_width = 100;
    unsigned paddle_height = 20;
    engine.init_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot, paddle_width, paddle_height);
    engine.update_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot);

    x11 visual;
    visual.init(width, height);
    float max, threshhold;
    max = 0;
    threshhold = 0;

    for(unsigned j=0; j<Ntimes; j++){
        handleEvent(&visual, &engine);
        engine.get_norm(&max, &threshhold);


        if(engine.norm_top > 0.5){
        //if(engine.norm_top > 0.5 || engine.win){
            usleep(100000);
            visual.set_victory_screen(0);
            if(j%10==0) std::cout << "Top player won! Press R to reset.\n";

        } else if(engine.norm_bot > 0.5){
        //} else if(engine.norm_bot > 0.5 || engine.win){
            usleep(100000);
            visual.set_victory_screen(1);
            if(j%10==0) std::cout << "Bottom player won! Press R to reset.\n";
        } else {
            
            engine.set_max(threshhold);
            engine.update_pixel(visual.image->data, visual.width, visual.height);
            if(engine.pressed_showcase) engine.clear_wf_away_from_pot(visual.image->data, visual.width, visual.height);
            if(engine.running) engine.iterate_time(3);
            if(engine.absorb_on) engine.absorb();
        }

        visual.update();
        //if(j%10==0){
            //std::cout << "time, norm, max, norm_top, norm_bot: " << j << " " << norm << " " << max << " " << engine.norm_top << " " << engine.norm_bot << "\n";
        //}
        //usleep(50000);
    }



    visual.finish();
    return 0;
}

