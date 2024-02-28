#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h> // sleep function

// OpenCL has to be included before X11! Are there function name conflicts??
#include "opencl.hpp"
#include "x11.hpp"
//#include <X11/keysym.h>

void handleEvent(x11 *vis, kpm *eng) {
    unsigned ix = eng->Lx/2;
    unsigned iy = eng->Ly/2;
    float kx = 0.5;
    float ky = 1.0;
    float broad = 20.0;

    float current_max;
    KeyCode keyCode;
    unsigned xt, xb, yt, yb;
    uint32_t KeyEventCode;
    while (XPending(vis->display) > 0){
        std::cout << "inside loop " << "\n";
        XEvent event;
        XNextEvent(vis->display, &event);

        std::cout << "event type: " << event.type << "\n";
        switch (event.type) {
            case KeyPress:
                // Handle key press event
                keyCode = event.xkey.keycode;
                KeyEventCode = keyCode; 

                std::cout << KeyEventCode << "\n"; 
                xt = eng->top_player_x;
                yt = eng->top_player_y;
                xb = eng->bot_player_x;
                yb = eng->bot_player_y;

                // Update player on top
                switch(KeyEventCode){
                    case 25:
                        std::cout << "W\n";
                        eng->update_paddles(xt, yt-10, xb, yb);
                        break;
                    case 38:
                        std::cout << "A\n";
                        eng->update_paddles(xt-10, yt, xb, yb);
                        break;
                    case 39:
                        std::cout << "S\n";
                        eng->update_paddles(xt, yt+10, xb, yb);
                        break;
                    case 40:
                        std::cout << "D\n";
                        eng->update_paddles(xt+10, yt, xb, yb);
                        break;

                    case 31:
                        std::cout << "I\n";
                        eng->update_paddles(xt, yt, xb, yb-10);
                        break;
                    case 44:
                        std::cout << "J\n";
                        eng->update_paddles(xt, yt, xb-10, yb);
                        break;
                    case 45:
                        std::cout << "K\n";
                        eng->update_paddles(xt, yt, xb, yb+10);
                        break;
                    case 46:
                        std::cout << "L\n";
                        eng->update_paddles(xt, yt, xb+10, yb);
                        break;

                    // Change norm
                    case 10:
                        current_max = eng->max;
                        eng->set_max(current_max/2);
                        std::cout << "current max: " << current_max << "\n";
                        break;
                    case 11:
                        current_max = eng->max;
                        eng->set_max(current_max*2);
                        std::cout << "current max: " << current_max << "\n";
                        break;

                    case 27:
                        std::cout << "reset" << current_max << "\n";
                        eng->set_H();
                        eng->initialize_wf(ix, iy, kx, ky, broad);
                        eng->reset_state();

                        break;
                }
                break;

            case ButtonPress:
                std::cout << "Button pressed\n";
                if (event.xbutton.button == Button1) {
                    int mouseX = event.xbutton.x;
                    int mouseY = event.xbutton.y;
                    unsigned w = 70;
                    float v = 4.0;
                    std::cout << "Left mouse button press at X = " << mouseX << ", Y = " << mouseY << std::endl;
                    eng->set_local_pot(mouseX, mouseY, w, w, v);
                }

                break;
        }
    }
}

int main(int argc, char** argv){

    unsigned Lx = 400;
    unsigned Ly = 700;
    unsigned pad = 1;
    unsigned local = 2;

    // Time evolution operator parameters
    float dt = 2.0;
    unsigned Ncheb = 10;

    unsigned height = Ly;
    unsigned width = Lx;

    // initial wavefunction parameters: center, momentum, spread
    unsigned ix = Lx/2;
    unsigned iy = Ly/2;
    float kx = 0.5;
    float ky = 1.0;
    float broad = 20.0;

    unsigned Ntimes = 20000;

    kpm engine;
    engine.init_cl();
    engine.init_geometry(Lx, Ly, pad, local);
    engine.init_window(width, height);
    engine.set_hamiltonian_sq();
    engine.init_buffers();
    engine.init_kernels();
    engine.initialize_tevop(dt, Ncheb);
    engine.set_H();
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
    float norm, max;
    norm = 0;
    max = 0;

    for(unsigned j=0; j<Ntimes; j++){
        handleEvent(&visual, &engine);
        engine.update_pixel(visual.image->data);
        norm = engine.get_norm(&max);

        if(engine.norm_top > 0.5){
            std::cout << "Bottom player won! Press R to reset.\n";
            usleep(1000000);
        } else if(engine.norm_bot > 0.5){
            std::cout << "Top player won! Press R to reset.\n";
            usleep(1000000);
        } else {
            engine.iterate_time(2);
        }
        engine.absorb();

        if(j%10==0){
            std::cout << "time, norm, max, norm_top, norm_bot: " << j << " " << norm << " " << max << " " << engine.norm_top << " " << engine.norm_bot << "\n";
        }
        visual.update();
        //usleep(50000);
    }



    visual.finish();
    return 0;
}

