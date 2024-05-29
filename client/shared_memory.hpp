class shared_memory{
    public:
        uint8_t *header;
        unsigned header_length;

        uint8_t *buffer;
        unsigned buffer_length;

        int *SDL_buffer;
        unsigned SDL_buffer_length;

        int server_status;

    //shared_memory(unsigned);
};
