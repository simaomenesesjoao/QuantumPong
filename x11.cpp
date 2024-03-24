#include <iostream>
#include "x11.hpp"
#include <X11/Xutil.h>
#include <cstring>
#include <algorithm>


void x11::init(unsigned w, unsigned h){
    width = w;
    height = h;
    // X11 initialization
    transp = 0;

    display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Unable to open display" << std::endl;
        exit(EXIT_FAILURE);
    }

    root = DefaultRootWindow(display);
    // Create a window
    window = XCreateSimpleWindow(display, root, 0, 0, width, height, 1, BlackPixel(display, 0), WhitePixel(display, 0));

    // Select events to listen for
    //XSelectInput(display, window, ExposureMask | KeyPressMask );
    XSelectInput(display, window, ButtonPressMask | KeyPressMask | ExposureMask);
    // Map the window to the screen
    XMapWindow(display, window);


// Check if the Window object is valid
if (window == 0) {
    fprintf(stderr, "Invalid window.\n");
}
    gc = XCreateGC(display, window, 0, NULL);
    if (gc == NULL) {
        fprintf(stderr, "Failed to create graphics context.\n");
    }


    // Create an initial XImage 
    image = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, 0), ZPixmap, 0, NULL, width, height, 32, 0);
    image->data = (char *)malloc(image->height * image->bytes_per_line);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int offset = y * image->bytes_per_line + (x * 4);  // Assuming 32 bits per pixel (4 bytes)

            // Found how the data is organized!
            image->data[offset+0] = 40;
            image->data[offset+1] = 40;
            image->data[offset+2] = 40;
            image->data[offset+3] = 0;

            // Found the endianness of each byte
            //image->data[offset+0] = 128+64;

            // Summary of alignment in memory: 
            // BGRA BGRA BGRA ...
            // B =  1  1  0  0  0 0 0 0 = 128+64
            // B = 128 64 32 16 8 4 2 1
        }
    }

    XPutImage(display, window, DefaultGC(display, 0), image, 0, 0, 0, 0, width, height);
    XFlush(display);

      /* read the image */
    //if (XpmReadFileToImage (display, "images/victory.xpm", &victory, NULL, NULL)) {
        //printf ("Error reading file\n");
        //exit (1);
    //}

    //XMapWindow(display, window);

}

void x11::update(){
    //std::cout << "updating: " << width << " " << height << "\n"<< std::flush;
    XPutImage(display, window, DefaultGC(display, 0), image, 0, 0, 0, 0, width, height);

    XFlush(display);
}

void x11::set_victory_screen(int player){

    int val;
    int offset;
    if(player == 0){
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                offset = y * image->bytes_per_line + (x * 4);  

                val = image->data[offset+0];
                if(val < 0) val += 255;
                image->data[offset+0] = std::min(255, val + 10);

                val = image->data[offset+1];
                if(val < 0) val += 255;
                image->data[offset+1] = std::min(122, val + 10);

                val = image->data[offset+2];
                if(val < 0) val += 255;
                image->data[offset+2] = std::max(0, val - 10);

            }
        }
    }



    if(player == 1){
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                offset = y * image->bytes_per_line + (x * 4);  

                val = image->data[offset+0];
                if(val < 0) val += 255;
                image->data[offset+0] = std::max(0, val - 10);

                val = image->data[offset+1];
                if(val < 0) val += 255;
                image->data[offset+1] = std::min(122, val + 10);

                val = image->data[offset+2];
                if(val < 0) val += 255;
                image->data[offset+2] = std::min(255, val + 10);

            }
        }
    }

}


void x11::finish(){
    // Clean up
    free(image->data);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}
