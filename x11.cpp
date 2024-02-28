#include <iostream>
#include "x11.hpp"


void x11::init(unsigned w, unsigned h){
    width = w;
    height = h;
    // X11 initialization

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

    // Create an initial XImage 
    image = XCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, 0), ZPixmap, 0, NULL, width, height, 32, 0);
    image->data = (char *)malloc(image->height * image->bytes_per_line);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int offset = y * image->bytes_per_line + (x * 4);  // Assuming 32 bits per pixel (4 bytes)

            // Found how the data is organized!
            image->data[offset+0] = 255;
            image->data[offset+1] = 0;
            image->data[offset+2] = 120;
            image->data[offset+3] = 120;

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
}
void x11::update(){
    //std::cout << "updating: " << width << " " << height << "\n"<< std::flush;
    XPutImage(display, window, DefaultGC(display, 0), image, 0, 0, 0, 0, width, height);
    XFlush(display);
}

void x11::finish(){
    // Clean up
    free(image->data);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}
