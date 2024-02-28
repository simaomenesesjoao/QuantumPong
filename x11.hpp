#include <X11/Xlib.h>
class x11{
    //private:
    public:
    Display *display;
    Window root;
    Window window;
    XEvent event;

    XImage *image;
    unsigned width, height;

    void finish();
    void update();
    void init(unsigned, unsigned);

};
