#include <X11/Xlib.h>
class x11{
    //private:
    public:
    Display *display;
    Window root;
    Window window;
    XEvent event;
    
    float transp;

    XImage *image;
    int width, height;
    GC gc;

    void finish();
    void update();
    void set_victory_screen(int);
    void init(unsigned, unsigned);

};
