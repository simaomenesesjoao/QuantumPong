
class event_queue{
    public:
        unsigned tail, head, queue_size;
        int **events;
        std::mutex mutex;

    event_queue(unsigned size);
    ~event_queue();
    
    int add_event(int, int*);
    int add_event(int);
    int add_event(int, int);
    int add_event(int, int, int);

    int print_queue_data();
    void read(int*, int**);

};

