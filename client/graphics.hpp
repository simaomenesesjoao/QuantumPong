
class graphics {

    public:
        SDL_Window* win;
        SDL_Renderer* rend;
        SDL_Texture* tex;

        unsigned width, height;
        unsigned Nbytes;
        uint8_t *buffer;
        unsigned PIXEL_SIZE;
        shared_memory *memory;

    graphics(shared_memory*);
    void init(unsigned, unsigned);
    void finalize();
    void update();
    bool get_one_SDL_event();


    void set_screen(unsigned, unsigned, unsigned);

};
